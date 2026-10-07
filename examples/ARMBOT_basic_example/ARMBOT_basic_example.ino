/*
 * TR: ARMBOT TEMEL ÖRNEK - Otomatik demo + Manuel kontrol
 *  - Açılışta OTOMATİK mod çalışır: kol önce her ekseni sırayla gösterir
 *    (taban, omuz, dirsek, kıskaç: bir uç, diğer uç, orta), ardından birkaç
 *    rastgele "evcil robot" hareketi yapar ve baştan başlar.
 *  - MINIBOT üzerindeki butona (B1 / GPIO0) basınca MANUEL moda geçer: kol
 *    olduğu yerde durur ve seri port komutlarıyla siz yönetirsiniz. Butona
 *    tekrar basınca otomatik moda döner.
 *  - Servolar her zaman yavaşça (adım adım) hedefe gider; loop hiç bloklanmaz,
 *    buton ve seri komutlar anında çalışır. Hareket ederken mavi LED yanar.
 *  - Seri port komutları (115200 baud). Türkçe veya İngilizce yazabilirsiniz:
 *      yardim           / help              -> komut listesi
 *      oto              / auto              -> otomatik mod
 *      manuel           / manual            -> manuel mod
 *      taban 90         / base 90           -> taban (eksen 1) açısı 0-180
 *      omuz 90          / shoulder 90       -> omuz (eksen 2) açısı 0-180
 *      dirsek 50        / elbow 50          -> dirsek (eksen 3) açısı 0-180
 *      kiskac ac        / gripper open      -> kıskacı aç
 *      kiskac kapat     / gripper close     -> kıskacı kapat
 *      kiskac 60        / gripper 60        -> kıskaç açısı 0-180
 *      ev               / home              -> başlangıç (kalibrasyon) pozu
 *      dur              / stop              -> kolu olduğu yerde durdur
 *      durum            / status            -> açıları yazdır
 *      dil              / lang              -> dili değiştir (Türkçe <-> English)
 *    Bir hareket komutu otomatik moddayken gelirse kol manuel moda geçer.
 *
 * EN: ARMBOT BASIC EXAMPLE - Automatic demo + Manual control
 *  - At startup AUTO mode runs: the arm first shows every axis in turn
 *    (base, shoulder, elbow, gripper: one end, the other end, middle), then
 *    makes a few random "robot pet" moves and starts over.
 *  - Press the button on the MINIBOT (B1 / GPIO0) to switch to MANUAL mode: the
 *    arm stops where it is and you drive it with serial commands. Press the
 *    button again to go back to auto mode.
 *  - The servos always move slowly (step by step) to their target; the loop
 *    never blocks, so the button and serial commands react instantly. The blue
 *    LED is on while the arm is moving.
 *  - Serial port commands (115200 baud). You can type Turkish or English:
 *      help             / yardim            -> command list
 *      auto             / oto               -> auto mode
 *      manual           / manuel            -> manual mode
 *      base 90          / taban 90          -> base (axis 1) angle 0-180
 *      shoulder 90      / omuz 90           -> shoulder (axis 2) angle 0-180
 *      elbow 50         / dirsek 50         -> elbow (axis 3) angle 0-180
 *      gripper open     / kiskac ac         -> open the gripper
 *      gripper close    / kiskac kapat      -> close the gripper
 *      gripper 60       / kiskac 60         -> gripper angle 0-180
 *      home             / ev                -> start (calibration) pose
 *      stop             / dur               -> stop the arm where it is
 *      status           / durum             -> print the angles
 *      lang             / dil               -> switch language (Turkish <-> English)
 *    A motion command received in auto mode switches the arm to manual mode.
 *
 * Bağlantı / Wiring: ARMBOT'un üzerindeki MINIBOT'a yükleyin (ESP8266).
 *   Taban / base GPIO5, omuz / shoulder GPIO4, dirsek / elbow GPIO12,
 *   kıskaç / gripper GPIO13, buzzer GPIO14, buton / button GPIO0, mavi LED / blue LED GPIO16.
 *   IOTBOT ile kullanım için / to use with an IOTBOT: IOTBOT_ARMBOT_Basic_Example.
 */

#include <ARMBOT.h> // ARMBOT kütüphanesi / ARMBOT library

