/*
 * TR: EĞLENCELİ PROJE - Taş Kağıt Makas Oyunu (Otomatik demo + Manuel oyun)
 *  - Her elde ARMBOT buzzer ile "Taş, Kağıt, Makas... ÇEKTİK!" sayar ve
 *    RASTGELE bir el hareketiyle KENDİ seçimini gösterir:
 *      Taş = yumruk (kıskaç kapalı), Kağıt = açık el (kıskaç açık),
 *      Makas = kıskaç hızlıca aç-kapa yapar.
 *    Siz de aynı anda elinizle bir seçim yapın - kim kazandı, siz karar verin!
 *  - Açılışta OTOMATİK mod çalışır: robot birkaç saniyede bir kendi kendine bir
 *    el oynar (vitrin / gösteri).
 *  - MINIBOT butonu (B1 / GPIO0):
 *      kısa basış      -> bir el oyna (otomatik moddaysa MANUEL oyuna geçer)
 *      uzun basış 1 sn -> OTOMATİK <-> MANUEL
 *  - Seri port komutları (115200 baud). Türkçe veya İngilizce yazabilirsiniz:
 *      yardim / help           -> komut listesi
 *      oyna / play             -> bir el oyna
 *      tas, kagit, makas / rock, paper, scissors -> o hareketi göster
 *      oto / auto, manuel / manual -> mod seçimi
 *      taban 90 / base 90, omuz 90 / shoulder 90, dirsek 50 / elbow 50
 *      kiskac ac / gripper open, kiskac kapat / gripper close
 *      ev / home               -> başlangıç pozu
 *      dur / stop              -> eli iptal et, kolu olduğu yerde durdur
 *      dil / lang              -> dili değiştir (Türkçe <-> English)
 *
 * EN: A FUN PROJECT - Rock-Paper-Scissors Game (Auto demo + Manual game)
 *  - In every round ARMBOT counts "Rock, Paper, Scissors... SHOOT!" on the
 *    buzzer, then reveals ITS OWN choice with a RANDOM hand gesture:
 *      Rock = fist (gripper closed), Paper = open hand (gripper open),
 *      Scissors = the gripper snips open-close quickly.
 *    Play your own hand at the same time and decide who wins!
 *  - At startup AUTO mode runs: the robot plays a round by itself every few
 *    seconds (shop window / show).
 *  - MINIBOT button (B1 / GPIO0):
 *      short press     -> play a round (in auto mode it switches to the MANUAL game)
 *      long press 1 s  -> AUTO <-> MANUAL
 *  - Serial port commands (115200 baud). You can type Turkish or English:
 *      help / yardim           -> command list
 *      play / oyna             -> play a round
 *      rock, paper, scissors / tas, kagit, makas -> show that gesture
 *      auto / oto, manual / manuel -> mode select
 *      base 90 / taban 90, shoulder 90 / omuz 90, elbow 50 / dirsek 50
 *      gripper open / kiskac ac, gripper close / kiskac kapat
 *      home / ev               -> start pose
 *      stop / dur              -> cancel the round, stop the arm where it is
 *      lang / dil              -> switch language (Turkish <-> English)
 *
 * NOT / NOTE: Açılar genel bir ARMBOT içindir; kendi kolunuza göre GRIP_OPEN /
 * GRIP_CLOSE değerlerini ayarlayabilirsiniz (küçük açı = açık, diğer CODLAI
 * örnekleriyle aynı). / The angles are for a typical ARMBOT; adjust GRIP_OPEN /
 * GRIP_CLOSE for your arm (small angle = open, same as the other CODLAI examples).
 *
 * Bağlantı / Wiring: ARMBOT'un üzerindeki MINIBOT'a yükleyin (ESP8266).
 *   Taban / base GPIO5, omuz / shoulder GPIO4, dirsek / elbow GPIO12,
 *   kıskaç / gripper GPIO13, buzzer GPIO14, buton / button GPIO0, mavi LED / blue LED GPIO16.
 */

#include <ARMBOT.h> // ARMBOT kütüphanesi / ARMBOT library

ARMBOT armbot; // ARMBOT nesnesi / ARMBOT object

