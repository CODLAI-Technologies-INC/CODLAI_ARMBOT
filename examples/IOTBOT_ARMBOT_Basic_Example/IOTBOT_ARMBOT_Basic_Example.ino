/*
 * TR: IOTBOT + ARMBOT TEMEL ÖRNEK - Otomatik demo + Manuel kontrol
 *  - Açılışta OTOMATİK mod çalışır: kol sırayla tabanı (0°-180°), omzu
 *    (45°-135°), dirseği ve kıskacı oynatır, sonra baştan başlar.
 *  - B3 butonuna basınca MANUEL moda geçer, kolu kart üzerindeki kontrollerle
 *    siz sürersiniz. B3'e tekrar basınca otomatik moda döner.
 *      Joystick sol/sağ   -> taban (eksen 1)
 *      Joystick ileri/geri-> omuz (eksen 2)
 *      Potansiyometre     -> dirsek (eksen 3)
 *      Joystick butonu    -> kıskaç aç / kapat
 *  - Servolar her zaman yavaşça (adım adım) hedefe gider; loop hiç bloklanmaz.
 *  - Seri port komutları (115200 baud). Türkçe veya İngilizce yazabilirsiniz:
 *      yardim       / help          -> komut listesi
 *      oto          / auto          -> otomatik mod
 *      manuel       / manual        -> manuel mod
 *      taban 90     / base 90       -> taban açısı 0-180
 *      omuz 90      / shoulder 90   -> omuz açısı 0-180
 *      dirsek 50    / elbow 50      -> dirsek açısı 0-180
 *      kiskac ac    / gripper open  -> kıskacı aç
 *      kiskac kapat / gripper close -> kıskacı kapat
 *      ev           / home          -> başlangıç (kalibrasyon) pozu
 *      dur          / stop          -> kolu olduğu yerde durdur
 *      dil          / lang          -> dili değiştir (Türkçe <-> English)
 *    Bir hareket komutu otomatik moddayken gelirse kol manuel moda geçer.
 *
 * EN: IOTBOT + ARMBOT BASIC EXAMPLE - Automatic demo + Manual control
 *  - At startup AUTO mode runs: the arm moves the base (0°-180°), the shoulder
 *    (45°-135°), the elbow and the gripper in turn, then starts over.
 *  - Press B3 to switch to MANUAL mode and drive the arm with the onboard
 *    controls. Press B3 again to go back to auto mode.
 *      Joystick left/right      -> base (axis 1)
 *      Joystick forward/back    -> shoulder (axis 2)
 *      Potentiometer            -> elbow (axis 3)
 *      Joystick button          -> gripper open / close
 *  - The servos always move slowly (step by step); the loop never blocks.
 *  - Serial port commands (115200 baud). You can type Turkish or English:
 *      help          / yardim       -> command list
 *      auto          / oto          -> auto mode
 *      manual        / manuel       -> manual mode
 *      base 90       / taban 90     -> base angle 0-180
 *      shoulder 90   / omuz 90      -> shoulder angle 0-180
 *      elbow 50      / dirsek 50    -> elbow angle 0-180
 *      gripper open  / kiskac ac    -> open the gripper
 *      gripper close / kiskac kapat -> close the gripper
 *      home          / ev           -> start (calibration) pose
 *      stop          / dur          -> stop the arm where it is
 *      lang          / dil          -> switch language (Turkish <-> English)
 *    A motion command received in auto mode switches the arm to manual mode.
 *
 * Bağlantı / Wiring: ARMBOT kablosunu IOTBOT'un P1-P5 soketlerine takın:
 *   taban / base IO25, omuz / shoulder IO26, dirsek / elbow IO27,
 *   kıskaç / gripper IO32, ARMBOT buzzer IO33. Bu soketlere başka modül takmayın.
 *   / Plug the ARMBOT cable into the IOTBOT's P1-P5 sockets (pins above).
 *   Do not plug other modules into these sockets.
 */

#include <IOTBOT.h> // IoTBot kütüphanesi / IoTBot library
#include <ARMBOT.h> // ARMBOT kütüphanesi / ARMBOT library

IOTBOT iotbot; // IoTBot nesnesi / IoTBot object
ARMBOT armbot; // ARMBOT nesnesi / ARMBOT object

// Dil seçimi: true = Türkçe, false = English. Seri porttan "dil" / "lang" ile de değişir.
// Language: true = Turkish, false = English. Can also be changed with "dil" / "lang".
bool turkish = true;
const char *L(const char *tr, const char *en) { return turkish ? tr : en; }

