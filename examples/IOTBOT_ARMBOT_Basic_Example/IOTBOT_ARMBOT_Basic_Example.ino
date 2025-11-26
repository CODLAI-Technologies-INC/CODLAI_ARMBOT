#include <IOTBOT.h>
#include <ARMBOT.h>

IOTBOT iotbot;
ARMBOT armbot;

void setup() {
  iotbot.begin();
  armbot.begin();
  iotbot.lcdClear();
  iotbot.lcdWriteCR(0, 0, "IOTBOT ARMBOT");
  iotbot.lcdWriteCR(0, 1, "Basic Example");
}

void loop() {
  // Simple movement demo
  // Move Axis 1 (Rotation) from 0 to 180
  armbot.axis1Motion(0, 10);
  delay(1000);
  armbot.axis1Motion(180, 10);
  delay(1000);
  
  // Move Axis 2 (Shoulder)
  armbot.axis2Motion(45, 10);
  delay(1000);
  armbot.axis2Motion(135, 10);
  delay(1000);
}
