/*
 * IOTBOT Armbot and Carbot Wireless Control (ESP-NOW Master)
 * 
 * This example allows IOTBOT to control ARMBOT and CARBOT wirelessly using ESP-NOW.
 * IOTBOT acts as the Master controller.
 * 
 * Instructions:
 * 1. Upload this code to IOTBOT.
 * 2. Upload "ARMBOT_ESP_NOW_Slave_Control" to ARMBOT.
 * 3. Upload "CARBOT_ESP_NOW_Slave_Control" to CARBOT.
 * 
 * Note: This example uses Broadcast (FF:FF:FF:FF:FF:FF) for simplicity.
 * All slaves will receive the data, but they should filter by "deviceType".
 */

#define USE_ESPNOW
#include <IOTBOT.h>
#include <esp_wifi.h> // For channel setting

IOTBOT iotbot;

// Broadcast Address / Yayın Adresi
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Data Structures / Veri Yapıları
// Must match the receiver's structure / Alıcının yapısıyla eşleşmelidir

// Type 1: Armbot Data
typedef struct struct_arm_message {
  uint8_t deviceType; // 1 = Armbot
  int axis1;
  int axis2;
  int axis3;
  int gripper;
  uint8_t action; // 0=None, 1=Horn, 2=Note
} struct_arm_message;

// Type 2: Carbot Data
typedef struct struct_car_message {
  uint8_t deviceType; // 2 = Carbot
  int steerAngle;
  int speed;      // -255 to 255 (negative for backward)
  bool led;
  bool horn;
} struct_car_message;

struct_arm_message armData;
struct_car_message carData;

// Connection Status
volatile bool lastSendStatus = false;

// Callback when data is sent / Veri gönderildiğinde geri çağırma
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendStatus = (status == ESP_NOW_SEND_SUCCESS);
}

// Variables for Logic
static const int JOY_CENTER = 2048;
static const int DEADZONE = 300;
inline int joyDelta(int raw) { return raw - JOY_CENTER; }

// Mode switching
enum Mode { MODE_CARBOT = 0, MODE_ARMBOT = 1 };
static Mode currentMode = MODE_CARBOT;
static int modeSelIndex = 0; // 0=CARBOT, 1=ARMBOT
enum AppState { APP_SELECT = 0, APP_RUN = 1 };
static AppState appState = APP_SELECT;

// Buttons & Debounce
static bool encBtnPrev = false;
static unsigned long encLastMs = 0;
static const unsigned long ENC_DEBOUNCE_MS = 150;
static unsigned long bootMs0 = 0;
static const unsigned long ENC_BOOT_GUARD_MS = 800;

// Armbot State
static int armRotAngle = 90;     // Axis1
static int armShoulderAngle = 90;// Axis2
static int armElbowAngle = 50;   // Axis3
static int armGripAngle = 60;    // Gripper
static int lastEncoderVal = 0;
static bool armCalibrated = false;
static int joyXMin=4095, joyXMax=0, joyXCenter=2048;
static int joyYMin=4095, joyYMax=0, joyYCenter=2048;
static int joyDeadZoneX=150, joyDeadZoneY=150;
static const int JOY_STEP_MAX = 5;
static const int JOY_STEP_DIV = 300;

// Carbot State
static bool ledState = true;
static bool ledAutoMode = true;
static bool b3Prev = false;
static bool b2Prev = false;
static unsigned long b3LastChange = 0;
static const unsigned long B3_DEBOUNCE_MS = 50;
static unsigned long lastBeepMs = 0;
static const unsigned long BEEP_DEBOUNCE_MS = 180;

// Sending Timer
static unsigned long lastSendMs = 0;
static const unsigned long SEND_INTERVAL_MS = 50; // Send every 50ms

// Helper for LCD
void lcdWriteFixedTxt(int col, int row, const char *txt, int width) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%-*s", width, txt);
  iotbot.lcdWriteCR(col, row, buf);
}
void lcdWriteFixed(int col, int row, int value, int width) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%-*d", width, value);
  iotbot.lcdWriteCR(col, row, buf);
}