const int GRIP_OPEN = 20;   // Küçük açı = açık / small angle = open
const int GRIP_CLOSE = 120;
const int HOME_POSE[4] = {90, 90, 50, 60};
const int JOY_DEADZONE = 300;  // Joystick ölü bölgesi / joystick dead zone
const int JOY_STEP_MAX = 4;    // Joystick ile 20 ms'de en fazla derece / max degrees per 20 ms with the joystick

// ---------------------------------------------------------------------------
// Servo hareket motoru / Servo motion engine
// Her eksen "current" açısından "target" açısına her stepMs'de 1° yaklaşır.
// Each axis moves 1° from "current" toward "target" every stepMs.
// ---------------------------------------------------------------------------
// 0 taban, 1 omuz, 2 dirsek, 3 kıskaç / 0 base, 1 shoulder, 2 elbow, 3 gripper
int current[4] = {90, 90, 50, 60};
int target[4] = {90, 90, 50, 60};
int stepMs = 10;
uint32_t lastServoMs = 0;

void writeServo(int axis, int angle) {
  if (axis == 0) armbot.axis1Motion(angle, 0); // Hız 0 = doğrudan yaz / speed 0 = write directly
  else if (axis == 1) armbot.axis2Motion(angle, 0);
  else if (axis == 2) armbot.axis3Motion(angle, 0);
  else armbot.gripperMotion(angle, 0);
}

bool atTarget() {
  for (int i = 0; i < 4; i++)
    if (current[i] != target[i]) return false;
  return true;
}

