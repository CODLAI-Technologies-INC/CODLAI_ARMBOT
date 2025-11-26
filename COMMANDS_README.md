# CODLAI_ARMBOT Library Documentation / Kütüphane Dokümantasyonu

**EN:** The `ARMBOT` library is used to control a 4-axis (3 axes + gripper) robot arm.
**TR:** `ARMBOT` kütüphanesi, 4 eksenli (3 eksen + tutucu/gripper) bir robot kolu kontrol etmek için kullanılır.

### Basic Control / Temel Kontrol
*   `void begin()`
    *   **EN:** Initializes the servos and moves them to the starting position.
    *   **TR:** Servoları başlatır ve başlangıç pozisyonlarına getirir.
*   `void end()`
    *   **EN:** Detaches the servos.
    *   **TR:** Servoları serbest bırakır (detach).
*   `void axis1Motion(int angle, int speed)`
    *   **EN:** Moves the 1st Axis (Base).
    *   **TR:** 1. Ekseni (Taban) hareket ettirir.
*   `void axis2Motion(int angle, int speed)`
    *   **EN:** Moves the 2nd Axis.
    *   **TR:** 2. Ekseni hareket ettirir.
*   `void axis3Motion(int angle, int speed)`
    *   **EN:** Moves the 3rd Axis.
    *   **TR:** 3. Ekseni hareket ettirir.
*   `void gripperMotion(int angle, int speed)`
    *   **EN:** Opens/Closes the Gripper.
    *   **TR:** Tutucuyu (Gripper) Açar/Kapatır.
*   `void waveHand(int count)`
    *   **EN:** Makes the robot arm perform a waving gesture.
    *   **TR:** Robot kol ile el sallama hareketi yapar.

### Sound and Music / Ses ve Müzik
*   `void buzzerPlay(int frequency, int duration)`
    *   **EN:** Plays a tone at the specified frequency and duration.
    *   **TR:** Belirtilen frekans ve sürede ton çalar.
*   `void buzzerStart(int frequency)`
    *   **EN:** Starts the sound.
    *   **TR:** Sesi başlatır.
*   `void buzzerStop()`
    *   **EN:** Stops the sound.
    *   **TR:** Sesi durdurur.
*   `void istiklalMarsiCal()`
    *   **EN:** Plays the Turkish National Anthem (İstiklal Marşı).
    *   **TR:** İstiklal Marşı'nı çalar.

### Serial Communication / Seri Haberleşme
*   `void serialStart(int baudrate)`
    *   **EN:** Starts the serial port.
    *   **TR:** Seri portu başlatır.
*   `void serialWrite(...)`
    *   **EN:** Writes data to the serial port (String, int, float, bool).
    *   **TR:** Seri porta veri yazar (String, int, float, bool).
