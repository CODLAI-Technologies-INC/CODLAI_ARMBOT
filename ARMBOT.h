/*
 * CODLAI ARMBOT Library
 * 
 * Structure Information:
 * This is a lightweight library designed for servo control.
 * It does not require a configuration file as it does not include heavy dependencies.
 * 
 * How to Add New Features:
 * Simply add new function declarations in ARMBOT.h and implementations in ARMBOT.cpp.
 * If adding heavy dependencies (like WiFi), consider implementing a Config file structure similar to IOTBOT.
 */

#ifndef ARMBOT_H
#define ARMBOT_H

#include <Arduino.h>

// Select appropriate library based on the platform / Platforma göre kütüphaneyi seç
#if defined(ESP32)
#include <ESP32Servo.h>
#elif defined(ESP8266)
#include <Servo.h>
#else
#include <Servo.h>
#endif

class ARMBOT
{
public:
  void begin();                                 // Initialize ARMBOT / ARMBOT'u başlat
  void end();                                   // Stop ARMBOT and detach servos / ARMBOT'u durdur ve servoları ayır
  void axis1Motion(int angle, int speed);       // Move Axis 1 / Eksen 1'i hareket ettir
  void axis2Motion(int angle, int speed);       // Move Axis 2 / Eksen 2'yi hareket ettir
  void axis3Motion(int angle, int speed);       // Move Axis 3 / Eksen 3'ü hareket ettir
  void gripperMotion(int angle, int speed);     // Move Gripper / Gripper'ı hareket ettir
  void waveHand(int count = 3);                 // Wave hand / El salla
  void buzzerPlay(int frequency, int duration); // Play a tone / Ses çıkar
  void buzzerStart(int frequency);              // Start playing tone / Sesi başlat
  void buzzerStop();                            // Stop playing tone / Sesi durdur
  void istiklalMarsiCal();                      // Play the National Anthem melody / İstiklal Marşı'nı çal

  /*********************************** Serial Port ***********************************
   */
  void serialStart(int baundrate);
  void serialWrite(const char *message);
  void serialWrite(String message);
  void serialWrite(long value);
  void serialWrite(int value);
  void serialWrite(float value);
  void serialWrite(bool value);

private:
  void moveToAngle(int &lastPos, int angle, int speed); // Smooth movement / Yumuşak hareket

  // Servo objects for the axes and gripper / Servo motor nesneleri
  Servo _axis1Servo, _axis2Servo, _axis3Servo, _gripperServo;
  int currentAngle = 0;

  // Last position variables for each axis / Her eksen için son pozisyon değişkenleri
  int _axis1LastPos = 90;
  int _axis2LastPos = 90;
  int _axis3LastPos = 50;
  int _gripperLastPos = 60;

  // Platform-specific pin assignments / Platforma özgü pin tanımlamaları
#if defined(ESP32)
  const int _axis1Pin = 25;
  const int _axis2Pin = 26;
  const int _axis3Pin = 27;
  const int _gripperPin = 32;
  const int _buzzerPin = 33;
#elif defined(ESP8266)
  const int _axis1Pin = 5;
  const int _axis2Pin = 4;
  const int _axis3Pin = 12;
  const int _gripperPin = 13;
  const int _buzzerPin = 14;
#else
#define AXIS1_CUSTOM_PIN
#define AXIS2_CUSTOM_PIN
#define AXIS3_CUSTOM_PIN
#define AXIS4_CUSTOM_PIN
#define BUZZER_CUSTOM_PIN

  const int _axis1Pin = AXIS1_CUSTOM_PIN;
  const int _axis2Pin = AXIS2_CUSTOM_PIN;
  const int _axis3Pin = AXIS3_CUSTOM_PIN;
  const int _gripperPin = AXIS4_CUSTOM_PIN;
  const int _buzzerPin = BUZZER_CUSTOM_PIN;
#endif
};

/*********************************** IMPLEMENTATION ***********************************/

// Initialize ARMBOT / ARMBOT'u başlat
inline void ARMBOT::begin()
{
#if defined(ESP32)
  Serial.println("Initializing ARMBOT on ESP32...");
#elif defined(ESP8266)
  Serial.println("Initializing ARMBOT on ESP8266...");
  analogWriteFreq(50); // **ESP8266 için PWM frekansını 50 Hz olarak ayarla**
#else
  Serial.println("Initializing ARMBOT on an unknown platform...");
#endif

  // Servo bağlama fonksiyonu / Function to attach servos
  auto attachServo = [](Servo &servo, int pin, const char *name)
  {
#if defined(ESP32)
    if (!servo.attach(pin, 500, 2500)) // **ESP32 için 1000-2000 µs kullan**
#elif defined(ESP8266)
    if (!servo.attach(pin, 500, 2500)) // **ESP8266 için PWM sinyal genişliği arttırıldı (500-2500 µs)**
#else
    if (!servo.attach(pin)) // **ESP32 için 1000-2000 µs kullan**
#endif
      Serial.println(String(name) + " Servo attach failed!");
  };

  // **Servo motorları bağla / Attach servos**
  attachServo(_axis1Servo, _axis1Pin, "Axis 1");
  attachServo(_axis2Servo, _axis2Pin, "Axis 2");
  attachServo(_axis3Servo, _axis3Pin, "Axis 3");
  attachServo(_gripperServo, _gripperPin, "Gripper");

  // **Servo motorlarını başlangıç pozisyonlarına ayarla / Set initial positions**
  _axis1Servo.write(_axis1LastPos);
  _axis2Servo.write(_axis2LastPos);
  _axis3Servo.write(_axis3LastPos);
  _gripperServo.write(_gripperLastPos);
}