void freezeArm() {
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
// Otomatik demo: taban, omuz, dirsek, kıskaç, bekleme (ms)
// Automatic demo: base, shoulder, elbow, gripper, hold time (ms)
// ---------------------------------------------------------------------------
struct DemoPose { int a[4]; uint16_t holdMs; };
const DemoPose DEMO[] = {
    {{0, 90, 50, 60}, 1000},    // Taban 0° / base 0°
    {{180, 90, 50, 60}, 1000},  // Taban 180° / base 180°
    {{90, 45, 50, 60}, 1000},   // Omuz 45° / shoulder 45°
    {{90, 135, 50, 60}, 1000},  // Omuz 135° / shoulder 135°
    {{90, 90, 30, 60}, 800},    // Dirsek / elbow
    {{90, 90, 100, 60}, 800},
    {{90, 90, 50, GRIP_OPEN}, 600},  // Kıskaç aç / gripper open
    {{90, 90, 50, GRIP_CLOSE}, 600}, // Kıskaç kapat / gripper close
    {{90, 90, 50, 60}, 1000},   // Başlangıç pozu / start pose
};
const int DEMO_LEN = sizeof(DEMO) / sizeof(DEMO[0]);
int demoIndex = 0;
uint32_t poseReachedMs = 0;

void startDemoStep() {
  for (int i = 0; i < 4; i++) target[i] = DEMO[demoIndex].a[i];
  poseReachedMs = 0;
}

void runAutoDemo(uint32_t now) {
  if (!atTarget()) return;
  if (poseReachedMs == 0) {
    poseReachedMs = now;
    return;
  }
  if (now - poseReachedMs >= DEMO[demoIndex].holdMs) {
    demoIndex = (demoIndex + 1) % DEMO_LEN;
    startDemoStep();
  }
}

// ---------------------------------------------------------------------------
// Manuel kontrol: joystick, potansiyometre, joystick butonu
// Manual control: joystick, potentiometer, joystick button
// ---------------------------------------------------------------------------
int joyXCenter = 2048, joyYCenter = 2048;
uint32_t lastJoyMs = 0;
int lastPotAngle = -1;     // Potansiyometrenin son açısı / last potentiometer angle
bool lastJoyBtn = false;

void calibrateJoystick() { // Joystick'e dokunmayın / do not touch the joystick
  long sx = 0, sy = 0;
  for (int i = 0; i < 20; i++) {
    sx += iotbot.joystickXRead();
    sy += iotbot.joystickYRead();
    delay(5);
  }
  joyXCenter = sx / 20;
  joyYCenter = sy / 20;
  if (abs(joyXCenter - 2048) > 1000) joyXCenter = 2048; // Tuhaf okuma: varsayılan / odd reading: default
  if (abs(joyYCenter - 2048) > 1000) joyYCenter = 2048;
}

// Joystick sapmasını adım boyuna çevir / turn the joystick deflection into a step size
int joyStep(int delta) {
  if (abs(delta) <= JOY_DEADZONE) return 0;
  int step = min(abs(delta) / 400 + 1, JOY_STEP_MAX);
  return delta > 0 ? step : -step;
}

void runManual(uint32_t now) {
  // Joystick: sabit hızda (50 Hz) artımlı hareket / incremental motion at a fixed rate (50 Hz)
  if (now - lastJoyMs >= 20) {
    lastJoyMs = now;
    int sx = joyStep(iotbot.joystickXRead() - joyXCenter);
    int sy = joyStep(iotbot.joystickYRead() - joyYCenter);
    // Kablolu kumandayla aynı yön: sola itince taban açısı artar, ileri itince omuz açısı artar.
    // Same directions as the wired controller: left increases the base angle, forward increases the shoulder.
    if (sx != 0) target[0] = constrain(current[0] - sx, 0, 180);
    if (sy != 0) target[1] = constrain(current[1] + sy, 0, 180);
  }

  // Potansiyometre -> dirsek, sadece gerçekten çevrilince (3° eşik)
  // Potentiometer -> elbow, only when really turned (3° threshold)
  int potAngle = map(iotbot.potentiometerRead(), 0, 4095, 0, 180);
  if (lastPotAngle < 0 || abs(potAngle - lastPotAngle) >= 3) {
    lastPotAngle = potAngle;
    target[2] = potAngle;
  }

  // Joystick butonu (basılıyken LOW) -> kıskaç aç/kapat / joystick button (LOW while pressed) -> gripper
  bool joyBtn = !iotbot.joystickButtonRead();
  if (joyBtn && !lastJoyBtn) {
    target[3] = (target[3] < (GRIP_OPEN + GRIP_CLOSE) / 2) ? GRIP_CLOSE : GRIP_OPEN;
    Serial.println(target[3] == GRIP_OPEN ? L("Kıskaç açılıyor.", "Gripper opening.") : L("Kıskaç kapanıyor.", "Gripper closing."));
  }
  lastJoyBtn = joyBtn;
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
  while (iotbot.serialAvailable() > 0) {
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
// Ekran ve mesajlar / Screen and messages
// ---------------------------------------------------------------------------
bool manualMode = false; // false = OTOMATİK, true = MANUEL / false = AUTO, true = MANUAL
uint32_t lastScreenMs = 0;
bool lastB3 = false;

// lcdWriteFixedTxt Türkçe harfleri LCD'de doğru gösterir ve satırı boşlukla doldurur.
// lcdWriteFixedTxt shows Turkish letters correctly on the LCD and pads the row with spaces.
void lcdRow(int row, const char *text) { iotbot.lcdWriteFixedTxt(0, row, text, 20); }

void printHelp() {
  iotbot.serialWrite(L("---- IOTBOT + ARMBOT - Komutlar ----", "---- IOTBOT + ARMBOT - Commands ----"));
  iotbot.serialWrite(L("  yardim          : bu liste", "  help            : this list"));
  iotbot.serialWrite(L("  oto / manuel    : otomatik / manuel mod", "  auto / manual   : auto / manual mode"));
  iotbot.serialWrite(L("  taban 0-180     : taban açısı", "  base 0-180      : base angle"));
  iotbot.serialWrite(L("  omuz 0-180      : omuz açısı", "  shoulder 0-180  : shoulder angle"));
  iotbot.serialWrite(L("  dirsek 0-180    : dirsek açısı", "  elbow 0-180     : elbow angle"));
  iotbot.serialWrite(L("  kiskac ac/kapat : kıskacı aç / kapat", "  gripper open/close : open / close the gripper"));
  iotbot.serialWrite(L("  ev              : başlangıç pozu", "  home            : start pose"));
  iotbot.serialWrite(L("  dur             : kolu durdur", "  stop            : stop the arm"));
  iotbot.serialWrite(L("  dil             : English'e geç", "  lang            : switch to Turkish"));
  iotbot.serialWrite(L("  B3 butonu       : OTOMATİK <-> MANUEL", "  B3 button       : AUTO <-> MANUAL"));
  iotbot.serialWrite(L("  Manuel: Joy X=taban, Joy Y=omuz, Pot=dirsek, Joy butonu=kıskaç",
                       "  Manual: Joy X=base, Joy Y=shoulder, Pot=elbow, Joy button=gripper"));
}

void drawStaticScreen() {
  lcdRow(0, "  IOTBOT + ARMBOT");
  lcdRow(3, manualMode ? L("Joy/Pot/JBtn B3:oto", "Joy/Pot/JBtn B3:auto") : L("B3: manuel kontrol", "B3: manual control"));
  lastScreenMs = 0; // Değerleri hemen çiz / draw the values right away
}

void setMode(bool manual) {
  manualMode = manual;
  // Kolun kendi buzzer'ı (IO33) / the arm's own buzzer (IO33)
  armbot.buzzerPlay(manual ? 1500 : 1000, 60);
  if (manual) {
    freezeArm();
    stepMs = 8;
    lastPotAngle = map(iotbot.potentiometerRead(), 0, 4095, 0, 180); // Pot hemen zıplatmasın / pot must not jump the elbow
    iotbot.serialWrite(L(">> MANUEL mod: joystick, potansiyometre ve joystick butonu ile kullanın.",
                         ">> MANUAL mode: use the joystick, potentiometer and joystick button."));
  } else {
    stepMs = 10;
    demoIndex = 0;
    startDemoStep();
    iotbot.serialWrite(L(">> OTOMATİK mod: kol demoyu kendi kendine yapıyor.", ">> AUTO mode: the arm runs the demo by itself."));
  }
  drawStaticScreen();
}

void moveAxisCommand(int axis, int angle) {
  if (!manualMode) setMode(true);
  target[axis] = constrain(angle, 0, 180);
  // Seri komutla verilen açıyı potansiyometre hemen ezmesin / the pot must not override the serial angle right away
  lastPotAngle = map(iotbot.potentiometerRead(), 0, 4095, 0, 180);
  const char *namesTr[4] = {"Taban", "Omuz", "Dirsek", "Kıskaç"};
  const char *namesEn[4] = {"Base", "Shoulder", "Elbow", "Gripper"};
  iotbot.serialWrite(String(L(namesTr[axis], namesEn[axis])) + L(" hedefi: ", " target: ") + target[axis] + "°");
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
    else iotbot.serialWrite(L("Kullanım: kiskac ac | kiskac kapat | kiskac 0-180", "Usage: gripper open | gripper close | gripper 0-180"));
  } else if (word == "ev" || word == "home") {
    if (!manualMode) setMode(true);
    for (int i = 0; i < 4; i++) target[i] = HOME_POSE[i];
    lastPotAngle = map(iotbot.potentiometerRead(), 0, 4095, 0, 180);
    iotbot.serialWrite(L("Başlangıç pozuna gidiliyor.", "Going to the start pose."));
  } else if (word == "dur" || word == "stop") {
    if (!manualMode) setMode(true);
    freezeArm();
    iotbot.serialWrite(L("Kol durduruldu.", "Arm stopped."));
  } else if (word == "dil" || word == "lang" || word == "language") {
    turkish = !turkish;
    iotbot.serialWrite(L("Dil: Türkçe", "Language: English"));
    drawStaticScreen();
    printHelp();
  } else {
    iotbot.serialWrite(String(L("Bilinmeyen komut: ", "Unknown command: ")) + cmd + L("  (yardim yazın)", "  (type help)"));
  }
}

// ---------------------------------------------------------------------------
void setup() {
  iotbot.begin();             // IoTBot başlatılıyor / Initialize IoTBot
  iotbot.serialStart(115200); // Seri haberleşme / Serial communication
  armbot.begin();             // Servolar başlangıç pozunda / servos start at the home pose
  for (int i = 0; i < 4; i++) writeServo(i, current[i]);
  calibrateJoystick();        // Açılışta joystick'e dokunmayın / do not touch the joystick at startup
  iotbot.lcdClear();
  iotbot.serialWrite(L("IOTBOT + ARMBOT temel örnek başladı.", "IOTBOT + ARMBOT basic example started."));
  printHelp();
  setMode(false); // OTOMATİK modla başla / start in AUTO mode
}

void loop() {
  uint32_t now = millis();

  // 1) B3 -> mod değiştir (sadece basıldığı an) / B3 -> toggle mode (on press only)
  bool b3 = iotbot.button3Read();
  if (b3 && !lastB3) setMode(!manualMode);
  lastB3 = b3;

  // 2) Seri komutlar / Serial commands
  String cmd;
  if (readCommand(cmd)) handleCommand(cmd);

  // 3) Hedefleri belirle / decide the targets
  if (manualMode) runManual(now);
  else runAutoDemo(now);

  // 4) Servoları hedefe doğru yürüt (loop hiç bloklanmaz) / walk the servos (loop never blocks)
  updateServos(now);

  // 5) LCD (200 ms'de bir, titremesiz) / LCD (every 200 ms, no flicker)
  if (now - lastScreenMs >= 200) {
    lastScreenMs = now;
    char line[41];
    snprintf(line, sizeof(line), L("Mod: %s", "Mode: %s"), manualMode ? L("MANUEL", "MANUAL") : L("OTOMATİK", "AUTO"));
    lcdRow(1, line);
    // T=taban O=omuz D=dirsek K=kıskaç / B=base S=shoulder E=elbow G=gripper
    snprintf(line, sizeof(line), L("T%3d O%3d D%3d K%3d", "B%3d S%3d E%3d G%3d"),
             current[0], current[1], current[2], current[3]);
    lcdRow(2, line);
  }
}
