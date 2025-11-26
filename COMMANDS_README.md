# CODLAI ARMBOT Library Reference / Kütüphane Referansı

## Introduction / Giriş
**EN:** The ARMBOT library is designed to control a 4-axis robotic arm (3 axes + gripper). It provides simple functions to move servos smoothly and control the buzzer.
**TR:** ARMBOT kütüphanesi, 4 eksenli bir robot kolu (3 eksen + tutucu) kontrol etmek için tasarlanmıştır. Servoları yumuşak bir şekilde hareket ettirmek ve buzzer'ı kontrol etmek için basit fonksiyonlar sağlar.

## Functions / Fonksiyonlar

### begin
**EN:** Initializes the library, attaches servos to their pins, and moves them to the default starting positions.
**TR:** Kütüphaneyi başlatır, servoları pinlerine bağlar ve varsayılan başlangıç pozisyonlarına getirir.
**Syntax:** `void begin()`

### end
**EN:** Detaches all servos, stopping the PWM signals.
**TR:** Tüm servoları ayırır, PWM sinyallerini durdurur.
**Syntax:** `void end()`

### axis1Motion
**EN:** Moves the first axis (base) to the specified angle with a specified speed delay.
**TR:** Birinci ekseni (taban) belirtilen açıya, belirtilen hız gecikmesiyle hareket ettirir.
**Syntax:** `void axis1Motion(int angle, int speed)`
*   `angle`: Target angle (0-180) / Hedef açı (0-180)
*   `speed`: Delay in ms between steps (lower is faster) / Adımlar arası ms cinsinden gecikme (düşük değer daha hızlıdır)

### axis2Motion
**EN:** Moves the second axis to the specified angle with a specified speed delay.
**TR:** İkinci ekseni belirtilen açıya, belirtilen hız gecikmesiyle hareket ettirir.
**Syntax:** `void axis2Motion(int angle, int speed)`

### axis3Motion
**EN:** Moves the third axis to the specified angle with a specified speed delay.
**TR:** Üçüncü ekseni belirtilen açıya, belirtilen hız gecikmesiyle hareket ettirir.
**Syntax:** `void axis3Motion(int angle, int speed)`

### gripperMotion
**EN:** Opens or closes the gripper to the specified angle with a specified speed delay.
**TR:** Tutucuyu (gripper) belirtilen açıya, belirtilen hız gecikmesiyle açar veya kapatır.
**Syntax:** `void gripperMotion(int angle, int speed)`

### waveHand
**EN:** Moves Axis 3 to 120 degrees and waves the base (Axis 1) left and right rapidly.
**TR:** Eksen 3'ü 120 dereceye getirir ve tabanı (Eksen 1) hızlıca sağa sola sallar.
**Syntax:** `void waveHand(int count = 3)`
*   `count`: Number of waves (Default: 3) / Sallama sayısı (Varsayılan: 3)

### buzzerPlay
**EN:** Plays a tone at a specific frequency for a specific duration.
**TR:** Belirli bir frekansta ve sürede bir ton çalar.
**Syntax:** `void buzzerPlay(int frequency, int duration)`

### istiklalMarsiCal
**EN:** Plays the Turkish National Anthem melody using the buzzer.
**TR:** Buzzer kullanarak İstiklal Marşı melodisini çalar.
**Syntax:** `void istiklalMarsiCal()`

### serialStart
**EN:** Starts serial communication at the specified baud rate.
**TR:** Seri haberleşmeyi belirtilen baud hızında başlatır.
**Syntax:** `void serialStart(int baudrate)`

### serialWrite
**EN:** Writes data to the serial port (Overloaded for String, int, float, bool).
**TR:** Seri porta veri yazar (String, int, float, bool için aşırı yüklenmiştir).
**Syntax:** `void serialWrite(data)`
