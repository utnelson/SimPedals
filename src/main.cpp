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

  // 🔹 Glättungsfaktoren (0.0 ... 1.0)
  float alphaClutch;
  float alphaThrottle;
  float alphaBrake;
} config;

// 🔧 Defaultwerte
Config defaultConfig = {
    0, 800,      // clutchMin, clutchMax
    0, 800,      // throttleMin, throttleMax
    0L, 200000L, // brakeMin, brakeMax
    20,          // deadzone
    A0,          // pinClutch
    A1,          // pinThrottle
    5,           // DT_PIN
    4,           // SCK_PIN
    0.1f,        // alphaClutch
    0.15f,       // alphaThrottle
    0.05f        // alphaBrake
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
  Serial.print(config.clutchMin);
  Serial.print(",");
  Serial.print(config.clutchMax);
  Serial.print(",");
  Serial.print(config.throttleMin);
  Serial.print(",");
  Serial.print(config.throttleMax);
  Serial.print(",");
  Serial.print(config.brakeMin);
  Serial.print(",");
  Serial.print(config.brakeMax);
  Serial.print(",");
  Serial.print(config.deadzone);
  Serial.print(",");
  Serial.print(config.pinClutch);
  Serial.print(",");
  Serial.print(config.pinThrottle);
  Serial.print(",");
  Serial.print(config.DT_PIN);
  Serial.print(",");
  Serial.print(config.SCK_PIN);
  Serial.print(",");
  Serial.print(config.alphaClutch, 3);
  Serial.print(",");
  Serial.print(config.alphaThrottle, 3);
  Serial.print(",");
  Serial.println(config.alphaBrake, 3);
}

int applyDeadzone(int value, int center, int width)
{
  if (abs(value - center) < width)
    return center;
  return value;
}

// 🔹 Glättungsspeicher
float clutchFiltered = 0;
float throttleFiltered = 0;
float brakeFiltered = 0;

unsigned long lastPrint = 0;
unsigned long lastHX711 = 0;
long brakeRaw = 0;

void setup()
{
  Serial.begin(115200);
  loadConfig();

  // HX711 initialisieren
  scale.begin(config.DT_PIN, config.SCK_PIN);
  Joystick.begin();
}

void loop()
{
    unsigned long now = millis();

    // 🔹 Serielle Befehle
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "LOAD")
        {
            loadConfig();
            scale.begin(config.DT_PIN, config.SCK_PIN);
        }
        else if (cmd == "SAVE")
            saveConfig();
        else if (cmd == "RESETCONFIG")
        {
            config = defaultConfig;
            EEPROM.write(MAGIC_BYTE_ADDR, MAGIC_BYTE_VAL);
            EEPROM.put(MAGIC_BYTE_ADDR + 1, config);
            scale.begin(config.DT_PIN, config.SCK_PIN);
            Serial.println("♻️ EEPROM auf DefaultConfig zurückgesetzt!");
            printConfig();
        }
        else if (cmd.startsWith("SET"))
        {
            int sep = cmd.indexOf(' ', 4);
            String key = cmd.substring(4, sep);
            String valStr = cmd.substring(sep + 1);
            long val = valStr.toInt();
            float fval = valStr.toFloat();

            if (key == "clutchMin") config.clutchMin = val;
            else if (key == "clutchMax") config.clutchMax = val;
            else if (key == "throttleMin") config.throttleMin = val;
            else if (key == "throttleMax") config.throttleMax = val;
            else if (key == "brakeMin") config.brakeMin = val;
            else if (key == "brakeMax") config.brakeMax = val;
            else if (key == "deadzone") config.deadzone = val;
            else if (key == "pinClutch") config.pinClutch = val;
            else if (key == "pinThrottle") config.pinThrottle = val;
            else if (key == "DT_PIN") { config.DT_PIN = val; scale.begin(config.DT_PIN, config.SCK_PIN); }
            else if (key == "SCK_PIN") { config.SCK_PIN = val; scale.begin(config.DT_PIN, config.SCK_PIN); }
            else if (key == "alphaClutch") config.alphaClutch = fval;
            else if (key == "alphaThrottle") config.alphaThrottle = fval;
            else if (key == "alphaBrake") config.alphaBrake = fval;

            Serial.print("✅ "); Serial.print(key); Serial.print(" = "); Serial.println(valStr);
        }
    }

    // 🔹 HX711 alle 20ms lesen (non-blocking)
    if (now - lastHX711 > 20)
    {
        lastHX711 = now;
        if (scale.is_ready())
            brakeRaw = scale.read();
    }

    // 🔹 Rohwerte einlesen
    int clutchRaw = analogRead(config.pinClutch);
    int throttleRaw = analogRead(config.pinThrottle);

    int clutch = map(constrain(clutchRaw, config.clutchMin, config.clutchMax), config.clutchMin, config.clutchMax, 0, 1023);
    int throttle = map(constrain(throttleRaw, config.throttleMin, config.throttleMax), config.throttleMin, config.throttleMax, 0, 1023);
    int brake = map(constrain(brakeRaw, config.brakeMin, config.brakeMax), config.brakeMin, config.brakeMax, 0, 1023);

    // 🔹 Filter initialisieren, falls noch 0
    if (clutchFiltered == 0) clutchFiltered = clutch;
    if (throttleFiltered == 0) throttleFiltered = throttle;
    if (brakeFiltered == 0) brakeFiltered = brake;

    // 🔹 Exponentielle Glättung
    clutchFiltered   = config.alphaClutch   * clutch   + (1 - config.alphaClutch)   * clutchFiltered;
    throttleFiltered = config.alphaThrottle * throttle + (1 - config.alphaThrottle) * throttleFiltered;
    brakeFiltered    = config.alphaBrake    * brake    + (1 - config.alphaBrake)    * brakeFiltered;

    // 🔹 Deadzone nach Filterung anwenden
    clutchFiltered   = applyDeadzone(clutchFiltered, 0, config.deadzone);
    throttleFiltered = applyDeadzone(throttleFiltered, 0, config.deadzone);
    brakeFiltered    = applyDeadzone(brakeFiltered, 0, config.deadzone);

    // 🔹 An Joystick senden
    Joystick.setAccelerator((int)clutchFiltered);
    Joystick.setThrottle((int)throttleFiltered);
    Joystick.setBrake((int)brakeFiltered);

    // 🔹 Serial-Ausgabe alle 20ms
    if (now - lastPrint > 20)
    {
        lastPrint = now;
        Serial.print(clutchRaw); Serial.print(',');
        Serial.print(throttleRaw); Serial.print(',');
        Serial.print(brakeRaw); Serial.print(',');
        Serial.print((int)clutchFiltered); Serial.print(',');
        Serial.print((int)throttleFiltered); Serial.print(',');
        Serial.println((int)brakeFiltered);
    }
}