ARMBOT armbot; // ARMBOT nesnesi / ARMBOT object

#define LED_PIN 16   // MINIBOT mavi LED / MINIBOT blue LED
#define BUTTON_PIN 0 // MINIBOT B1 butonu (basılıyken LOW) / MINIBOT B1 button (LOW while pressed)

// Dil seçimi: true = Türkçe, false = English. Seri porttan "dil" / "lang" ile de değişir.
// Language: true = Turkish, false = English. Can also be changed with "dil" / "lang".
bool turkish = true;
const char *L(const char *tr, const char *en) { return turkish ? tr : en; }

// Kıskaç açıları (diğer CODLAI örnekleriyle aynı: küçük açı = açık)
// Gripper angles (same as the other CODLAI examples: small angle = open)
const int GRIP_OPEN = 20;
const int GRIP_CLOSE = 120;
const int HOME_POSE[4] = {90, 90, 50, 60}; // Kalibrasyon pozu / calibration pose

// ---------------------------------------------------------------------------
// Servo hareket motoru / Servo motion engine
// Her eksen "current" açısından "target" açısına her stepMs'de 1° yaklaşır.
// Each axis moves 1° from "current" toward "target" every stepMs.
// ---------------------------------------------------------------------------
// 0 taban, 1 omuz, 2 dirsek, 3 kıskaç / 0 base, 1 shoulder, 2 elbow, 3 gripper
int current[4] = {90, 90, 50, 60};
int target[4] = {90, 90, 50, 60};
int stepMs = 20;         // Derece başına ms (büyük = yavaş) / ms per degree (bigger = slower)
uint32_t lastServoMs = 0;

void writeServo(int axis, int angle) {
  // Hız 0 = bekleme yapmadan doğrudan yaz / speed 0 = write directly, no waiting
  if (axis == 0) armbot.axis1Motion(angle, 0);
  else if (axis == 1) armbot.axis2Motion(angle, 0);
  else if (axis == 2) armbot.axis3Motion(angle, 0);
  else armbot.gripperMotion(angle, 0);
}

bool atTarget() {
  for (int i = 0; i < 4; i++)
    if (current[i] != target[i]) return false;
  return true;
}

void setPose(int base, int shoulder, int elbow, int gripper) {
  target[0] = constrain(base, 0, 180);
  target[1] = constrain(shoulder, 0, 180);
  target[2] = constrain(elbow, 0, 180);
  target[3] = constrain(gripper, 0, 180);
}

void freezeArm() { // Kolu olduğu yerde tut / hold the arm where it is
  for (int i = 0; i < 4; i++) target[i] = current[i];
}

void updateServos(uint32_t now) {
  if (now - lastServoMs < (uint32_t)stepMs) return;
  lastServoMs = now;
  for (int i = 0; i < 4; i++) {
    if (current[i] != target[i]) {
      current[i] += (target[i] > current[i]) ? 1 : -1;
      writeServo(i, current[i]);
    }
  }
}

// ---------------------------------------------------------------------------
// Otomatik demo / Automatic demo
// ---------------------------------------------------------------------------
// Her satır bir poz: taban, omuz, dirsek, kıskaç, bekleme (ms), açıklama.
// Each row is a pose: base, shoulder, elbow, gripper, hold time (ms), label.
struct DemoPose { int a[4]; uint16_t holdMs; const char *tr; const char *en; };
const DemoPose DEMO[] = {
    {{0, 90, 50, 60}, 1000, "Eksen 1 (taban): 0°", "Axis 1 (base): 0°"},
    {{180, 90, 50, 60}, 1000, "Eksen 1 (taban): 180°", "Axis 1 (base): 180°"},
    {{90, 90, 50, 60}, 1000, "Eksen 1 (taban): 90°", "Axis 1 (base): 90°"},
    {{90, 0, 50, 60}, 1000, "Eksen 2 (omuz): 0°", "Axis 2 (shoulder): 0°"},
    {{90, 180, 50, 60}, 1000, "Eksen 2 (omuz): 180°", "Axis 2 (shoulder): 180°"},
    {{90, 90, 50, 60}, 1000, "Eksen 2 (omuz): 90°", "Axis 2 (shoulder): 90°"},
    {{90, 90, 20, 60}, 1000, "Eksen 3 (dirsek): 20°", "Axis 3 (elbow): 20°"},
    {{90, 90, 180, 60}, 1000, "Eksen 3 (dirsek): 180°", "Axis 3 (elbow): 180°"},
    {{90, 90, 50, 60}, 1000, "Eksen 3 (dirsek): 50°", "Axis 3 (elbow): 50°"},
    {{90, 90, 50, 0}, 1000, "Kıskaç: 0° (açık)", "Gripper: 0° (open)"},
    {{90, 90, 50, 110}, 1000, "Kıskaç: 110° (kapalı)", "Gripper: 110° (closed)"},
    {{90, 90, 50, 60}, 1000, "Kıskaç: 60°", "Gripper: 60°"},
};
const int DEMO_LEN = sizeof(DEMO) / sizeof(DEMO[0]);
const int RANDOM_MOVES = 6; // Demo sonrası rastgele hareket sayısı / random moves after the demo

