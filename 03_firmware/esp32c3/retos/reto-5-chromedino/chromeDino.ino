#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LittleFS.h>

// --- User config: ESP32-C3 Super Mini pin mapping ---
#define TOUCH_SENSOR_PIN 2    // TTP223 → GPIO 2
#define BUZZER_PIN       10   // Buzzer  → GPIO 10
#define I2C_SDA_PIN      8    // OLED SDA → GPIO 8
#define I2C_SCL_PIN      9    // OLED SCL → GPIO 9
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
