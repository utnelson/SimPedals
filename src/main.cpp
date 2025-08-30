#include <Arduino.h>
#include <Joystick.h>
#include "HX711.h"
#include <EEPROM.h>

// 🔹 Forward-Declarations
void saveConfig();
void loadConfig();
void printConfig();
int applyDeadzone(int value, int center, int width);

// 🎮 Joystick initialisieren
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
                   JOYSTICK_TYPE_JOYSTICK,
                   0, 0,
                   false, false, false, // X, Y, Z
                   false, false, false, // Rx, Ry, Rz
                   false,               // Rudder
                   true,                // Throttle
                   true,                // Accelerator
                   true,                // Brake
                   false);              // Steering

struct Config
{
  bool debugMode;
  int16_t clutchMin;
  int16_t clutchMax;
  int16_t throttleMin;
  int16_t throttleMax;
  long brakeMin;
  long brakeMax;
  int16_t deadzone;
  int16_t pinClutch;
  int16_t pinThrottle;
  int16_t DT_PIN;
  int16_t SCK_PIN;
  int16_t brakeSmoothing; // neu: Glättungsfaktor für Bremse
} config;

// 🔧 Defaultwerte
Config defaultConfig = {
    true,        // debugMode
    0, 800,      // clutchMin, clutchMax
    0, 800,      // throttleMin, throttleMax
    0L, 200000L, // brakeMin, brakeMax
    20,          // deadzone
    A0,          // pinClutch
    A1,          // pinThrottle
    5,           // DT_PIN
    4,           // SCK_PIN
    10           // brakeSmoothing (10% neu, 90% alt)
};

#define MAGIC_BYTE_ADDR 0
#define MAGIC_BYTE_VAL 0x42

HX711 scale;

// 🧠 EEPROM-Funktionen
void saveConfig()
{
  EEPROM.put(MAGIC_BYTE_ADDR + 1, config);
  EEPROM.write(MAGIC_BYTE_ADDR, MAGIC_BYTE_VAL);
  Serial.println("✅ Config gespeichert");
}

void loadConfig()
{
  if (EEPROM.read(MAGIC_BYTE_ADDR) != MAGIC_BYTE_VAL)
  {
    // EEPROM leer → Default laden
    config = defaultConfig;
    EEPROM.write(MAGIC_BYTE_ADDR, MAGIC_BYTE_VAL);
    EEPROM.put(MAGIC_BYTE_ADDR + 1, config);
    Serial.println("⚠️ EEPROM leer → DefaultConfig gesetzt");
  }
  else
  {
    EEPROM.get(MAGIC_BYTE_ADDR + 1, config);
  }
  printConfig();
}

void printConfig()
{
  Serial.print(config.debugMode ? "1" : "0"); Serial.print(",");
  Serial.print(config.clutchMin); Serial.print(",");
  Serial.print(config.clutchMax); Serial.print(",");
  Serial.print(config.throttleMin); Serial.print(",");
  Serial.print(config.throttleMax); Serial.print(",");
  Serial.print(config.brakeMin); Serial.print(",");
  Serial.print(config.brakeMax); Serial.print(",");
  Serial.print(config.deadzone); Serial.print(",");
  Serial.print(config.pinClutch); Serial.print(",");
  Serial.print(config.pinThrottle); Serial.print(",");
  Serial.print(config.DT_PIN); Serial.print(",");
  Serial.print(config.SCK_PIN); Serial.print(",");
  Serial.println(config.brakeSmoothing); // \n am Ende
}


int applyDeadzone(int value, int center, int width)
{
  if (abs(value - center) < width)
    return center;
  return value;
}

// 🔹 Glättungsspeicher für die Bremse
float brakeFiltered = 0;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  loadConfig();

  // HX711 initialisieren
  scale.begin(config.DT_PIN, config.SCK_PIN);
  Joystick.begin();
}

void loop()
{
  // 🔹 Serielle Befehle
  if (Serial.available())
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "LOAD"){
      loadConfig();
      config.debugMode = true; // Debug direkt aktivieren
    }
    else if (cmd == "SAVE")
      saveConfig();
    else if (cmd == "PRINT")
      printConfig();
    else if (cmd.startsWith("SET"))
    {
      int sep = cmd.indexOf(' ', 4);
      String key = cmd.substring(4, sep);
      long val = cmd.substring(sep + 1).toInt();

      if (key == "DebugMode")
        config.debugMode = (val != 0);
      else if (key == "clutchMin")
        config.clutchMin = val;
      else if (key == "clutchMax")
        config.clutchMax = val;
      else if (key == "throttleMin")
        config.throttleMin = val;
      else if (key == "throttleMax")
        config.throttleMax = val;
      else if (key == "brakeMin")
        config.brakeMin = val;
      else if (key == "brakeMax")
        config.brakeMax = val;
      else if (key == "deadzone")
        config.deadzone = val;
      else if (key == "pinClutch")
        config.pinClutch = val;
      else if (key == "pinThrottle")
        config.pinThrottle = val;
      else if (key == "DT_PIN")
        config.DT_PIN = val;
      else if (key == "SCK_PIN")
        config.SCK_PIN = val;
      else if (key == "brakeSmoothing")
        config.brakeSmoothing = val;

      Serial.print("✅ ");
      Serial.print(key);
      Serial.print(" = ");
      Serial.println(val);
    }
  }

  // 🔹 Rohwerte einlesen
  int clutchRaw = analogRead(config.pinClutch);
  int throttleRaw = analogRead(config.pinThrottle);
  long brakeRaw = scale.read();

  int clutch = map(constrain(clutchRaw, config.clutchMin, config.clutchMax), config.clutchMin, config.clutchMax, 0, 1023);
  int throttle = map(constrain(throttleRaw, config.throttleMin, config.throttleMax), config.throttleMin, config.throttleMax, 0, 1023);
  int brake = map(constrain((long)brakeRaw, config.brakeMin, config.brakeMax), config.brakeMin, config.brakeMax, 0, 1023);

  clutch = applyDeadzone(clutch, 0, config.deadzone);
  throttle = applyDeadzone(throttle, 0, config.deadzone);
  brake = applyDeadzone(brake, 0, config.deadzone);

  Joystick.setAccelerator(clutch);
  Joystick.setThrottle(throttle);
  Joystick.setBrake(brake);

  if (config.debugMode)
  {
    Serial.print(clutchRaw);
    Serial.print(',');
    Serial.print(throttleRaw);
    Serial.print(',');
    Serial.print(brakeRaw);
    Serial.print(',');
    Serial.print(clutch);
    Serial.print(',');
    Serial.print(throttle);
    Serial.print(',');
    Serial.println(brake);
  }

}
