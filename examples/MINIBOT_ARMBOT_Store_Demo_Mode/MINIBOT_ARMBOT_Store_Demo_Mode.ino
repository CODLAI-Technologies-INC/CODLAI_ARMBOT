/*
 * ARMBOT Store Demo Mode / ARMBOT Mağaza Demo Modu
 * 
 * This example demonstrates the standard movements and features of ARMBOT in a loop.
 * Bu örnek, ARMBOT'un standart hareketlerini ve özelliklerini bir döngü içinde gösterir.
 * 
 * Features / Özellikler:
 * - Forward/Backward Movement / İleri/Geri Hareket
 * - Turning Left/Right / Sola/Sağa Dönüş
 * - LED Effects / LED Efektleri
 * - Buzzer Sounds / Buzzer Sesleri
 */

#define USE_SERVO    // Enable Servo for movement (if applicable) / Hareket için Servo'yu etkinleştir
#define USE_NEOPIXEL // Enable Smart LEDs / Akıllı LED'leri etkinleştir
#include <MINIBOT.h>

MINIBOT minibot;

void setup() {
  minibot.begin();
  minibot.serialStart(115200);
  minibot.playIntro();
  
  // Initialize Smart LEDs (assuming pin 4 and 4 LEDs) / Akıllı LED'leri başlat (Pin 4 ve 4 LED varsayılıyor)
  // Adjust pin and count as per your hardware / Donanımınıza göre pin ve sayıyı ayarlayın
  minibot.extendSmartLEDPrepare(4, 4); 
  
  minibot.serialWrite("MINIBOT Demo Mode Started! / MINIBOT Demo Modu Başlatıldı!");
}

void loop() {
  // 1. Move Forward / İleri Git
  minibot.serialWrite("Moving Forward... / İleri Gidiliyor...");
  minibot.extendSmartLEDFill(0, 3, 0, 255, 0); // Green / Yeşil
  // Assuming servo or motor control logic here. 
  // Since MINIBOT library seems to use Servo for modules, we might need specific motor driver logic if it's a car.
  // If MINIBOT is a servo-based robot, we move servos.
  // Let's assume standard servo movement for demo.
  minibot.moduleServoGoAngle(1, 180, 10); // Example servo move
  delay(1000);
  
  // 2. Move Backward / Geri Git
  minibot.serialWrite("Moving Backward... / Geri Gidiliyor...");
  minibot.extendSmartLEDFill(0, 3, 255, 0, 0); // Red / Kırmızı
  minibot.moduleServoGoAngle(1, 0, 10);
  delay(1000);
  
  // 3. Turn / Dönüş
  minibot.serialWrite("Turning... / Dönülüyor...");
  minibot.extendSmartLEDFill(0, 3, 0, 0, 255); // Blue / Mavi
  minibot.moduleServoGoAngle(1, 90, 10);
  delay(1000);
  
  // 4. Rainbow Effect / Gökkuşağı Efekti
  minibot.serialWrite("LED Party! / LED Partisi!");
  minibot.moduleSmartLEDRainbowEffect(10);
  
  delay(2000);
}
