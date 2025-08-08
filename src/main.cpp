#include <Arduino.h>
#include <Joystick.h>

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

const int pinBrake = A0;
const int pinThrottle = A1;

// 🔧 Kalibrierung
const int brakeMin = 579;
const int brakeMax = 693;

const int throttleMin = 619;
const int throttleMax = 804;

// ⚙️ Deadzone in beiden Richtungen um 0
const int deadzone = 20;

// 🧠 Funktion zum Anwenden der Deadzone
int applyDeadzone(int value, int center, int width) {
  if (abs(value - center) < width) {
    return center;
  }
  return value;
}

void setup() {
  Serial.begin(115200);
  Joystick.begin();
}

void loop() {
  // Rohdaten einlesen
  int brakeRaw = analogRead(pinBrake);
  int throttleRaw = analogRead(pinThrottle);

  // Kalibrierung auf 0–1023
  int brake = map(constrain(brakeRaw, brakeMin, brakeMax), brakeMin, brakeMax, 0, 1023);
  int throttle = map(constrain(throttleRaw, throttleMin, throttleMax), throttleMin, throttleMax, 0, 1023);

  // Deadzone um 0 anwenden
  brake = applyDeadzone(brake, 0, deadzone);
  throttle = applyDeadzone(throttle, 0, deadzone);

  // An Joystick senden
  Joystick.setBrake(brake);
  Joystick.setThrottle(throttle);

  // Serielle Ausgabe für Plotter
  Serial.print(brake);
  Serial.print('\t');
  Serial.println(throttle);

  delay(10);
}
