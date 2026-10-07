/*
 * TR: ARMBOT EL SALLAMA ÖRNEĞİ - Otomatik demo + Manuel kontrol
 *  - Açılışta OTOMATİK mod çalışır: kol dirseğini kaldırır, tabanı sağa-sola
 *    çevirerek el sallar (önce 3, sonra 5 kez) ve arada dinlenir.
 *  - MINIBOT üzerindeki butona (B1 / GPIO0) basınca MANUEL moda geçer: kol
 *    olduğu yerde durur, seri komutlarla siz yönetirsiniz. Butona tekrar basınca
 *    otomatik moda döner.
 *  - Kütüphanedeki hazır armbot.waveHand(adet) fonksiyonunu denemek için seri
 *    porta "salla 3" yazın. DİKKAT: waveHand() hareket bitene kadar programı
 *    bekletir (her sallama ~0,3 sn); otomatik mod bu yüzden aynı hareketi
 *    adım adım (bloklamadan) yapar.
 *  - Seri port komutları (115200 baud). Türkçe veya İngilizce yazabilirsiniz:
 *      yardim        / help           -> komut listesi
 *      oto           / auto           -> otomatik mod
 *      manuel        / manual         -> manuel mod
 *      salla 3       / wave 3         -> armbot.waveHand(3) ile el salla (1-10)
 *      taban 90      / base 90        -> taban açısı 0-180
 *      omuz 90       / shoulder 90    -> omuz açısı 0-180
 *      dirsek 50     / elbow 50       -> dirsek açısı 0-180
 *      kiskac ac     / gripper open   -> kıskacı aç (kapat / close: kapat)
 *      ev            / home           -> başlangıç pozu
 *      dur           / stop           -> kolu olduğu yerde durdur
 *      dil           / lang           -> dili değiştir (Türkçe <-> English)
 *
 * EN: ARMBOT WAVE HAND EXAMPLE - Automatic demo + Manual control
 *  - At startup AUTO mode runs: the arm raises its elbow and waves by turning
 *    the base left and right (first 3, then 5 times), resting in between.
 *  - Press the button on the MINIBOT (B1 / GPIO0) to switch to MANUAL mode: the
 *    arm stops where it is and you drive it with serial commands. Press the
 *    button again to go back to auto mode.
 *  - To try the library's ready-made armbot.waveHand(count) function, type
 *    "wave 3". NOTE: waveHand() makes the program wait until the motion is
 *    done (~0.3 s per wave); that is why auto mode does the same motion step by
 *    step (without blocking).
 *  - Serial port commands (115200 baud). You can type Turkish or English:
 *      help          / yardim         -> command list
 *      auto          / oto            -> auto mode
 *      manual        / manuel         -> manual mode
 *      wave 3        / salla 3        -> wave with armbot.waveHand(3) (1-10)
 *      base 90       / taban 90       -> base angle 0-180
 *      shoulder 90   / omuz 90        -> shoulder angle 0-180
 *      elbow 50      / dirsek 50      -> elbow angle 0-180
 *      gripper open  / kiskac ac      -> open the gripper (close / kapat: close)
 *      home          / ev             -> start pose
 *      stop          / dur            -> stop the arm where it is
 *      lang          / dil            -> switch language (Turkish <-> English)
 *
 * Bağlantı / Wiring: ARMBOT'un üzerindeki MINIBOT'a yükleyin (ESP8266).
 *   Taban / base GPIO5, omuz / shoulder GPIO4, dirsek / elbow GPIO12,
 *   kıskaç / gripper GPIO13, buzzer GPIO14, buton / button GPIO0, mavi LED / blue LED GPIO16.
 */

#include <ARMBOT.h> // ARMBOT kütüphanesi / ARMBOT library

ARMBOT armbot; // ARMBOT nesnesi / ARMBOT object

#define LED_PIN 16   // MINIBOT mavi LED / MINIBOT blue LED
#define BUTTON_PIN 0 // MINIBOT B1 butonu (basılıyken LOW) / MINIBOT B1 button (LOW while pressed)

// Dil seçimi: true = Türkçe, false = English. Seri porttan "dil" / "lang" ile de değişir.
// Language: true = Turkish, false = English. Can also be changed with "dil" / "lang".
bool turkish = true;
const char *L(const char *tr, const char *en) { return turkish ? tr : en; }

const int GRIP_OPEN = 20;   // Küçük açı = açık / small angle = open
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
int stepMs = 15;
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
// Otomatik el sallama (bloklamayan) / Automatic waving (non-blocking)
// Kütüphanedeki waveHand() ile aynı hareket: dirsek 120°'ye kalkar, taban
// ±40° sallanır, sonra her şey eski yerine döner.
// Same motion as the library's waveHand(): elbow up to 120°, base swings
// ±40°, then everything returns.
// ---------------------------------------------------------------------------
struct WavePose { int base; int elbow; int stepMs; uint16_t holdMs; };
WavePose waveSeq[16];
int waveLen = 0;
int waveIndex = 0;
uint32_t poseReachedMs = 0;
bool fiveWaves = false; // Sırayla 3 ve 5 kez salla / wave 3 and 5 times in turn

void addWavePose(int base, int elbow, int ms, uint16_t hold) {
  if (waveLen < 16) waveSeq[waveLen++] = {constrain(base, 0, 180), elbow, ms, hold};
}

