/*
 * ARMBOT ESP-NOW Slave Control / ARMBOT ESP-NOW Slave Kontrolü
 *
 * This example receives commands from an IOTBOT (Master) via ESP-NOW to control servos/axes.
 * Bu örnek, IOTBOT'tan (Master) ESP-NOW üzerinden gelen komutları alarak servoları/eksenleri kontrol eder.
 * Master: IOTBOT_Armbot_and_Carbot_Wireless_Control.ino
 *
 * Komut / command (deviceType 1): axis1 govde / base, axis2 omuz / shoulder,
 * axis3 dirsek / elbow, gripper kiskac / gripper (hepsi 0-180 derece / all 0-180 degrees).
 *   action = 10: MAGAZA MODU - kumanda su an CARBOT'u kontrol ediyor; kol kendi
 *   gosterisini yapar, normal komut gelince hemen birakir. / STORE MODE - the
 *   controller is driving CARBOT right now; the arm runs its own show and drops
 *   it as soon as a normal command arrives.
 * Bu kart 500 ms'de bir "buradayim" sinyali (deviceType 4) yayinlar; kumanda
 * baglanti gostergesini buna gore cizer. / This board broadcasts a heartbeat
 * (deviceType 4) every 500 ms; the controller draws its link indicator from it.
 *
 * Servolar hedefe SINIRLI hizla gider: gosteri pozundan normal kontrole donerken
 * kol ani sicrama yapmaz. / Servos approach their target at a LIMITED speed, so
 * the arm does not jump when going from a show pose back to normal control.
 *
 * Seri port (115200 baud), Türkçe veya İngilizce / Serial port, Turkish or English:
 *   yardim / help   -> komut listesi / command list
 *   durum / status  -> mod ve açılar / mode and angles
 *   dil / lang      -> dili değiştir / switch language (Türkçe <-> English)
 * Kol seri porttan sürülmez; kontrol kumandadadır. / The arm is not driven from
 * the serial port; the controller is in charge.
 */

#define USE_ESPNOW
#define USE_WIFI
#define USE_SERVO
#include <MINIBOT.h>
#include <ARMBOT.h>

MINIBOT minibot;
ARMBOT armbot;

// Dil seçimi: true = Türkçe, false = English. Seri porttan "dil" / "lang" ile de değişir.
// Language: true = Turkish, false = English. Can also be changed with "dil" / "lang".
bool turkish = true;
const char *L(const char *tr, const char *en) { return turkish ? tr : en; }

static const uint8_t TYPE_ARM_CMD = 1;
static const uint8_t TYPE_ARM_HEARTBEAT = 4;
static const uint8_t ACTION_STORE_MODE = 10;
static const unsigned long STORE_TIMEOUT_MS = 2000; // Magaza komutu kesilirse gosteriyi bitir / end the show if store commands stop
static const unsigned long SERVO_TICK_MS = 15;      // Servo adim araligi / servo step interval
static const int STEP_NORMAL = 4;                   // Normal kontrolde adim basina en fazla derece / max degrees per step in normal control
static const int STEP_STORE = 1;                    // Gosteride yavas ve yumusak / slow and smooth in the show

static uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static unsigned long lastHeartbeatMs = 0;
static unsigned long lastServoTickMs = 0;

// 0 govde, 1 omuz, 2 dirsek, 3 kiskac / 0 base, 1 shoulder, 2 elbow, 3 gripper
static int current[4] = {90, 90, 50, 60};
static int target[4] = {90, 90, 50, 60};

// Magaza modu gosterisi: her satir bir poz + bekleme suresi.
// / Store-mode show: each row is a pose + hold time.
struct ShowPose { int base, shoulder, elbow, gripper; unsigned int holdMs; };
static const ShowPose SHOW[] = {
    {90, 90, 60, 60, 300},   // Hazir / ready
    {20, 90, 60, 60, 300},   // Govde taramasi / base sweep
    {160, 90, 60, 60, 300},
    {90, 90, 60, 60, 300},
    {90, 50, 120, 60, 300},  // Omuz/dirsek / shoulder/elbow
    {90, 120, 30, 60, 300},
    {90, 90, 60, 20, 300},   // Kiskac ac / gripper open
    {90, 90, 60, 120, 300},  // Kiskac kapat / gripper close
    {90, 60, 110, 60, 200},  // El salla / wave
    {120, 60, 110, 60, 150},
    {60, 60, 110, 60, 150},
    {120, 60, 110, 60, 150},
    {60, 60, 110, 60, 150},
    {90, 90, 60, 60, 1200},  // Dinlen / rest
};
static const int SHOW_LEN = sizeof(SHOW) / sizeof(SHOW[0]);
static bool storeMode = false;
static unsigned long lastStoreCmdMs = 0;
static int showIndex = 0;
static unsigned long poseReachedMs = 0;

