#include <ARMBOT.h>

// Uncomment the following line if testing on IOTBOT to enable LCD feedback (IOTBOT ekranında durum görmek için aşağıdaki satırı aktif edin)
// #define USE_IOTBOT_SCREEN

#ifdef USE_IOTBOT_SCREEN
#include <IOTBOT.h>
IOTBOT iotbot;
#endif

#define LED_PIN 16 // Minibot blue LED pin (Minibot mavi led pini)
#define B1_PIN 0   // Minibot built-in button

ARMBOT armbot;

enum Mode { IDLE_MODE, DEMO_MODE, RANDOM_MODE };
Mode currentMode = IDLE_MODE; // Varsayılan mod durgun (IDLE)
bool demoDone = false;
unsigned long afkTimer = 0; // AFK zamanlayıcısı

// Button logic variables
bool b1State = false;
unsigned long b1Timer = 0;
int b1Clicks = 0;
bool b1LongPressed = false;

// Helper function to print status to Serial and IOTBOT LCD (Seri porta ve IOTBOT LCD'ye durum yazdıran yardımcı fonksiyon)
void showStatus(String title, String detail = "")
{
  if (detail == "") {
    armbot.serialWrite(title);
  } else {
    armbot.serialWrite(title + ": " + detail);
  }

#ifdef USE_IOTBOT_SCREEN
  iotbot.lcdClear();
  iotbot.lcdWriteCR(0, 0, title);
  if (detail != "") {
    iotbot.lcdWriteCR(0, 1, detail);
  }
#endif
}

// Sesli bildirim fonksiyonu (Audible Feedback for Modes)
void playModeSound(Mode m) {
    if (m == IDLE_MODE) {
        // IDLE: Durgun mod (Tek kalın tok ses)
        armbot.buzzerPlay(200, 150);
    } else if (m == RANDOM_MODE) {
        // RANDOM: Otonom eğlenceli robotik ritim
        armbot.buzzerPlay(800, 100);
        delay(50);
        armbot.buzzerPlay(600, 100);
        delay(50);
        armbot.buzzerPlay(1000, 150);
    } else if (m == DEMO_MODE) {
        // DEMO: Demo başliyor cıngılı (3 neşeli nota)
        armbot.buzzerPlay(400, 100);
        delay(50);
        armbot.buzzerPlay(500, 100);
        delay(50);
        armbot.buzzerPlay(650, 200);
    }
}