// Stop ARMBOT and detach servos / ARMBOT'u durdur ve servoları ayır
inline void ARMBOT::end()
{
  _axis1Servo.detach();
  _axis2Servo.detach();
  _axis3Servo.detach();
  _gripperServo.detach();
}

// Smooth movement to target angle / Hedef açıya yumuşak hareket
inline void ARMBOT::moveToAngle(int &currentAngle, int angle, int speed)
{
  // Eğer açı sınırların dışındaysa, sınırlar içinde tut
  angle = constrain(angle, 0, 180);

  // Hız 0 ise bekleme yapmadan direkt yaz / If speed is 0, write directly without delay
  if (speed <= 0) {
    currentAngle = angle;
    if (&currentAngle == &_axis1LastPos) _axis1Servo.write(currentAngle);
    else if (&currentAngle == &_axis2LastPos) _axis2Servo.write(currentAngle);
    else if (&currentAngle == &_axis3LastPos) _axis3Servo.write(currentAngle);
    else if (&currentAngle == &_gripperLastPos) {
        _gripperServo.write(currentAngle);
    }
    return;
  }

  // İlk hareketi mevcut açıdan başlat (ilk başta 0'a gitmesini önlemek için)
  if (currentAngle == -1)
  {
    currentAngle = angle;
  }

  int step = (angle > currentAngle) ? 1 : -1; // Hareket yönünü belirle

  while (currentAngle != angle)
  {
    currentAngle += step;

    if (&currentAngle == &_axis1LastPos)
      _axis1Servo.write(currentAngle);
    if (&currentAngle == &_axis2LastPos)
      _axis2Servo.write(currentAngle);
    if (&currentAngle == &_axis3LastPos)
      _axis3Servo.write(currentAngle);
    if (&currentAngle == &_gripperLastPos)
      _gripperServo.write(currentAngle);

    delay(speed); // Hareket hızını kontrol et

    // Hedef açıya ulaşıldıysa çık
    if ((step > 0 && currentAngle >= angle) || (step < 0 && currentAngle <= angle))
    {
      currentAngle = angle;
      break;
    }
  }
}

// **Eksen kontrol fonksiyonları / Axis control functions**
inline void ARMBOT::axis1Motion(int angle, int speed) { moveToAngle(_axis1LastPos, angle, speed); }
inline void ARMBOT::axis2Motion(int angle, int speed) { moveToAngle(_axis2LastPos, angle, speed); }
inline void ARMBOT::axis3Motion(int angle, int speed) { moveToAngle(_axis3LastPos, angle, speed); }
inline void ARMBOT::gripperMotion(int angle, int speed) { moveToAngle(_gripperLastPos, angle, speed); }

// Wave hand / El salla
inline void ARMBOT::waveHand(int count)
{
  // Save current positions
  int startPos3 = _axis3LastPos;
  int startPos1 = _axis1LastPos;

  // Move Axis 3 to 120 degrees (Upright position)
  axis3Motion(120, 2);
  
  // Wave motion using Axis 1 (Base) only
  for(int i=0; i<count; i++) {
      // Move Axis 1 right
      axis1Motion(startPos1 + 40, 2);
      
      // Move Axis 1 left
      axis1Motion(startPos1 - 40, 2);
  }
  
  // Return to start positions
  axis1Motion(startPos1, 2);
  axis3Motion(startPos3, 2);
}

// Play a tone with the buzzer / Buzzer ile ses çıkar
inline void ARMBOT::buzzerPlay(int frequency, int duration)
{
  tone(_buzzerPin, frequency, duration);
  delay(duration);
}

inline void ARMBOT::buzzerStart(int frequency)
{
  tone(_buzzerPin, frequency);
}

inline void ARMBOT::buzzerStop()
{
  noTone(_buzzerPin);
}

// Play the National Anthem / İstiklal Marşı'nı çal
inline void ARMBOT::istiklalMarsiCal()
{
  buzzerPlay(262, 400);
  delay(400);
  buzzerPlay(330, 400);
  delay(400);
  buzzerPlay(392, 400);
  delay(400);
  buzzerPlay(349, 400);
  delay(400);
  buzzerPlay(330, 600);
  delay(600);
  buzzerPlay(349, 400);
  delay(400);
  buzzerPlay(330, 400);
  delay(400);
  buzzerPlay(294, 400);
  delay(400);
  buzzerPlay(262, 600);
  delay(600);
  buzzerPlay(330, 400);
  delay(400);
  buzzerPlay(349, 400);
  delay(400);
  buzzerPlay(392, 400);
  delay(400);
  buzzerPlay(330, 600);
  delay(600);
  buzzerPlay(294, 400);
  delay(400);
  buzzerPlay(262, 400);
  delay(400);
  buzzerPlay(294, 600);
  delay(600);
}

/*********************************** Serial Port ***********************************
 */
inline void ARMBOT::serialStart(int baudrate)
{
  Serial.begin(baudrate);
}

inline void ARMBOT::serialWrite(const char *message)
{
  Serial.println(message);
}

inline void ARMBOT::serialWrite(String message)
{
  Serial.println(message.c_str());
}

inline void ARMBOT::serialWrite(long value)
{
  Serial.println(String(value).c_str());
}

inline void ARMBOT::serialWrite(int value)
{
  Serial.println(String(value).c_str());
}

inline void ARMBOT::serialWrite(float value)
{
  Serial.println(String(value).c_str());
}

inline void ARMBOT::serialWrite(bool value)
{
  Serial.println(value ? "true" : "false");
}

#endif
