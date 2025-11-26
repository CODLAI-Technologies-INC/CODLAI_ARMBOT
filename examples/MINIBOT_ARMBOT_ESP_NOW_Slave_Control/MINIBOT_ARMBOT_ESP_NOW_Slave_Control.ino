/*
 * ARMBOT ESP-NOW Slave Control / ARMBOT ESP-NOW Slave Kontrolü
 *
 * This example receives commands from an IOTBOT (Master) via ESP-NOW to control servos/axes.
 * Bu örnek, IOTBOT'tan (Master) ESP-NOW üzerinden gelen komutları alarak servoları/eksenleri kontrol eder.
 */

#define USE_ESPNOW
#define USE_WIFI
#define USE_SERVO
#include <MINIBOT.h>
#include <ARMBOT.h>

MINIBOT minibot;
ARMBOT armbot;

void setup()
{
  minibot.begin();
  minibot.serialStart(115200);
  minibot.playIntro();

  // Initialize ARMBOT (Attaches servos to pins 5, 4, 12, 13)
  // Buzzer is on Pin 14
  armbot.begin();

  minibot.serialWrite("Initializing ESP-NOW Slave...");

  minibot.initESPNow();
  minibot.setWiFiChannel(1); // Master ile aynı kanalda olmalı / Must be on same channel as Master

  minibot.startListening();
  minibot.serialWrite("Ready to receive commands!");
}

void loop()
{
  if (minibot.newData)
  {
    minibot.newData = false;

    if (minibot.receivedData.deviceType != 1)
      return; // Not for Armbot

    // Move Servos using ARMBOT Library
    // Speed 0 = Instant (or as fast as possible)
    armbot.axis1Motion(minibot.receivedData.axis1, 0);
    armbot.axis2Motion(minibot.receivedData.axis2, 0);
    armbot.axis3Motion(minibot.receivedData.axis3, 0);
    armbot.gripperMotion(minibot.receivedData.gripper, 0);

    // Actions
    if (minibot.receivedData.action == 1)
    {
      armbot.buzzerPlay(1000, 50); // Horn
    }
    else if (minibot.receivedData.action == 2)
    {
      armbot.buzzerPlay(2000, 100); // Note
    }
  }
}