void showCarbotScreen() {
  iotbot.lcdClear();
  iotbot.lcdWriteCR(0, 0, "CARBOT (WIFI)"); // Shortened to avoid overlap
  iotbot.lcdWriteCR(0, 1, "LED:");
  lcdWriteFixedTxt(4, 1, ledState ? "ON" : "OFF", 3);
  iotbot.lcdWriteCR(7, 1, ledAutoMode ? "(A)" : "   ");
  // Status indicator at end of line 1
  iotbot.lcdWriteCR(12, 1, lastSendStatus ? "CON:OK " : "CON:ERR");
  
  lcdWriteFixedTxt(2, 2, "X:Steer  Y:Drive", 16);
  iotbot.lcdWriteCR(0, 3, "B1:Horn  ENC:MODE");
}

void showArmbotScreen() {
  iotbot.lcdClear();
  iotbot.lcdWriteCR(0, 0, "ARMBOT (WIFI)"); // Shortened
  iotbot.lcdWriteCR(0, 1, "ROT:"); lcdWriteFixedTxt(4, 1, "", 3);
  iotbot.lcdWriteCR(8, 1, "SHO:");  lcdWriteFixedTxt(12,1, "", 3);
  // Status indicator at end of line 0
  iotbot.lcdWriteCR(19, 0, lastSendStatus ? "*" : "!");

  iotbot.lcdWriteCR(0, 2, "ELB:"); lcdWriteFixedTxt(4, 2, "", 3);
  iotbot.lcdWriteCR(8, 2, "GRP:"); lcdWriteFixedTxt(12,2, "", 3);
  iotbot.lcdWriteCR(0, 3, "B1:Horn  B2:Note");
}

void updateModeSelectLine() {
  if (modeSelIndex == 0) lcdWriteFixedTxt(0, 2, " [CARBOT]   ARMBOT", 20);
  else lcdWriteFixedTxt(0, 2, "  CARBOT   [ARMBOT]", 20);
}

void showModeSelectScreen() {
  iotbot.lcdClear();
  iotbot.lcdWriteCR(1, 0, "Select Wireless Mode");
  updateModeSelectLine();
  lcdWriteFixedTxt(0, 3, "   Encoder: Start", 20);
}

void setup() {
  iotbot.begin();

  iotbot.serialStart(115200);
  iotbot.serialWrite("IOTBOT Wireless Control System Starting...");

  // Init ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Clean start
  
  // Force Channel 1
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  iotbot.serialWrite("WiFi Channel set to 1");

  if (esp_now_init() != ESP_OK) {
    iotbot.serialWrite("Error initializing ESP-NOW");
    return;
  }
  iotbot.serialWrite("ESP-NOW Initialized");
  esp_now_register_send_cb(OnDataSent);
  
  // Register Peer (Broadcast)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo)); // Initialize with zeros
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1; // Force Channel 1
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA; // Explicitly set interface to Station
  
  esp_err_t addStatus = esp_now_add_peer(&peerInfo);
  if (addStatus != ESP_OK){
    iotbot.serialWrite("Failed to add peer. Error Code: " + String(addStatus, HEX));
    return;
  }
  iotbot.serialWrite("Peer Added Successfully");

  showModeSelectScreen();
  bootMs0 = millis();
  lastEncoderVal = iotbot.encoderRead();
  
  // Init Data Types
  armData.deviceType = 1;
  carData.deviceType = 2;
  iotbot.serialWrite("Setup Complete. Loop starting...");
}

