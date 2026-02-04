/*
  ARMBOT-only Store Mode
  - Uses only ARMBOT APIs (servos + buzzer + serial)
  - Runs a continuous demo loop (store mode) on boot
*/

#include <ARMBOT.h>

ARMBOT armbot;

void setup()
{
  armbot.begin();
  armbot.serialStart(115200);

  armbot.serialWrite("ARMBOT Store Mode ready");
  // small intro melody
  armbot.buzzerPlay(800, 120);
  delay(150);
}

// Demo sequence using ARMBOT motions
void storeModeActions()
{
  while (true) {
    armbot.serialWrite("Cycle: base sweep");
    // announce + small preparatory chirp
    armbot.buzzerPlay(600, 80);
    delay(60);
    armbot.buzzerPlay(900, 60);
    // smoother automated sweep
    armbot.axis1Motion(0, 10);
    delay(250);
    armbot.axis1Motion(180, 10);
    delay(500);
    armbot.axis1Motion(90, 10);
    delay(250);

    armbot.serialWrite("Cycle: shoulder/elbow");
    // announce shoulder/elbow move with lower tone
    armbot.buzzerPlay(700, 70);
    delay(50);
    // move with staggered timings to look automated
    armbot.axis2Motion(40, 12);
    delay(120);
    armbot.axis3Motion(120, 12);
    delay(420);
    armbot.axis2Motion(120, 12);
    delay(120);
    armbot.axis3Motion(30, 12);
    delay(420);
    armbot.axis2Motion(90, 10);
    armbot.axis3Motion(60, 10);
    delay(320);

    armbot.serialWrite("Cycle: gripper open/close");
    armbot.buzzerPlay(900, 60);
    armbot.gripperMotion(20, 8);
    delay(320);
    armbot.buzzerPlay(1000, 60);
    armbot.gripperMotion(120, 8);
    delay(320);

    armbot.serialWrite("Cycle: wave hand");
    armbot.buzzerPlay(750, 60);
    delay(60);
    armbot.waveHand(3);

    armbot.buzzerPlay(1000, 100);
    delay(120);
    armbot.buzzerPlay(1400, 120);
    delay(200);

    delay(800);
  }
}

void loop()
{
  // start store/demo mode immediately
  storeModeActions();
}