// --- RTOS BUTTON WATCHER (ESP32 Ozel) ---
// Motor hareketleri (axis1Motion vb.) ARMBOT kütüphanesinde bekleme(delay/loop) içerdiği için
// hareket sırasında butonu okuyabilmek adına arka planda RTOS süreci başlatıyoruz.
#if defined(ESP32)
TaskHandle_t btnTaskHandle;
void buttonWatcherTask(void * parameter) {
    for(;;) {
        // Minibot uzerindeki donanimsal butonu I2C çakışması olmadan sürekli dinler
        if (digitalRead(B1_PIN) == LOW) {
            // Herhangi bir butona basilma aninda kilitlenen motor sweep islemini keser.
            // Kilit acilinca ana dongudeki checkButtons hizlica isler ve dogru modu secer.
            armbot.abortMotion(); 
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
#endif

bool isB1Pressed() {
#ifdef USE_IOTBOT_SCREEN
  return iotbot.button1Read();
#else
  return (digitalRead(B1_PIN) == LOW);
#endif
}

bool isB2Pressed() {
#ifdef USE_IOTBOT_SCREEN
  return iotbot.button2Read();
#else
  return false;
#endif
}

// Checks buttons dynamically. Returns true if a mode switch occurred so callers can abort their sequence.
bool checkButtons() {
    bool b1Pressed = isB1Pressed();
    bool modeChanged = false;
    
    // Check for double click timeout (wait 400ms for second click)
    if (!b1Pressed && b1Clicks > 0 && (millis() - b1Timer > 400)) {
        if (b1Clicks == 2) {
            // Double click action: Toggle RANDOM_MODE
            if (currentMode == RANDOM_MODE) {
                currentMode = IDLE_MODE;
                showStatus("Mod Degisti", "IDLE (Durgun)");
                playModeSound(IDLE_MODE);
                armbot.axis1Motion(90, 20); armbot.axis2Motion(90, 20);
                armbot.axis3Motion(50, 20); armbot.gripperMotion(60, 20);
                afkTimer = millis(); 
                modeChanged = true;
            } else if (currentMode == IDLE_MODE) {
                currentMode = RANDOM_MODE;
                showStatus("Mod Degisti", "RANDOM MODE");
                playModeSound(RANDOM_MODE);
                afkTimer = millis(); 
                modeChanged = true;
            }
        }
        b1Clicks = 0; 
    }

    if (b1Pressed && !b1State) {
        // Button just pressed
        b1State = true;
        b1Timer = millis();
        b1LongPressed = false;
        afkTimer = millis(); 
    } else if (b1Pressed && b1State) {
        // Button held
        if (!b1LongPressed && (millis() - b1Timer > 1000)) {
            b1LongPressed = true;
            // Hold action: Toggle DEMO_MODE
            if (currentMode == DEMO_MODE) {
               currentMode = IDLE_MODE;
               showStatus("Mod Degisti", "IDLE (Durgun)");
               playModeSound(IDLE_MODE);
               armbot.axis1Motion(90, 20); armbot.axis2Motion(90, 20);
               armbot.axis3Motion(50, 20); armbot.gripperMotion(60, 20);
               modeChanged = true;
            } else if (currentMode == IDLE_MODE) {
               currentMode = DEMO_MODE;
               demoDone = false;
               showStatus("Mod Degisti", "DEMO MODE");
               playModeSound(DEMO_MODE);
               modeChanged = true;
            }
            // If in RANDOM_MODE, we ignore the hold to prevent activating demo by mistake.
            
            b1Clicks = 0; 
        }
    } else if (!b1Pressed && b1State) {
        // Button just released
        b1State = false;
        if (!b1LongPressed) {
            b1Clicks++;
            b1Timer = millis(); 
        }
    }

    // For IOTBOT users: B2 functions as a panic button returning to IDLE
    bool b2Pressed = isB2Pressed();
    if (b2Pressed && currentMode != IDLE_MODE) {
        currentMode = IDLE_MODE;
        showStatus("Mod Degisti", "IDLE (Durgun)");
        playModeSound(IDLE_MODE);
        armbot.axis1Motion(90, 20); armbot.axis2Motion(90, 20);
        armbot.axis3Motion(50, 20); armbot.gripperMotion(60, 20);
        afkTimer = millis();
        modeChanged = true;
    }

    return modeChanged;
}

// Macro to replace delay() with a non-blocking delay that periodically checks for button presses
#define SMART_DELAY(ms) \
  do { \
    unsigned long s = millis(); \
    while(millis() - s < ms) { \
      if (checkButtons()) return; \
      delay(50); \
    } \
  } while(0)

// ---- DEMO MODE SEQUENCE ----
void runFullArmbotDemo()
{
  // Axis 1 movements
  showStatus("Hareket: Eksen 1");
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(100, 200); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;

  showStatus("Eksen 1", "0 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis1Motion(0, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Eksen 1", "180 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis1Motion(180, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Eksen 1", "90 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis1Motion(90, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;

  // Axis 2 movements
  showStatus("Hareket: Eksen 2");
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(100, 50); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(200, 200); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;

  showStatus("Eksen 2", "0 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis2Motion(0, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Eksen 2", "180 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis2Motion(180, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Eksen 2", "90 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis2Motion(90, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;

  // Axis 3 movements
  showStatus("Hareket: Eksen 3");
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(100, 50); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(150, 50); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(200, 200); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;

  showStatus("Eksen 3", "20 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis3Motion(20, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Eksen 3", "180 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis3Motion(180, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Eksen 3", "50 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.axis3Motion(50, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;

  // Gripper
  showStatus("Hareket: Gripper");
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(100, 50); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(150, 50); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(200, 50); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;
  digitalWrite(LED_PIN, HIGH);
  armbot.buzzerPlay(225, 200); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(100); if(currentMode != DEMO_MODE) return;

  showStatus("Gripper", "0 Derece (Acik)");
  digitalWrite(LED_PIN, HIGH);
  armbot.gripperMotion(0, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Gripper", "110 Derece (Kapali)");
  digitalWrite(LED_PIN, HIGH);
  armbot.gripperMotion(110, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
  
  showStatus("Gripper", "60 Derece");
  digitalWrite(LED_PIN, HIGH);
  armbot.gripperMotion(60, 20); 
  digitalWrite(LED_PIN, LOW);
  SMART_DELAY(1000); if(currentMode != DEMO_MODE) return;
}

// ---- RANDOM (ROBOTIC PET / FACTORY WANDERER) MODE ----
void runRandomMode() 
{
  // Pick a random axis (0: Axis1, 1: Axis2, 2: Axis3, 3: Gripper)
  int selectedAxis = random(0, 4);
  int rndAngle = 0;
  
  digitalWrite(LED_PIN, HIGH);
  
  if (selectedAxis == 0) {
      rndAngle = random(20, 160);
      showStatus("Rastgele (Random)", "Eksen 1 -> " + String(rndAngle));
      armbot.axis1Motion(rndAngle, 30);
  } else if (selectedAxis == 1) {
      rndAngle = random(20, 160);
      showStatus("Rastgele (Random)", "Eksen 2 -> " + String(rndAngle));
      armbot.axis2Motion(rndAngle, 30);
  } else if (selectedAxis == 2) {
      rndAngle = random(40, 140); // Eksen 3'ün yere vurmaması için limitli
      showStatus("Rastgele (Random)", "Eksen 3 -> " + String(rndAngle));
      armbot.axis3Motion(rndAngle, 30);
  } else {
      rndAngle = random(0, 110);
      showStatus("Rastgele (Random)", "Gripper -> " + String(rndAngle));
      armbot.gripperMotion(rndAngle, 30);
      armbot.buzzerPlay(random(600, 1200), 50); // Gripper hareketleri robotik küçük sesler yapar
  }
  
  digitalWrite(LED_PIN, LOW);

  // Kısa veya uzun rastgele bekleme (Organik görünüm)
  int waitTime = random(300, 1500); 
  SMART_DELAY(waitTime);
}

// ---- IDLE MODE ----
void runIdleMode() 
{
  // AFK (Durgunluk) Kontrolü - 15 saniyeden uzun süredir butona basılmadıysa
  if (millis() - afkTimer > 15000) {
    showStatus("Durum", "AFK! Buradayim.");
    
    // Flaşör
    digitalWrite(LED_PIN, HIGH);
    SMART_DELAY(75); if(currentMode != IDLE_MODE) return;
    digitalWrite(LED_PIN, LOW);
    SMART_DELAY(75); if(currentMode != IDLE_MODE) return;
    digitalWrite(LED_PIN, HIGH);
    SMART_DELAY(75); if(currentMode != IDLE_MODE) return;
    digitalWrite(LED_PIN, LOW);
    
    // Sevimli nefes alma hareketi (Eksen 3 hafif inip kalkar)
    armbot.axis3Motion(60, 10);
    SMART_DELAY(500); if(currentMode != IDLE_MODE) return;
    armbot.axis3Motion(50, 10);
    
    // Küçük AFK ping sesi
    armbot.buzzerPlay(1500, 50);
    
    afkTimer = millis(); 
  }

  // Butonları hızlı dinleyebilmek için
  SMART_DELAY(250);
}

void setup()
{
  armbot.serialStart(115200);
  armbot.begin();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(B1_PIN, INPUT_PULLUP);

#ifdef USE_IOTBOT_SCREEN
  iotbot.begin();
#endif
  
  // Basit rastgelelik tohumlaması (Random Seed)
  randomSeed(analogRead(14));

  // Başlangıç noktasında duruş ayarı
  armbot.axis1Motion(90, 20);
  armbot.axis2Motion(90, 20);
  armbot.axis3Motion(50, 20);
  armbot.gripperMotion(60, 20);

  showStatus("ARMBOT Ready", "IDLE MODE (Durgun)");
  // Açılış sesi
  playModeSound(IDLE_MODE); 
  afkTimer = millis(); 

  // RTOS Arka Plan Sürecini Başlat
#if defined(ESP32)
  xTaskCreate(
      buttonWatcherTask, 
      "BtnWatcher", 
      2048, 
      NULL, 
      1, 
      &btnTaskHandle
  );
#endif
}

void loop()
{
  // Buton dinleyicisi her dongude kontrol edilir 
  if (checkButtons()) return;

  if (currentMode == DEMO_MODE) {
    if (!demoDone) {
      runFullArmbotDemo();
      demoDone = true;
      // Demo bittikten sonra kendi kendine tekrar durgun moda geçer
      if (currentMode == DEMO_MODE) {
        currentMode = IDLE_MODE;
        afkTimer = millis(); 
        showStatus("Demo Bitti", "IDLE (Durgun)");
        playModeSound(IDLE_MODE);
        armbot.axis1Motion(90, 20); armbot.axis2Motion(90, 20);
        armbot.axis3Motion(50, 20); armbot.gripperMotion(60, 20);
      }
    }
  } 
  else if (currentMode == RANDOM_MODE) {
    runRandomMode();
  }
  else if (currentMode == IDLE_MODE) {
    runIdleMode();
  }
}