void loop() {
  unsigned long now = millis();
  bool encPressed = (digitalRead(ENCODER_BUTTON_PIN) == LOW);

  // --- Mode Selection ---
  if (appState == APP_SELECT) {
    int xRawSel = iotbot.joystickXRead();
    int dxSel = joyDelta(xRawSel);
    int prevSel = modeSelIndex;
    if (dxSel < -DEADZONE) modeSelIndex = 0;
    else if (dxSel > DEADZONE) modeSelIndex = 1;
    
    if (modeSelIndex != prevSel) updateModeSelectLine();

    if ((now - bootMs0) > ENC_BOOT_GUARD_MS && encPressed && !encBtnPrev && (now - encLastMs) > ENC_DEBOUNCE_MS) {
      currentMode = (modeSelIndex == 0) ? MODE_CARBOT : MODE_ARMBOT;
      if (currentMode == MODE_CARBOT) showCarbotScreen();
      else showArmbotScreen();
      appState = APP_RUN;
      encLastMs = now;
    }
    encBtnPrev = encPressed;
    return;
  }

  // --- Running Mode ---
  
  // Switch Mode
  if ((now - bootMs0) > ENC_BOOT_GUARD_MS && encPressed && !encBtnPrev && (now - encLastMs) > ENC_DEBOUNCE_MS) {
    currentMode = (currentMode == MODE_CARBOT) ? MODE_ARMBOT : MODE_CARBOT;
    if (currentMode == MODE_CARBOT) showCarbotScreen();
    else {
      showArmbotScreen();
      armCalibrated = false; // Recalibrate joystick for arm
    }
    encLastMs = now;
  }
  encBtnPrev = encPressed;

  // --- ARMBOT Logic ---
  if (currentMode == MODE_ARMBOT) {
    // Calibration
    if(!armCalibrated){
      long sumX=0,sumY=0; joyXMin=4095; joyXMax=0; joyYMin=4095; joyYMax=0;
      for(int i=0;i<40;i++){
        int x=iotbot.joystickXRead(); int y=iotbot.joystickYRead();
        sumX+=x; sumY+=y;
        if(x<joyXMin) joyXMin=x; if(x>joyXMax) joyXMax=x;
        if(y<joyYMin) joyYMin=y; if(y>joyYMax) joyYMax=y;
        delay(3);
      }
      joyXCenter = (int)(sumX/40);
      joyYCenter = (int)(sumY/40);
      int devX = max(joyXMax-joyXCenter, joyXCenter-joyXMin);
      int devY = max(joyYMax-joyYCenter, joyYCenter-joyYMin);
      joyDeadZoneX = constrain(devX + 60, 80, 300);
      joyDeadZoneY = constrain(devY + 60, 80, 300);
      armCalibrated = true;
    }

    int xRaw = iotbot.joystickXRead(); // Pin 35
    int yRaw = iotbot.joystickYRead(); // Pin 34
    int potRaw = iotbot.potentiometerRead(); // Pin 36
    
    // Safety: If Joystick X is 0 (likely disconnected or wrong pin), force center
    if (xRaw == 0) {
       xRaw = 2048; // Force Center
    }

    bool btn3 = iotbot.button3Read();
    bool encBtn = (digitalRead(ENCODER_BUTTON_PIN) == LOW);

    // Debug Joystick Values
    String debugMsg = "JoyX(35): " + String(xRaw) + " | JoyY: " + String(yRaw) + " | Pot: " + String(potRaw) + " | B3: " + String(btn3) + " | EncBtn: " + String(encBtn);
    iotbot.serialWrite(debugMsg);

    int dx = xRaw - joyXCenter;
    int dy = yRaw - joyYCenter;

    // Axis 1 (Rotation)
    if (abs(dx) > joyDeadZoneX) {
      int step = (abs(dx) / JOY_STEP_DIV) + 1;
      if(step > JOY_STEP_MAX) step = JOY_STEP_MAX;
      armRotAngle += (dx < 0 ? +step : -step);
      if (armRotAngle < 0) armRotAngle = 0;
      if (armRotAngle > 180) armRotAngle = 180;
    }

    // Axis 2 (Shoulder)
    if (abs(dy) > joyDeadZoneY) {
      int step = (abs(dy) / JOY_STEP_DIV) + 1;
      if(step > JOY_STEP_MAX) step = JOY_STEP_MAX;
      armShoulderAngle += (dy < 0 ? -step : step);
      if (armShoulderAngle < 0) armShoulderAngle = 0;
      if (armShoulderAngle > 180) armShoulderAngle = 180;
    }

    // Axis 3 (Elbow) - Potentiometer
    armElbowAngle = (int)((long)potRaw * 180L / 4095L);
    if (armElbowAngle < 0) armElbowAngle = 0;
    if (armElbowAngle > 180) armElbowAngle = 180;

    // Gripper - Encoder Fine Tuning
    int currEnc = iotbot.encoderRead();
    int encDiff = currEnc - lastEncoderVal;
    lastEncoderVal = currEnc;
    if(encDiff != 0){
      // Increased sensitivity for better response
      armGripAngle += encDiff * 5; 
      // Limit max angle to 120 as requested
      armGripAngle = constrain(armGripAngle, 10, 120);
    }

    // Gripper - Button 3 Toggle
    bool b3Now = iotbot.button3Read();
    if(b3Now && !b3Prev && (now - b3LastChange) > B3_DEBOUNCE_MS){
      const int GRIP_OPEN = 20;
      const int GRIP_CLOSE = 120;
      armGripAngle = (armGripAngle < (GRIP_OPEN + GRIP_CLOSE)/2) ? GRIP_CLOSE : GRIP_OPEN;
      b3LastChange = now;
    }
    b3Prev = b3Now;

    // Update LCD
    lcdWriteFixed(4, 1, armRotAngle, 3);
    lcdWriteFixed(12, 1, armShoulderAngle, 3);
    lcdWriteFixed(4, 2, armElbowAngle, 3);
    lcdWriteFixed(12, 2, armGripAngle, 3);

    // Prepare Data
    armData.axis1 = armRotAngle;
    armData.axis2 = armShoulderAngle;
    armData.axis3 = armElbowAngle;
    armData.gripper = armGripAngle;
    
    // Actions
    armData.action = 0;
    if(iotbot.button1Read()) armData.action = 1; // Horn
    else if(iotbot.button2Read()) armData.action = 2; // Note

    // Send Data Periodically
    if (now - lastSendMs >= SEND_INTERVAL_MS) {
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &armData, sizeof(armData));
      lastSendMs = now;
      // Update Status on LCD
      iotbot.lcdWriteCR(19, 0, lastSendStatus ? "*" : "!");
    }

  } 
  // --- CARBOT Logic ---
  else {
    int xRaw = iotbot.joystickXRead();
    int yRaw = iotbot.joystickYRead();
    int ldrRaw = iotbot.ldrRead();
    int dx = joyDelta(xRaw);
    int dy = joyDelta(yRaw);

    // Steering
    int angle = 90;
    if (abs(dx) > DEADZONE) {
      float norm = (float)dx / 2048.0f;
      angle = 90 + (int)(norm * 45.0f);
      if (angle < 45) angle = 45;
      if (angle > 135) angle = 135;
    }
    carData.steerAngle = angle;
    lcdWriteFixed(17, 0, angle, 3);

    // Speed / Direction
    int speed = 0;
    if (dy < -DEADZONE) { // Backward
       // Map dy (-DEADZONE to -2048) to (0 to -255)
       speed = map(dy, -DEADZONE, -2048, -50, -255);
    } else if (dy > DEADZONE) { // Forward
       speed = map(dy, DEADZONE, 2048, 50, 255);
    }
    carData.speed = speed;

    // LED Auto/Manual
    const int LDR_THRESHOLD = 1200;
    bool autoShouldOn = (ldrRaw < LDR_THRESHOLD);

    bool b2Now = iotbot.button2Read();
    if (b2Now && !b2Prev && (now - lastBeepMs) > BEEP_DEBOUNCE_MS) {
      ledAutoMode = !ledAutoMode;
      iotbot.lcdWriteCR(7, 1, ledAutoMode ? "(A)" : "   ");
      lastBeepMs = now;
    }
    b2Prev = b2Now;

    bool b3Now = iotbot.button3Read();
    if (!ledAutoMode && b3Now && !b3Prev && (now - b3LastChange) > B3_DEBOUNCE_MS) {
      ledState = !ledState;
      lcdWriteFixedTxt(4, 1, ledState ? "ON" : "OFF", 3);
      b3LastChange = now;
    }
    b3Prev = b3Now;

    if (ledAutoMode) {
      ledState = autoShouldOn;
      lcdWriteFixedTxt(4, 1, ledState ? "ON" : "OFF", 3);
    }
    carData.led = ledState;

    // Horn (Button 1)
    carData.horn = iotbot.button1Read();

    // Send Data Periodically
    if (now - lastSendMs >= SEND_INTERVAL_MS) {
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &carData, sizeof(carData));
      lastSendMs = now;
      // Update Status on LCD
      iotbot.lcdWriteCR(12, 1, lastSendStatus ? "CON:OK " : "CON:ERR");
    }
  }
}