void setShowTarget(int i)
{
  target[0] = SHOW[i].base;
  target[1] = SHOW[i].shoulder;
  target[2] = SHOW[i].elbow;
  target[3] = SHOW[i].gripper;
  poseReachedMs = 0;
}

bool atTarget()
{
  for (int i = 0; i < 4; i++)
    if (current[i] != target[i])
      return false;
  return true;
}

void writeServos()
{
  armbot.axis1Motion(current[0], 0); // Hiz 0 = dogrudan yaz / speed 0 = write directly
  armbot.axis2Motion(current[1], 0);
  armbot.axis3Motion(current[2], 0);
  armbot.gripperMotion(current[3], 0);
}

// ---------------------------------------------------------------------------
// Seri komut okuyucu (bloklamaz) / Serial command reader (non-blocking)
// Seri Monitör'ün satır sonu ayarı ne olursa olsun çalışır (NL, CR, ikisi, hiçbiri).
// Works with any Serial Monitor line-ending setting (NL, CR, both, none).
// ---------------------------------------------------------------------------
String cmdBuffer;
unsigned long lastCharMs = 0;

// Küçük harfe çevirir ve Türkçe harfleri sadeleştirir: "YARDIM" -> "yardim"
// Lower-cases and simplifies Turkish letters: "YARDIM" -> "yardim"
String normalizeCommand(String s)
{
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

bool readCommand(String &cmd)
{
  while (Serial.available() > 0)
  {
    char c = Serial.read();
    lastCharMs = millis();
    if (c == '\n' || c == '\r')
    {
      if (cmdBuffer.length() == 0)
        continue;
      cmd = normalizeCommand(cmdBuffer);
      cmdBuffer = "";
      return true;
    }
    if (cmdBuffer.length() < 40)
      cmdBuffer += c;
  }
  // "Satır sonu yok" seçiliyse: 150 ms sessizlikten sonra komutu kabul et.
  // "No line ending" selected: accept the command after 150 ms of silence.
  if (cmdBuffer.length() > 0 && millis() - lastCharMs > 150)
  {
    cmd = normalizeCommand(cmdBuffer);
    cmdBuffer = "";
    return true;
  }
  return false;
}

void printHelp()
{
  minibot.serialWrite(L("---- ARMBOT ALICI - Komutlar ----", "---- ARMBOT RECEIVER - Commands ----"));
  minibot.serialWrite(L("  yardim : bu liste", "  help   : this list"));
  minibot.serialWrite(L("  durum  : mod ve açılar", "  status : mode and angles"));
  minibot.serialWrite(L("  dil    : English'e geç", "  lang   : switch to Turkish"));
  minibot.serialWrite(L("  Kol, IOTBOT kablosuz kumandasıyla (kanal 1) yönetilir.",
                        "  The arm is driven by the IOTBOT wireless controller (channel 1)."));
}

void handleCommand(const String &cmd)
{
  if (cmd == "yardim" || cmd == "help" || cmd == "?")
  {
    printHelp();
  }
  else if (cmd == "durum" || cmd == "status")
  {
    minibot.serialWrite(String(L("Mod: ", "Mode: ")) + (storeMode ? L("mağaza", "store") : L("kontrol", "control")) +
                        L(" | taban ", " | base ") + current[0] + L(" omuz ", " shoulder ") + current[1] +
                        L(" dirsek ", " elbow ") + current[2] + L(" kıskaç ", " gripper ") + current[3]);
  }
  else if (cmd == "dil" || cmd == "lang" || cmd == "language")
  {
    turkish = !turkish;
    minibot.serialWrite(L("Dil: Türkçe", "Language: English"));
    printHelp();
  }
  else
  {
    minibot.serialWrite(String(L("Bilinmeyen komut: ", "Unknown command: ")) + cmd + L("  (yardim yazın)", "  (type help)"));
  }
}

void setup()
{
  minibot.begin();
  minibot.serialStart(115200);
  minibot.playIntro();

  // Initialize ARMBOT (Attaches servos to pins 5, 4, 12, 13)
  // Buzzer is on Pin 14
  armbot.begin();
  writeServos();

  minibot.serialWrite(L("ESP-NOW alıcısı (ARMBOT) başlatılıyor...", "Initializing ESP-NOW receiver (ARMBOT)..."));

  minibot.initESPNow();
  minibot.setWiFiChannel(1); // Master ile aynı kanalda olmalı / Must be on same channel as Master

  // Sinyal gonderebilmek icin yayin peer'i / broadcast peer to send the heartbeat
#if defined(ESP8266)
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
#elif defined(ESP32)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
#endif

  minibot.startListening();
  minibot.serialWrite(L("Komutları almaya hazır!", "Ready to receive commands!"));
  printHelp();
}

void loop()
{
  unsigned long now = millis();

  if (minibot.newData)
  {
    CodlaiESPNowMessage msg = minibot.receivedData;
    minibot.newData = false;

    if (msg.deviceType == TYPE_ARM_CMD)
    {
      if (msg.action == ACTION_STORE_MODE)
      {
        lastStoreCmdMs = now;
        if (!storeMode)
        {
          storeMode = true;
          showIndex = 0;
          setShowTarget(showIndex);
          minibot.serialWrite(L("Mağaza modu", "Store mode"));
        }
      }
      else
      {
        if (storeMode)
          minibot.serialWrite(L("Kontrol modu", "Control mode"));
        storeMode = false;
        target[0] = constrain(msg.axis1, 0, 180);
        target[1] = constrain(msg.axis2, 0, 180);
        target[2] = constrain(msg.axis3, 0, 180);
        target[3] = constrain(msg.gripper, 0, 180);

        // Actions
        if (msg.action == 1)
          armbot.buzzerPlay(1000, 50); // Horn
        else if (msg.action == 2)
          armbot.buzzerPlay(2000, 100); // Note
      }
    }
  }

  // Magaza komutu kesildiyse (kumanda kapandi) gosteriyi bitir, oldugu yerde kal
  // / If store commands stopped (controller off), end the show and stay put
  if (storeMode && (now - lastStoreCmdMs) > STORE_TIMEOUT_MS)
  {
    storeMode = false;
    for (int i = 0; i < 4; i++)
      target[i] = current[i];
  }

  // Gosteri: poza ulasinca bekle, sonra siradaki poz / show: hold at each pose, then next
  if (storeMode && atTarget())
  {
    if (poseReachedMs == 0)
      poseReachedMs = now;
    else if (now - poseReachedMs >= SHOW[showIndex].holdMs)
    {
      showIndex = (showIndex + 1) % SHOW_LEN;
      setShowTarget(showIndex);
    }
  }

  // Servolari hedefe dogru sinirli hizla yurut / walk the servos to the target at a limited speed
  if (now - lastServoTickMs >= SERVO_TICK_MS)
  {
    lastServoTickMs = now;
    int maxStep = storeMode ? STEP_STORE : STEP_NORMAL;
    bool moved = false;
    for (int i = 0; i < 4; i++)
    {
      int diff = target[i] - current[i];
      if (diff != 0)
      {
        current[i] += constrain(diff, -maxStep, maxStep);
        moved = true;
      }
    }
    if (moved)
      writeServos();
  }

  if (now - lastHeartbeatMs >= 500)
  {
    lastHeartbeatMs = now;
    CodlaiESPNowMessage hb = {};
    hb.deviceType = TYPE_ARM_HEARTBEAT;
    esp_now_send(broadcastAddress, (uint8_t *)&hb, sizeof(hb));
  }

  // Seri komutlar (yalnızca yardım/durum/dil; kontrolü etkilemez)
  // / Serial commands (help/status/lang only; they don't affect control)
  String cmd;
  if (readCommand(cmd))
    handleCommand(cmd);
}