// Tipler en üstte olmalı (Arduino fonksiyon prototiplerini ilk fonksiyonun önüne ekler).
// Types must be at the top (Arduino puts function prototypes before the first function).
enum Choice { ROCK = 0, PAPER = 1, SCISSORS = 2 };

#define LED_PIN 16   // MINIBOT mavi LED / MINIBOT blue LED
#define BUTTON_PIN 0 // MINIBOT B1 butonu (basılıyken LOW) / MINIBOT B1 button (LOW while pressed)

// Dil seçimi: true = Türkçe, false = English. Seri porttan "dil" / "lang" ile de değişir.
// Language: true = Turkish, false = English. Can also be changed with "dil" / "lang".
bool turkish = true;
const char *L(const char *tr, const char *en) { return turkish ? tr : en; }

const int GRIP_OPEN = 20;       // Kağıt / paper (açık el / open hand)
const int GRIP_CLOSE = 120;     // Taş / rock (yumruk / fist)
const int GRIP_SCISSORS = 70;   // Makas orta konumu / scissors middle position
const int HOME_POSE[4] = {90, 90, 50, 60};
const uint32_t AUTO_ROUND_GAP_MS = 6000; // Otomatik modda eller arası bekleme / pause between rounds in auto mode
const int KEEP = -1;                     // Bu ekseni değiştirme / leave this axis as it is

// ---------------------------------------------------------------------------
// Servo hareket motoru / Servo motion engine
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

// Bloklamayan bip: buzzer'ı başlat, süre dolunca loop durdurur.
// Non-blocking beep: start the buzzer, the loop stops it when time is up.
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
// Bir oyun eli = adım listesi / One round = a list of steps
// Her adım: hedef poz (KEEP = değiştirme), hız, bekleme, bip, mesaj.
// Each step: target pose (KEEP = unchanged), speed, hold time, beep, message.
// ---------------------------------------------------------------------------
struct Step { int a[4]; int stepMs; uint16_t holdMs; uint16_t beepHz; uint16_t beepMs; const char *tr; const char *en; };
Step steps[16];
int stepCount = 0;
int stepIndex = -1;          // -1 = el oynanmıyor / no round running
uint32_t stepReachedMs = 0;  // Adımın pozuna varış zamanı / time the step's pose was reached
uint32_t lastRoundEndMs = 0;

void addStep(int b, int s, int e, int g, int ms, uint16_t hold, uint16_t hz, uint16_t bms, const char *tr, const char *en) {
  if (stepCount < 16) steps[stepCount++] = {{b, s, e, g}, ms, hold, hz, bms, tr, en};
}

void beginStep() {
  const Step &st = steps[stepIndex];
  for (int i = 0; i < 4; i++)
    if (st.a[i] != KEEP) target[i] = constrain(st.a[i], 0, 180);
  stepMs = st.stepMs;
  stepReachedMs = 0;
  if (st.beepHz) beep(st.beepHz, st.beepMs);
  if (st.tr) Serial.println(L(st.tr, st.en));
}

// Seçilen hareketin adımlarını ekle / add the steps of the chosen gesture
void addGesture(Choice pick) {
  if (pick == ROCK) {
    addStep(KEEP, 90, 80, GRIP_CLOSE, 8, 0, 0, 0, "ARMBOT seçimi: TAŞ (yumruk)", "ARMBOT picked: ROCK (fist)");
  } else if (pick == PAPER) {
    addStep(KEEP, 90, 40, GRIP_OPEN, 8, 0, 0, 0, "ARMBOT seçimi: KAĞIT (açık el)", "ARMBOT picked: PAPER (open hand)");
  } else {
    addStep(KEEP, 90, 60, GRIP_OPEN, 8, 0, 0, 0, "ARMBOT seçimi: MAKAS (kes kes)", "ARMBOT picked: SCISSORS (snip snip)");
    for (int i = 0; i < 3; i++) { // Makas gibi hızlıca aç-kapa / snip open-close quickly
      addStep(KEEP, KEEP, KEEP, GRIP_SCISSORS, 3, 150, 0, 0, nullptr, nullptr);
      addStep(KEEP, KEEP, KEEP, GRIP_OPEN, 3, 150, 0, 0, nullptr, nullptr);
    }
  }
}