int demoIndex = 0;            // 0..DEMO_LEN-1 demo, sonrası rastgele / after that: random moves
uint32_t poseReachedMs = 0;   // Poza varış zamanı (0 = henüz varmadı) / time the pose was reached
uint16_t holdMs = 0;          // Bu pozda bekleme süresi / hold time at this pose

void startDemoStep() {
  poseReachedMs = 0;
  if (demoIndex < DEMO_LEN) {
    const DemoPose &p = DEMO[demoIndex];
    setPose(p.a[0], p.a[1], p.a[2], p.a[3]);
    holdMs = p.holdMs;
    stepMs = 20;
    Serial.println(String(L("Demo: ", "Demo: ")) + L(p.tr, p.en));
    if (demoIndex % 3 == 0) armbot.buzzerPlay(600 + demoIndex * 40, 60); // Yeni eksen sesi / new axis chirp
  } else {
    // Rastgele "evcil robot" hareketi: bir eksen, güvenli bir açı.
    // Random "robot pet" move: one axis, a safe angle.
    int axis = random(0, 4);
    int angle;
    if (axis == 2) angle = random(40, 140);      // Dirsek yere vurmasın / keep the elbow off the ground
    else if (axis == 3) angle = random(0, 110);
    else angle = random(20, 160);
    target[axis] = angle;
    holdMs = random(300, 1500);
    stepMs = 30;
    const char *namesTr[4] = {"taban", "omuz", "dirsek", "kıskaç"};
    const char *namesEn[4] = {"base", "shoulder", "elbow", "gripper"};
    Serial.println(String(L("Rastgele: ", "Random: ")) + L(namesTr[axis], namesEn[axis]) + " -> " + angle + "°");
    if (axis == 3) armbot.buzzerPlay(random(600, 1200), 50); // Kıskaç robotik bir ses çıkarır / gripper chirps
  }
}

void runAutoDemo(uint32_t now) {
  if (!atTarget()) return;
  if (poseReachedMs == 0) {
    poseReachedMs = now;
    return;
  }
  if (now - poseReachedMs >= holdMs) {
    demoIndex++;
    if (demoIndex >= DEMO_LEN + RANDOM_MOVES) {
      demoIndex = 0;
      Serial.println(L("Demo baştan başlıyor.", "Demo starts over."));
    }
    startDemoStep();
  }
}

// ---------------------------------------------------------------------------
// Seri komut okuyucu / Serial command reader
// Seri Monitör'ün satır sonu ayarı ne olursa olsun çalışır (NL, CR, ikisi, hiçbiri).
// Works with any Serial Monitor line-ending setting (NL, CR, both, none).
// ---------------------------------------------------------------------------
String cmdBuffer;
uint32_t lastCharMs = 0;

// Küçük harfe çevirir ve Türkçe harfleri sadeleştirir: "KISKAÇ" -> "kiskac"
// Lower-cases and simplifies Turkish letters: "KISKAÇ" -> "kiskac"
String normalizeCommand(String s) {
  s.trim();
  s.replace("İ", "i"); s.replace("I", "i"); s.replace("ı", "i");
  s.replace("Ş", "s"); s.replace("ş", "s");
  s.replace("Ğ", "g"); s.replace("ğ", "g");
  s.replace("Ü", "u"); s.replace("ü", "u");
  s.replace("Ö", "o"); s.replace("ö", "o");
  s.replace("Ç", "c"); s.replace("ç", "c");
  s.toLowerCase();
  return s;
}

