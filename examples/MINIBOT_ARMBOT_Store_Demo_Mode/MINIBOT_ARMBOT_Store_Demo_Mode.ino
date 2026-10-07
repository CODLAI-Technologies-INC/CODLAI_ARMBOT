/*
 * TR: ARMBOT MAĞAZA (GÖSTERİ) MODU - kumanda gerekmez
 *  - Açılışta OTOMATİK mod çalışır: kol sürekli bir gösteri yapar (taban
 *    taraması, omuz/dirsek, kıskaç aç/kapa, el sallama) ve her bölümde kısa bir
 *    bip sesi çıkarır. Gösteri adım adım (bloklamadan) çalışır.
 *  - MINIBOT üzerindeki butona (B1 / GPIO0) basınca MANUEL moda geçer: kol
 *    olduğu yerde durur, seri komutlarla siz yönetirsiniz. Butona tekrar basınca
 *    gösteri kaldığı bölümden devam eder.
 *  - Seri port komutları (115200 baud). Türkçe veya İngilizce yazabilirsiniz:
 *      yardim        / help           -> komut listesi
 *      oto           / auto           -> otomatik mod (gösteri)
 *      manuel        / manual         -> manuel mod (seri komutlar)
 *      taban 90      / base 90        -> tabanı 90°'ye çevir
 *      omuz 90       / shoulder 90    -> omzu 90°'ye götür
 *      dirsek 50     / elbow 50       -> dirseği 50°'ye götür
 *      kiskac ac     / gripper open   -> kıskacı aç
 *      kiskac kapat  / gripper close  -> kıskacı kapat
 *      kiskac 60     / gripper 60     -> kıskacı 60°'ye götür
 *      ev            / home           -> başlangıç pozuna dön
 *      dur           / stop           -> kolu hemen durdur
 *      dil           / lang           -> dili değiştir (Türkçe <-> English)
 *  - PlatformIO ortamı / environment: MINIBOT_ARMBOT_DEMO (src/kontrol/minibot_armbot_demo.cpp
 *    bu dosyayı içeri alır / includes this file).
 *
 * EN: ARMBOT STORE (SHOW) MODE - no controller needed
 *  - At startup AUTO mode runs: the arm loops a show (base sweep, shoulder/elbow,
 *    gripper open/close, waving) with a short beep for each part. The show runs
 *    step by step (without blocking).
 *  - Press the button on the MINIBOT (B1 / GPIO0) to switch to MANUAL mode: the
 *    arm stops where it is and you drive it with serial commands. Press the
 *    button again and the show continues from the part it was in.
 *  - Serial port commands (115200 baud). You can type Turkish or English:
 *      help           / yardim        -> command list
 *      auto           / oto           -> auto mode (show)
 *      manual         / manuel        -> manual mode (serial commands)
 *      base 90        / taban 90      -> turn the base to 90°
 *      shoulder 90    / omuz 90       -> move the shoulder to 90°
 *      elbow 50       / dirsek 50     -> move the elbow to 50°
 *      gripper open   / kiskac ac     -> open the gripper
 *      gripper close  / kiskac kapat  -> close the gripper
 *      gripper 60     / kiskac 60     -> move the gripper to 60°
 *      home           / ev            -> back to the start pose
 *      stop           / dur           -> stop the arm now
 *      lang           / dil           -> switch language (Turkish <-> English)
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
const int HOME_POSE[4] = {90, 90, 50, 60};
const int KEEP = -1;        // Bu ekseni değiştirme / leave this axis as it is

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

// Bloklamayan bip / non-blocking beep
uint32_t beepUntilMs = 0;
bool beepOn = false;
void beep(int hz, int ms) {
  armbot.buzzerStart(hz);
  beepOn = true;
  beepUntilMs = millis() + ms;
}
void updateBeep(uint32_t now) {
  if (beepOn && (int32_t)(now - beepUntilMs) >= 0) {
    armbot.buzzerStop();
    beepOn = false;
  }
}

// ---------------------------------------------------------------------------
// Gösteri: her satır bir adım / Show: each row is one step
// taban, omuz, dirsek, kıskaç (KEEP = değiştirme), hız (ms/derece), bekleme, bip Hz, mesaj
// base, shoulder, elbow, gripper (KEEP = unchanged), speed (ms/degree), hold, beep Hz, message
// ---------------------------------------------------------------------------
struct ShowStep { int a[4]; uint8_t stepMs; uint16_t holdMs; uint16_t beepHz; const char *tr; const char *en; };
const ShowStep SHOW[] = {
    {{0, KEEP, KEEP, KEEP}, 10, 250, 600, "Bölüm: taban taraması", "Part: base sweep"},
    {{180, KEEP, KEEP, KEEP}, 10, 500, 0, nullptr, nullptr},
    {{90, KEEP, KEEP, KEEP}, 10, 250, 0, nullptr, nullptr},
    {{KEEP, 40, KEEP, KEEP}, 12, 120, 700, "Bölüm: omuz / dirsek", "Part: shoulder / elbow"},
    {{KEEP, KEEP, 120, KEEP}, 12, 420, 0, nullptr, nullptr},
    {{KEEP, 120, KEEP, KEEP}, 12, 120, 0, nullptr, nullptr},
    {{KEEP, KEEP, 30, KEEP}, 12, 420, 0, nullptr, nullptr},
    {{KEEP, 90, 60, KEEP}, 10, 320, 0, nullptr, nullptr},
    {{KEEP, KEEP, KEEP, GRIP_OPEN}, 8, 320, 900, "Bölüm: kıskaç aç / kapat", "Part: gripper open / close"},
    {{KEEP, KEEP, KEEP, GRIP_CLOSE}, 8, 320, 1000, nullptr, nullptr},
    {{KEEP, KEEP, 120, KEEP}, 3, 0, 750, "Bölüm: el sallama", "Part: wave hand"},
    {{130, KEEP, KEEP, KEEP}, 3, 0, 0, nullptr, nullptr},
    {{50, KEEP, KEEP, KEEP}, 3, 0, 0, nullptr, nullptr},
    {{130, KEEP, KEEP, KEEP}, 3, 0, 0, nullptr, nullptr},
    {{50, KEEP, KEEP, KEEP}, 3, 0, 0, nullptr, nullptr},
    {{130, KEEP, KEEP, KEEP}, 3, 0, 0, nullptr, nullptr},
    {{50, KEEP, KEEP, KEEP}, 3, 0, 0, nullptr, nullptr},
    {{90, KEEP, 60, KEEP}, 3, 300, 1000, nullptr, nullptr},
    {{KEEP, KEEP, KEEP, KEEP}, 10, 1000, 1400, "Gösteri baştan başlıyor.", "The show starts over."},
};
const int SHOW_LEN = sizeof(SHOW) / sizeof(SHOW[0]);
int showIndex = 0;
uint32_t poseReachedMs = 0;

void beginShowStep() {
  const ShowStep &st = SHOW[showIndex];
  for (int i = 0; i < 4; i++)
    if (st.a[i] != KEEP) target[i] = st.a[i];
  stepMs = st.stepMs;
  poseReachedMs = 0;
  if (st.beepHz) beep(st.beepHz, 70);
  if (st.tr) Serial.println(L(st.tr, st.en));
}

void runShow(uint32_t now) {
  if (!atTarget()) return;
  if (poseReachedMs == 0) {
    poseReachedMs = now;
    return;
  }
  if (now - poseReachedMs >= SHOW[showIndex].holdMs) {
    showIndex = (showIndex + 1) % SHOW_LEN;
    beginShowStep();
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
bool manualMode = false; // false = OTOMATİK (gösteri), true = MANUEL / false = AUTO (show), true = MANUAL

void printHelp() {
  Serial.println(L("---- ARMBOT MAĞAZA MODU - Komutlar ----", "---- ARMBOT STORE MODE - Commands ----"));
  Serial.println(L("  yardim          : bu liste", "  help            : this list"));
  Serial.println(L("  oto / manuel    : gösteri / manuel mod", "  auto / manual   : show / manual mode"));
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
  armbot.buzzerStop();
  beepOn = false;
  armbot.buzzerPlay(manual ? 1500 : 1000, 60);
  if (manual) {
    freezeArm();
    stepMs = 15;
    Serial.println(L(">> MANUEL mod: kolu seri komutlarla yönetin (yardim yazın).",
                     ">> MANUAL mode: drive the arm with serial commands (type help)."));
  } else {
    Serial.println(L(">> OTOMATİK mod: mağaza gösterisi çalışıyor.", ">> AUTO mode: the store show is running."));
    beginShowStep(); // Kaldığı adımdan devam / continue from the current step
  }
}

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
  armbot.begin();             // ARMBOT başlatılıyor / Initialize ARMBOT
  armbot.serialStart(115200); // Seri haberleşme / Serial communication
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int i = 0; i < 4; i++) writeServo(i, current[i]);

  Serial.println();
  Serial.println(L("ARMBOT mağaza modu hazır.", "ARMBOT store mode ready."));
  printHelp();
  armbot.buzzerPlay(800, 120); // Kısa açılış sesi / short intro sound
  setMode(false);              // Gösteriyle başla / start with the show
}

void loop() {
  uint32_t now = millis();

  // 1) Buton -> mod değiştir / button -> toggle mode
  if (buttonPressed(now)) setMode(!manualMode);

  // 2) Seri komutlar / Serial commands
  String cmd;
  if (readCommand(cmd)) handleCommand(cmd);

  // 3) Gösteri / show
  if (!manualMode) runShow(now);

  // 4) Servolar ve buzzer / servos and buzzer
  updateServos(now);
  updateBeep(now);

  // 5) Hareket ederken mavi LED yanar / blue LED on while moving
  digitalWrite(LED_PIN, atTarget() ? LOW : HIGH);
}
