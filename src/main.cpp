#include <Arduino.h>
#include <Joystick.h>
#include "HX711.h"

// Joystick initialisieren:
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
                   JOYSTICK_TYPE_JOYSTICK,
                   0, 0,
                   false, false, false, // X, Y, Z
                   false, false, false, // Rx, Ry, Rz
                   false,               // Rudder
                   true,                // Throttle (Gas)
                   true,                // Accelerator (Kupplung → optional)
                   true,                // Brake
                   false);              // Steering

const int pinClutch = A0;
const int pinThrottle = A1;
const int DT_PIN = 5;
const int SCK_PIN = 4;

// 🔧 Kalibrierung
const int clutchMin = 579;
const int clutchMax = 693;

const int throttleMin = 619;
const int throttleMax = 770;

const long brakeMin = 90000;
const long brakeMax = 140000;

// ⚙️ Deadzone in beiden Richtungen um 0
const int deadzone = 20;

// 🧠 Funktion zum Anwenden der Deadzone
int applyDeadzone(int value, int center, int width) {
  if (abs(value - center) < width) {
    return center;
  }
  return value;
}

HX711 scale;

void setup() {
  Serial.begin(115200);
  scale.begin(DT_PIN, SCK_PIN);
  Joystick.begin();
}

void loop() {
  // Rohdaten einlesen
  int clutchRaw = analogRead(pinClutch);
  int throttleRaw = analogRead(pinThrottle);
  long brakeRaw = scale.read();

  // Kalibrierung auf 0–1023
  int clutch = map(constrain(clutchRaw, clutchMin, clutchMax), clutchMin, clutchMax, 0, 1023);
  int throttle = map(constrain(throttleRaw, throttleMin, throttleMax), throttleMin, throttleMax, 0, 1023);
  int brake = map(constrain(brakeRaw, brakeMin, brakeMax), brakeMin, brakeMax, 0, 1023);

  // Deadzone um 0 anwenden
  clutch = applyDeadzone(clutch, 0, deadzone);
  throttle = applyDeadzone(throttle, 0, deadzone);
  brake = applyDeadzone(brake, 0, deadzone);

  // An Joystick senden
  Joystick.setAccelerator(clutch);
  Joystick.setThrottle(throttle);
  Joystick.setBrake(brake);

  // Serielle Ausgabe für Plotter
  Serial.print(clutch);
  Serial.print('\t');
  Serial.print(throttle);
  Serial.print('\t');
  Serial.println(brake);


  delay(10);
}
