/*
 * ARMBOT Wave Hand Example
 * 
 * This example demonstrates how to use the waveHand() function
 * to make the robot arm wave its hand (Axis 3).
 * 
 * Hardware:
 * - CODLAI ARMBOT
 */

#include <ARMBOT.h>

ARMBOT armbot;

void setup() {
  // Initialize serial communication for debugging
  armbot.serialStart(115200);
  
  // Initialize the ARMBOT
  armbot.begin();
  
  armbot.serialWrite("ARMBOT Initialized.");
  armbot.serialWrite("Get ready to wave!");
  
  // Wait a bit before starting
  delay(2000);
}

void loop() {
  armbot.serialWrite("Waving hand...");
  
  // Wave hand 3 times (default)
  armbot.waveHand();
  
  delay(1000);
  
  armbot.serialWrite("Waving hand 5 times...");
  
  // Wave hand 5 times
  armbot.waveHand(5);
  
  // Wait for 5 seconds before repeating
  delay(5000);
}
