#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LittleFS.h>

// --- User config: ESP32-C6 SuperMini pin mapping ---
#define TOUCH_SENSOR_PIN 2    // TTP223 → GPIO 2
#define BUZZER_PIN       19   // Buzzer  → GPIO 19
#define I2C_SDA_PIN      6    // OLED SDA → GPIO 6
#define I2C_SCL_PIN      7    // OLED SCL → GPIO 7
// ---------------------------------------------------

#include "t-rex-duino.h"

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  Wire.setBufferSize(256); // fix: default 128 drops last column byte (1 prefix + 128 data = 129 > 128)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  pinMode(TOUCH_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  runDinoGame();
}