void startWave(int count) {
  waveLen = 0;
  addWavePose(HOME_POSE[0], 120, 4, 0);           // Kolu kaldır / raise the arm
  for (int i = 0; i < count; i++) {
    addWavePose(HOME_POSE[0] + 40, 120, 3, 0);    // Sağa / right
    addWavePose(HOME_POSE[0] - 40, 120, 3, 0);    // Sola / left
  }
  addWavePose(HOME_POSE[0], 120, 3, 0);           // Ortala / center
  addWavePose(HOME_POSE[0], HOME_POSE[2], 4, fiveWaves ? 5000 : 1000); // İndir ve dinlen / lower and rest
  waveIndex = 0;
  poseReachedMs = 0;
  target[0] = waveSeq[0].base;
  target[2] = waveSeq[0].elbow;
  stepMs = waveSeq[0].stepMs;
  Serial.println(String(L("El sallıyor (", "Waving hand (")) + count + L(" kez)...", " times)..."));
}

void runAutoWave(uint32_t now) {
  if (!atTarget()) return;
  if (poseReachedMs == 0) {
    poseReachedMs = now;
    return;
  }
  if (now - poseReachedMs < waveSeq[waveIndex].holdMs) return;
  waveIndex++;
  if (waveIndex >= waveLen) {
    fiveWaves = !fiveWaves;
    startWave(fiveWaves ? 5 : 3);
    return;
  }
  poseReachedMs = 0;
  target[0] = waveSeq[waveIndex].base;
  target[2] = waveSeq[waveIndex].elbow;
  stepMs = waveSeq[waveIndex].stepMs;
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
  Serial.println(L("---- ARMBOT EL SALLAMA - Komutlar ----", "---- ARMBOT WAVE HAND - Commands ----"));
  Serial.println(L("  yardim          : bu liste", "  help            : this list"));
  Serial.println(L("  oto / manuel    : otomatik / manuel mod", "  auto / manual   : auto / manual mode"));
  Serial.println(L("  salla 1-10      : waveHand() ile el salla", "  wave 1-10       : wave with waveHand()"));
  Serial.println(L("  taban 0-180     : taban açısı", "  base 0-180      : base angle"));
  Serial.println(L("  omuz 0-180      : omuz açısı", "  shoulder 0-180  : shoulder angle"));
  Serial.println(L("  dirsek 0-180    : dirsek açısı", "  elbow 0-180     : elbow angle"));
  Serial.println(L("  kiskac ac/kapat : kıskacı aç / kapat", "  gripper open/close : open / close the gripper"));
  Serial.println(L("  ev              : başlangıç pozu", "  home            : start pose"));
  Serial.println(L("  dur             : kolu durdur", "  stop            : stop the arm"));
  Serial.println(L("  dil             : English'e geç", "  lang            : switch to Turkish"));
  Serial.println(L("  Buton (B1)      : OTOMATİK <-> MANUEL", "  Button (B1)     : AUTO <-> MANUAL"));
}

void setMode(bool manual) {
  manualMode = manual;
  armbot.buzzerPlay(manual ? 1500 : 1000, 60);
  if (manual) {
    freezeArm();
    stepMs = 15;
    Serial.println(L(">> MANUEL mod: kolu seri komutlarla yönetin (yardim yazın).",
                     ">> MANUAL mode: drive the arm with serial commands (type help)."));
  } else {
    Serial.println(L(">> OTOMATİK mod: kol kendi kendine el sallıyor.", ">> AUTO mode: the arm waves by itself."));
    target[1] = HOME_POSE[1]; // Omuz ve kıskaç başlangıç konumuna / shoulder and gripper to the start pose
    target[3] = HOME_POSE[3];
    fiveWaves = false;
    startWave(3);
  }
}

void moveAxisCommand(int axis, int angle) {
  if (!manualMode) setMode(true);
  target[axis] = constrain(angle, 0, 180);
  stepMs = 15;
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
  } else if (word == "salla" || word == "wave") {
    if (!manualMode) setMode(true);
    int count = hasValue ? constrain(value, 1, 10) : 3;
    // Kol olduğu yerden başlar; waveHand() bitince yine buraya döner.
    // The arm starts where it is; waveHand() returns it here when done.
    freezeArm();
    Serial.println(String(L("armbot.waveHand(", "armbot.waveHand(")) + count + L(") çalışıyor...", ") running..."));
    digitalWrite(LED_PIN, HIGH);
    armbot.waveHand(count); // Kütüphane fonksiyonu: bitene kadar bekler / library function: waits until done
    Serial.println(L("El sallama bitti.", "Waving done."));
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
    for (int i = 0; i < 4; i++) target[i] = HOME_POSE[i];
    Serial.println(L("Başlangıç pozuna gidiliyor.", "Going to the start pose."));
  } else if (word == "dur" || word == "stop") {
    if (!manualMode) setMode(true);
    freezeArm();
    Serial.println(L("Kol durduruldu.", "Arm stopped."));
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
  armbot.begin();             // ARMBOT başlatılıyor / Initialize ARMBOT
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < 4; i++) writeServo(i, current[i]);

  Serial.println();
  Serial.println(L("ARMBOT hazır. El sallamaya hazırlanın!", "ARMBOT initialized. Get ready to wave!"));
  printHelp();
  setMode(false); // OTOMATİK modla başla / start in AUTO mode
}

void loop() {
  uint32_t now = millis();

  // 1) Buton -> mod değiştir / button -> toggle mode
  if (buttonPressed(now)) setMode(!manualMode);

  // 2) Seri komutlar / Serial commands
  String cmd;
  if (readCommand(cmd)) handleCommand(cmd);

  // 3) Otomatik el sallama / Automatic waving
  if (!manualMode) runAutoWave(now);

  // 4) Servoları hedefe doğru yürüt / walk the servos to the target
  updateServos(now);

  // 5) Hareket ederken mavi LED yanar / blue LED on while moving
  digitalWrite(LED_PIN, atTarget() ? LOW : HIGH);
}