bool readCommand(String &cmd) {
  while (Serial.available() > 0) {
    char c = Serial.read();
    lastCharMs = millis();
    if (c == '\n' || c == '\r') {
      if (cmdBuffer.length() == 0) continue;
      cmd = normalizeCommand(cmdBuffer);
      cmdBuffer = "";
      return true;
    }
    if (cmdBuffer.length() < 40) cmdBuffer += c;
  }
  // "Satır sonu yok" seçiliyse: 150 ms sessizlikten sonra komutu kabul et.
  // "No line ending" selected: accept the command after 150 ms of silence.
  if (cmdBuffer.length() > 0 && millis() - lastCharMs > 150) {
    cmd = normalizeCommand(cmdBuffer);
    cmdBuffer = "";
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Mesajlar ve modlar / Messages and modes
// ---------------------------------------------------------------------------
bool manualMode = false; // false = OTOMATİK, true = MANUEL / false = AUTO, true = MANUAL

void printHelp() {
  Serial.println(L("---- ARMBOT - Komutlar ----", "---- ARMBOT - Commands ----"));
  Serial.println(L("  yardim          : bu liste", "  help            : this list"));
  Serial.println(L("  oto / manuel    : otomatik / manuel mod", "  auto / manual   : auto / manual mode"));
  Serial.println(L("  taban 0-180     : taban açısı", "  base 0-180      : base angle"));
  Serial.println(L("  omuz 0-180      : omuz açısı", "  shoulder 0-180  : shoulder angle"));
  Serial.println(L("  dirsek 0-180    : dirsek açısı", "  elbow 0-180     : elbow angle"));
  Serial.println(L("  kiskac ac/kapat : kıskacı aç / kapat", "  gripper open/close : open / close the gripper"));
  Serial.println(L("  kiskac 0-180    : kıskaç açısı", "  gripper 0-180   : gripper angle"));
  Serial.println(L("  ev              : başlangıç pozu", "  home            : start pose"));
  Serial.println(L("  dur             : kolu durdur", "  stop            : stop the arm"));
  Serial.println(L("  durum           : açıları yazdır", "  status          : print the angles"));
  Serial.println(L("  dil             : English'e geç", "  lang            : switch to Turkish"));
  Serial.println(L("  Buton (B1)      : OTOMATİK <-> MANUEL", "  Button (B1)     : AUTO <-> MANUAL"));
}

void printStatus() {
  Serial.println(String(L("Mod: ", "Mode: ")) + (manualMode ? L("MANUEL", "MANUAL") : L("OTOMATİK", "AUTO")) +
                 L(" | taban ", " | base ") + current[0] + L("° omuz ", "° shoulder ") + current[1] +
                 L("° dirsek ", "° elbow ") + current[2] + L("° kıskaç ", "° gripper ") + current[3] + "°");
}

void setMode(bool manual) {
  manualMode = manual;
  armbot.buzzerPlay(manual ? 1500 : 1000, 60);
  if (manual) {
    freezeArm(); // Manuelde kol olduğu yerde bekler / in manual the arm waits where it is
    stepMs = 15;
    Serial.println(L(">> MANUEL mod: kolu seri komutlarla yönetin (yardim yazın).",
                     ">> MANUAL mode: drive the arm with serial commands (type help)."));
  } else {
    demoIndex = 0;
    startDemoStep();
    Serial.println(L(">> OTOMATİK mod: kol demoyu kendi kendine yapıyor.", ">> AUTO mode: the arm runs the demo by itself."));
  }
}

// Bir eksene seri komutla açı ver / give an axis an angle from a serial command
void moveAxisCommand(int axis, int angle) {
  if (!manualMode) setMode(true);
  target[axis] = constrain(angle, 0, 180);
  const char *namesTr[4] = {"Taban", "Omuz", "Dirsek", "Kıskaç"};
  const char *namesEn[4] = {"Base", "Shoulder", "Elbow", "Gripper"};
  Serial.println(String(L(namesTr[axis], namesEn[axis])) + L(" hedefi: ", " target: ") + target[axis] + "°");
}

void handleCommand(const String &cmd) {
  int space = cmd.indexOf(' ');
  String word = (space < 0) ? cmd : cmd.substring(0, space);
  String arg = (space < 0) ? String("") : cmd.substring(space + 1);
  arg.trim();
  bool hasValue = arg.length() > 0 && (isDigit(arg[0]) || arg[0] == '-');
  int value = arg.toInt();

  if (word == "yardim" || word == "help" || word == "?") {
    printHelp();
  } else if (word == "oto" || word == "otomatik" || word == "auto") {
    setMode(false);
  } else if (word == "manuel" || word == "manual") {
    setMode(true);
  } else if ((word == "taban" || word == "base") && hasValue) {
    moveAxisCommand(0, value);
  } else if ((word == "omuz" || word == "shoulder") && hasValue) {
    moveAxisCommand(1, value);
  } else if ((word == "dirsek" || word == "elbow") && hasValue) {
    moveAxisCommand(2, value);
  } else if (word == "kiskac" || word == "gripper") {
    if (arg == "ac" || arg == "open") moveAxisCommand(3, GRIP_OPEN);
    else if (arg == "kapat" || arg == "close") moveAxisCommand(3, GRIP_CLOSE);
    else if (hasValue) moveAxisCommand(3, value);
    else Serial.println(L("Kullanım: kiskac ac | kiskac kapat | kiskac 0-180", "Usage: gripper open | gripper close | gripper 0-180"));
  } else if (word == "ev" || word == "home") {
    if (!manualMode) setMode(true);
    setPose(HOME_POSE[0], HOME_POSE[1], HOME_POSE[2], HOME_POSE[3]);
    Serial.println(L("Başlangıç pozuna gidiliyor.", "Going to the start pose."));
  } else if (word == "dur" || word == "stop") {
    // Dur her zaman çalışır: kol olduğu yerde kalır ve manuele geçer.
    // Stop always works: the arm stays where it is and switches to manual.
    if (!manualMode) setMode(true);
    freezeArm();
    Serial.println(L("Kol durduruldu.", "Arm stopped."));
  } else if (word == "durum" || word == "status") {
    printStatus();
  } else if (word == "dil" || word == "lang" || word == "language") {
    turkish = !turkish;
    Serial.println(L("Dil: Türkçe", "Language: English"));
    printHelp();
  } else {
    Serial.println(String(L("Bilinmeyen komut: ", "Unknown command: ")) + cmd + L("  (yardim yazın)", "  (type help)"));
  }
}

// ---------------------------------------------------------------------------
bool lastButton = false;
uint32_t lastButtonMs = 0;

// Butona yeni basıldıysa true (titreşim süzgeçli) / true on a new press (debounced)
bool buttonPressed(uint32_t now) {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  bool edge = pressed && !lastButton && (now - lastButtonMs) > 200;
  if (edge) lastButtonMs = now;
  lastButton = pressed;
  return edge;
}

void setup() {
  armbot.serialStart(115200); // Seri haberleşme / Serial communication
  armbot.begin();             // Servolar başlangıç pozunda / servos start at the home pose
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(A0));

  for (int i = 0; i < 4; i++) writeServo(i, current[i]);

  Serial.println();
  Serial.println(L("ARMBOT temel örnek başladı.", "ARMBOT basic example started."));
  printHelp();
  setMode(false); // OTOMATİK modla başla / start in AUTO mode
}

void loop() {
  uint32_t now = millis();

  // 1) Buton -> mod değiştir (sadece basıldığı an) / button -> toggle mode (on press only)
  if (buttonPressed(now)) setMode(!manualMode);

  // 2) Seri komutlar / Serial commands
  String cmd;
  if (readCommand(cmd)) handleCommand(cmd);

  // 3) Otomatik demo / Automatic demo
  if (!manualMode) runAutoDemo(now);

  // 4) Servoları hedefe doğru yürüt (loop hiç bloklanmaz) / walk the servos to the target (loop never blocks)
  updateServos(now);

  // 5) Hareket ederken mavi LED yanar / blue LED on while moving
  digitalWrite(LED_PIN, atTarget() ? LOW : HIGH);
}
