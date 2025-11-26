#include <ARMBOT.h>
ARMBOT armbot;

void setup()
{
  armbot.serialStart(115200); // Initialize serial communication (Seri haberleşmeyi başlatır)
  armbot.begin(); // Initialize the ARMBOT library (ARMbot kütüphanesini başlatır)
  
  armbot.serialWrite("ARMBOT Basic Example Started");

  // Example of playing a buzzer tone (Buzzer çalma örneği)
  armbot.buzzerPlay(200, 300); // Play a tone at 200 Hz for 300 ms (200 Hz frekansta 300 ms süreyle çalar)
  delay(1000);
}

void loop()
{
  // Axis 1 movements (Eksen 1 hareketleri)
  armbot.serialWrite("Moving Axis 1...");
  armbot.buzzerPlay(100, 200); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);

  armbot.axis1Motion(0, 20); // Move Axis 1 to 0 degrees at speed 20 (Eksen 1'i sıfır açısına 20 hızda hareket ettirir)
  delay(1000);
  armbot.axis1Motion(180, 20); // Move Axis 1 to 180 degrees at speed 20 (Eksen 1'i 180 açısına 20 hızda hareket ettirir)
  delay(1000);
  armbot.axis1Motion(90, 20); // Move Axis 1 to 90 degrees at speed 20 (Eksen 1'i 90 açısına 20 hızda hareket ettirir)
  delay(1000);

  // Axis 2 movements (Eksen 2 hareketleri)
  armbot.serialWrite("Moving Axis 2...");
  armbot.buzzerPlay(100, 50); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);
  armbot.buzzerPlay(200, 200); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);

  armbot.axis2Motion(0, 20); // Move Axis 2 to 0 degrees at speed 20 (Eksen 2'yi sıfır açısına 20 hızda hareket ettirir)
  delay(1000);
  armbot.axis2Motion(180, 20); // Move Axis 2 to 180 degrees at speed 20 (Eksen 2'yi 180 açısına 20 hızda hareket ettirir)
  delay(1000);
  armbot.axis2Motion(90, 20); // Move Axis 2 to 90 degrees at speed 20 (Eksen 2'yi 90 açısına 20 hızda hareket ettirir)
  delay(1000);

  // Axis 3 movements (Eksen 3 hareketleri)
  armbot.serialWrite("Moving Axis 3...");
  armbot.buzzerPlay(100, 50); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);
  armbot.buzzerPlay(150, 50); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);
  armbot.buzzerPlay(200, 200); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);

  armbot.axis3Motion(20, 20); // Move Axis 3 to 20 degrees at speed 20 (Eksen 3'ü 20 açısına 20 hızda hareket ettirir)
  delay(1000);
  armbot.axis3Motion(180, 20); // Move Axis 3 to 180 degrees at speed 20 (Eksen 3'ü 180 açısına 20 hızda hareket ettirir)
  delay(1000);
  armbot.axis3Motion(50, 20); // Move Axis 3 to 50 degrees at speed 20 (Eksen 3'ü 50 açısına 20 hızda hareket ettirir)
  delay(1000);

  // Gripper movements (Gripper hareketleri)
  armbot.serialWrite("Moving Gripper...");
  armbot.buzzerPlay(100, 50); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);
  armbot.buzzerPlay(150, 50); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);
  armbot.buzzerPlay(200, 50); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);
  armbot.buzzerPlay(225, 200); // Play a tone before moving (Hareketten önce bir ses çalar)
  delay(100);

  armbot.gripperMotion(0, 20); // Open gripper to 0 degrees at speed 20 (Gripper'ı 0 açısına 20 hızda açar)
  delay(1000);
  armbot.gripperMotion(110, 20); // Close gripper to 110 degrees at speed 20 (Gripper'ı 110 açısına 20 hızda kapatır)
  delay(1000);
  armbot.gripperMotion(60, 20); // Move gripper to 60 degrees at speed 20 (Gripper'ı 60 açısına 20 hızda hareket ettirir)
  delay(1000);
}
