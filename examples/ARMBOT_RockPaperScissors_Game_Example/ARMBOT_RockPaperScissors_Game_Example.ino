// TR: EGLENCELI PROJE - Tas Kagit Makas Oyunu. Butona her basisinizda
// ARMBOT "Tas, Kagit, Makas... CEKTIK!" sayimini buzzer ile yapar ve
// ardindan RASTGELE bir el hareketi yaparak KENDI secimini gosterir
// (Tas = yumruk/kapali gripper, Kagit = acik el/acik gripper, Makas =
// gripper'in hizlica ac-kapa yapmasi). Siz de ayni anda kendi elinizle
// bir secim yapip robota karsi oynayabilirsiniz - kim kazandi, siz karar
// verin!
// EN: A FUN PROJECT - Rock-Paper-Scissors Game. Every time you press the
// button, ARMBOT plays a "Rock, Paper, Scissors... Shoot!" countdown on
// the buzzer, then makes a RANDOM hand gesture to reveal its own choice
// (Rock = closed fist/closed gripper, Paper = open hand/open gripper,
// Scissors = the gripper snipping open-close quickly). Play your own
// hand at the same time and decide who wins!
//
// NOT / NOTE: Asagidaki aci (angle) degerleri genel bir ARMBOT icin
// ayarlanmistir; kendi kolunuzun kalibrasyonuna gore GRIPPER_OPEN_ANGLE /
// GRIPPER_CLOSED_ANGLE degerlerini ayarlamaniz gerekebilir. / The angle
// values below are tuned for a typical ARMBOT; you may need to adjust
// GRIPPER_OPEN_ANGLE / GRIPPER_CLOSED_ANGLE for your own arm's calibration.

#include <ARMBOT.h>

#define LED_PIN 16 // Minibot mavi LED pini / Minibot blue LED pin
#define B1_PIN 0   // Minibot uzerindeki dahili buton / Minibot's built-in button

ARMBOT armbot;

// TR/EN: Bu degeri false yapip yeniden yukleyerek dili degistirebilirsiniz.
// Change this to false and re-upload to switch the language.
bool turkish = true;

namespace {
  constexpr int kSpeed = 15;
  constexpr int kGripperOpenAngle = 100;   // Kagit / Paper
  constexpr int kGripperClosedAngle = 10;  // Tas / Rock
  constexpr int kGripperScissorsAngle = 55; // Makas orta konumu / Scissors mid position

  enum Choice { ROCK = 0, PAPER = 1, SCISSORS = 2 };

  bool lastButtonState = true; // digitalRead: HIGH = birakilmis / released

  void say(const char *tr, const char *en) {
    armbot.serialWrite(turkish ? tr : en);
  }

  void countdown() {
    say("Tas...", "Rock...");
    armbot.buzzerPlay(400, 200);
    delay(500);
    say("Kagit...", "Paper...");
    armbot.buzzerPlay(500, 200);
    delay(500);
    say("Makas...", "Scissors...");
    armbot.buzzerPlay(600, 200);
    delay(500);
    say("CEKTIK!", "SHOOT!");
    armbot.buzzerPlay(900, 300);
  }

  void showRock() {
    say("ARMBOT secimi: TAS (yumruk)", "ARMBOT picked: ROCK (fist)");
    armbot.axis2Motion(90, kSpeed);
    armbot.axis3Motion(80, kSpeed);
    armbot.gripperMotion(kGripperClosedAngle, kSpeed);
  }

  void showPaper() {
    say("ARMBOT secimi: KAGIT (acik el)", "ARMBOT picked: PAPER (open hand)");
    armbot.axis2Motion(90, kSpeed);
    armbot.axis3Motion(40, kSpeed);
    armbot.gripperMotion(kGripperOpenAngle, kSpeed);
  }

  void showScissors() {
    say("ARMBOT secimi: MAKAS (kes kes)", "ARMBOT picked: SCISSORS (snip snip)");
    armbot.axis2Motion(90, kSpeed);
    armbot.axis3Motion(60, kSpeed);
    // Makas gibi hizlica ac-kapa yap / snip open-close quickly
    for (int i = 0; i < 3; ++i) {
      armbot.gripperMotion(kGripperOpenAngle, 30);
      delay(150);
      armbot.gripperMotion(kGripperScissorsAngle, 30);
      delay(150);
    }
  }
}

void setup() {
  armbot.begin();
  armbot.serialStart(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(B1_PIN, INPUT_PULLUP);
  randomSeed(analogRead(A0));

  armbot.calibrationPose(kSpeed);
  say("Tas Kagit Makas hazir! Baslatmak icin butona basin."
     , "Rock-Paper-Scissors ready! Press the button to play.");
}

void loop() {
  bool buttonState = (digitalRead(B1_PIN) == HIGH);

  if (lastButtonState == true && buttonState == false) {
    digitalWrite(LED_PIN, HIGH);
    countdown();

    Choice pick = static_cast<Choice>(random(0, 3));
    switch (pick) {
      case ROCK: showRock(); break;
      case PAPER: showPaper(); break;
      case SCISSORS: showScissors(); break;
    }

    delay(1500);
    armbot.calibrationPose(kSpeed);
    digitalWrite(LED_PIN, LOW);
    say("Tekrar oynamak icin butona basin.", "Press the button to play again.");
    delay(300); // debounce
  }
  lastButtonState = buttonState;

  delay(20);
}