// Bir el başlat: sayım + hareket + bekle + başlangıç pozu.
// Start a round: countdown + gesture + hold + start pose.
// forced < 0 ise seçim rastgele / if forced < 0 the choice is random.
void startRound(int forced) {
  stepCount = 0;
  bool countdown = forced < 0;
  if (countdown) {
    addStep(KEEP, KEEP, KEEP, KEEP, 15, 500, 400, 200, "Taş...", "Rock...");
    addStep(KEEP, KEEP, KEEP, KEEP, 15, 500, 500, 200, "Kağıt...", "Paper...");
    addStep(KEEP, KEEP, KEEP, KEEP, 15, 500, 600, 200, "Makas...", "Scissors...");
    addStep(KEEP, KEEP, KEEP, KEEP, 15, 0, 900, 300, "ÇEKTİK!", "SHOOT!");
  }
  Choice pick = countdown ? static_cast<Choice>(random(0, 3)) : static_cast<Choice>(forced);
  addGesture(pick);
  steps[stepCount - 1].holdMs = 1500; // Hareketi göster ve bekle / show the gesture and hold
  addStep(HOME_POSE[0], HOME_POSE[1], HOME_POSE[2], HOME_POSE[3], 15, 0, 0, 0, nullptr, nullptr);
  stepIndex = 0;
  beginStep();
}

void runRound(uint32_t now) {
  if (stepIndex < 0 || !atTarget()) return;
  if (stepReachedMs == 0) {
    stepReachedMs = now;
    return;
  }
  if (now - stepReachedMs < steps[stepIndex].holdMs) return;
  stepIndex++;
  if (stepIndex >= stepCount) {
    stepIndex = -1;
    lastRoundEndMs = now;
    Serial.println(L("Tekrar oynamak için butona basın (veya oyna yazın).", "Press the button to play again (or type play)."));
    return;
  }
  beginStep();
}

void cancelRound() {
  stepIndex = -1;
  freezeArm();
  armbot.buzzerStop();
  beepOn = false;
}

// ---------------------------------------------------------------------------
// Seri komut okuyucu / Serial command reader
// Seri Monitör'ün satır sonu ayarı ne olursa olsun çalışır (NL, CR, ikisi, hiçbiri).
// Works with any Serial Monitor line-ending setting (NL, CR, both, none).
// ---------------------------------------------------------------------------
String cmdBuffer;
uint32_t lastCharMs = 0;

// Küçük harfe çevirir ve Türkçe harfleri sadeleştirir: "KAĞIT" -> "kagit"
// Lower-cases and simplifies Turkish letters: "KAĞIT" -> "kagit"
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
  Serial.println(L("---- TAŞ KAĞIT MAKAS - Komutlar ----", "---- ROCK PAPER SCISSORS - Commands ----"));
  Serial.println(L("  yardim             : bu liste", "  help               : this list"));
  Serial.println(L("  oyna               : bir el oyna", "  play               : play a round"));
  Serial.println(L("  tas / kagit / makas: o hareketi göster", "  rock / paper / scissors : show that gesture"));
  Serial.println(L("  oto / manuel       : otomatik / manuel mod", "  auto / manual      : auto / manual mode"));
  Serial.println(L("  taban|omuz|dirsek 0-180 : eksen açısı", "  base|shoulder|elbow 0-180 : axis angle"));
  Serial.println(L("  kiskac ac/kapat    : kıskacı aç / kapat", "  gripper open/close : open / close the gripper"));
  Serial.println(L("  ev                 : başlangıç pozu", "  home               : start pose"));
  Serial.println(L("  dur                : eli iptal et, kolu durdur", "  stop               : cancel the round, stop the arm"));
  Serial.println(L("  dil                : English'e geç", "  lang               : switch to Turkish"));
  Serial.println(L("  Buton kısa / uzun  : bir el oyna / OTOMATİK <-> MANUEL", "  Button short / long: play a round / AUTO <-> MANUAL"));
}

void setMode(bool manual) {
  manualMode = manual;
  armbot.buzzerPlay(manual ? 1500 : 1000, 60);
  lastRoundEndMs = millis();
  Serial.println(manual ? L(">> MANUEL mod: oynamak için butona basın (veya oyna yazın).", ">> MANUAL mode: press the button to play (or type play).")
                        : L(">> OTOMATİK mod: robot birkaç saniyede bir kendi kendine oynuyor.", ">> AUTO mode: the robot plays by itself every few seconds."));
}

void moveAxisCommand(int axis, int angle) {
  if (!manualMode) setMode(true);
  if (stepIndex >= 0) cancelRound();
  target[axis] = constrain(angle, 0, 180);
  stepMs = 15;
  const char *namesTr[4] = {"Taban", "Omuz", "Dirsek", "Kıskaç"};
  const char *namesEn[4] = {"Base", "Shoulder", "Elbow", "Gripper"};
  Serial.println(String(L(namesTr[axis], namesEn[axis])) + L(" hedefi: ", " target: ") + target[axis] + "°");
}

void playCommand(int forced) {
  if (!manualMode) setMode(true);
  if (stepIndex >= 0) cancelRound();
  startRound(forced);
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
  } else if (word == "oyna" || word == "play") {
    playCommand(-1);
  } else if (word == "tas" || word == "rock") {
    playCommand(ROCK);
  } else if (word == "kagit" || word == "paper") {
    playCommand(PAPER);
  } else if (word == "makas" || word == "scissors") {
    playCommand(SCISSORS);
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
    if (stepIndex >= 0) cancelRound();
    for (int i = 0; i < 4; i++) target[i] = HOME_POSE[i];
    stepMs = 15;
    Serial.println(L("Başlangıç pozuna gidiliyor.", "Going to the start pose."));
  } else if (word == "dur" || word == "stop") {
    if (!manualMode) setMode(true);
    cancelRound();
    Serial.println(L("Durduruldu.", "Stopped."));
  } else if (word == "dil" || word == "lang" || word == "language") {
    turkish = !turkish;
    Serial.println(L("Dil: Türkçe", "Language: English"));
    printHelp();
  } else {
    Serial.println(String(L("Bilinmeyen komut: ", "Unknown command: ")) + cmd + L("  (yardim yazın)", "  (type help)"));
  }
}

// ---------------------------------------------------------------------------
// Buton: kısa basış = oyna, uzun basış (1 sn) = mod değiştir
// Button: short press = play, long press (1 s) = toggle mode
// ---------------------------------------------------------------------------
bool buttonDown = false;
bool longHandled = false;
uint32_t buttonDownMs = 0;

void handleButton(uint32_t now) {
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);
  if (pressed && !buttonDown && now - buttonDownMs > 50) { // Yeni basış / new press
    buttonDown = true;
    longHandled = false;
    buttonDownMs = now;
  } else if (pressed && buttonDown && !longHandled && now - buttonDownMs >= 1000) {
    longHandled = true; // Uzun basış: mod değiştir / long press: toggle mode
    if (stepIndex >= 0) cancelRound();
    setMode(!manualMode);
  } else if (!pressed && buttonDown) { // Bırakıldı / released
    buttonDown = false;
    buttonDownMs = now;
    if (!longHandled && stepIndex < 0) playCommand(-1); // Kısa basış: oyna / short press: play
  }
}

void setup() {
  armbot.begin();             // ARMBOT başlatılıyor / Initialize ARMBOT
  armbot.serialStart(115200); // Seri haberleşme / Serial communication
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  randomSeed(analogRead(A0));
  for (int i = 0; i < 4; i++) writeServo(i, current[i]);

  Serial.println();
  Serial.println(L("Taş Kağıt Makas hazır!", "Rock-Paper-Scissors ready!"));
  printHelp();
  setMode(false); // OTOMATİK modla başla / start in AUTO mode
}

void loop() {
  uint32_t now = millis();

  // 1) Buton / button
  handleButton(now);

  // 2) Seri komutlar / Serial commands
  String cmd;
  if (readCommand(cmd)) handleCommand(cmd);

  // 3) Otomatik modda belli aralıklarla kendi kendine oyna / in auto mode play by itself now and then
  if (!manualMode && stepIndex < 0 && now - lastRoundEndMs >= AUTO_ROUND_GAP_MS) startRound(-1);

  // 4) Eli yürüt, servoları ve buzzer'ı güncelle / run the round, update servos and buzzer
  runRound(now);
  updateServos(now);
  updateBeep(now);

  // 5) El oynanırken mavi LED yanar / blue LED on during a round
  digitalWrite(LED_PIN, stepIndex >= 0 ? HIGH : LOW);
}
