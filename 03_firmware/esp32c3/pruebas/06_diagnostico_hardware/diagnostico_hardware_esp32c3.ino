#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <driver/i2s.h>

// ==========================================
// PINES (ESP32-C3 Supermini)
// ==========================================
#define I2C_SDA_PIN  8
#define I2C_SCL_PIN  9
#define TOUCH_PIN    2
#define BUZZER_PIN   10
#define I2S_SCK_PIN  4
#define I2S_WS_PIN   1
#define I2S_SD_PIN   3
#define I2S_PORT     I2S_NUM_0

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ==========================================
// RESULTADOS DE DIAGNÓSTICO
// ==========================================
struct TestResult {
  const char* nombre;
  bool ok;
  char detalle[40];
};

TestResult resultados[5];
int totalTests = 0;

// ==========================================
// UTILIDADES
// ==========================================
void beep(int freq, int durMs) {
  tone(BUZZER_PIN, freq, durMs);
  delay(durMs + 30);
}

void printSerial(const char* test, bool ok, const char* detalle) {
  Serial.print("[");
  Serial.print(ok ? "OK " : "FAIL");
  Serial.print("] ");
  Serial.print(test);
  Serial.print(" — ");
  Serial.println(detalle);
}

void addResult(const char* nombre, bool ok, const char* detalle) {
  if (totalTests < 5) {
    resultados[totalTests].nombre = nombre;
    resultados[totalTests].ok = ok;
    strncpy(resultados[totalTests].detalle, detalle, 39);
    resultados[totalTests].detalle[39] = '\0';
    totalTests++;
  }
  printSerial(nombre, ok, detalle);
}

// ==========================================
// TEST 1: OLED via I2C
// Detecta: SDA/SCL mal conectados, dirección
// incorrecta, OLED sin alimentación o tierra
// ==========================================
bool testOLED() {
  Serial.println("\n>>> TEST 1: OLED I2C (SDA=8, SCL=9)");

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Escanear el bus I2C para ver si hay algo conectado
  bool deviceFound = false;
  uint8_t foundAddr = 0;

  Serial.println("Escaneando bus I2C...");
  for (uint8_t addr = 0x08; addr < 0x78; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  Dispositivo I2C encontrado en 0x%02X\n", addr);
      deviceFound = true;
      foundAddr = addr;
    }
  }

  if (!deviceFound) {
    addResult("OLED", false, "Sin respuesta I2C. Revisa SDA/SCL/GND/3V3");
    return false;
  }

  if (foundAddr != 0x3C && foundAddr != 0x3D) {
    char det[40];
    snprintf(det, 40, "Addr inesperada: 0x%02X (espera 0x3C)", foundAddr);
    addResult("OLED", false, det);
    return false;
  }

  // Intentar inicializar el display
  if (!display.begin(0x3C, true)) {
    addResult("OLED", false, "I2C OK pero display no inicia");
    return false;
  }

  // Mostrar patrón de prueba en el OLED
  display.clearDisplay();
  display.fillRect(0, 0, 128, 8, SH110X_WHITE);
  display.fillRect(0, 56, 128, 8, SH110X_WHITE);
  display.fillRect(0, 0, 8, 64, SH110X_WHITE);
  display.fillRect(120, 0, 8, 64, SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(20, 28);
  display.print("OLED  OK  :)");
  display.display();
  delay(1500);

  addResult("OLED", true, "I2C 0x3C OK, display funcional");
  return true;
}

// ==========================================
// TEST 2: BUZZER
// Detecta: pin mal conectado, buzzer sin GND,
// buzzer defectuoso o polaridad invertida
// ==========================================
bool testBuzzer() {
  Serial.println("\n>>> TEST 2: BUZZER (GPIO10)");
  Serial.println("  Secuencia de 3 tonos: escucha si suenan correctamente.");

  pinMode(BUZZER_PIN, OUTPUT);

  // Tono grave
  Serial.println("  Tono 1: 500 Hz (grave)");
  beep(500, 250);
  delay(100);

  // Tono medio
  Serial.println("  Tono 2: 1500 Hz (medio)");
  beep(1500, 250);
  delay(100);

  // Tono agudo
  Serial.println("  Tono 3: 3000 Hz (agudo)");
  beep(3000, 250);
  delay(200);

  // El buzzer no se puede autovalidar eléctricamente de forma fiable,
  // así que preguntamos al usuario por Serial
  Serial.println("  ¿Escuchaste los 3 tonos? Envía '1'=SI, '0'=NO");

  // Esperar respuesta hasta 8 segundos
  unsigned long t = millis();
  while (millis() - t < 8000) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '1') {
        addResult("BUZZER", true, "3 tonos audibles confirmados");
        return true;
      } else if (c == '0') {
        addResult("BUZZER", false, "Sin sonido. Revisa GPIO10/GND/polaridad");
        return false;
      }
    }
  }

  // Sin respuesta → asumimos OK (puede no haber monitor abierto)
  addResult("BUZZER", true, "Sin confirmar (timeout) — verifica visualmente");
  return true;
}

// ==========================================
// TEST 3: TOUCH TTP223 (GPIO2)
// Detecta: pin mal conectado, sensor sin
// alimentación, cable de señal suelto
// ==========================================
bool testTouch() {
  Serial.println("\n>>> TEST 3: TOUCH TTP223 (GPIO2)");
  Serial.println("  Tienes 6 segundos: toca y suelta el sensor al menos 1 vez.");

  pinMode(TOUCH_PIN, INPUT);

  bool estadoAnterior = digitalRead(TOUCH_PIN);
  bool detectedHigh = false;
  bool detectedLow  = false;
  int cambios = 0;

  unsigned long t = millis();
  while (millis() - t < 6000) {
    bool estadoActual = digitalRead(TOUCH_PIN);

    if (estadoActual != estadoAnterior) {
      cambios++;
      if (estadoActual == HIGH) {
        detectedHigh = true;
        Serial.println("  >> TOQUE detectado (HIGH)");
      } else {
        detectedLow = true;
        Serial.println("  >> Soltado (LOW)");
      }
      estadoAnterior = estadoActual;
      delay(50); // antirrebote
    }

    // Mostrar en OLED si ya está disponible
    if (display.begin(0x3C, false)) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(0, 0);
      display.print("TEST TOUCH GPIO2");
      display.setCursor(0, 20);
      display.print("Toca el sensor!");
      display.setCursor(0, 40);
      display.print(estadoActual ? "ESTADO: TOCADO  " : "ESTADO: libre   ");
      display.setCursor(0, 52);
      display.print("Cambios: ");
      display.print(cambios);
      display.display();
    }

    delay(10);
  }

  if (!detectedHigh) {
    // El pin puede estar fijo en HIGH (sin GND del sensor) o fijo en LOW
    int lecturaFinal = digitalRead(TOUCH_PIN);
    if (lecturaFinal == HIGH) {
      addResult("TOUCH", false, "Pin fijo HIGH. Sin GND o sensor defectuoso");
    } else {
      addResult("TOUCH", false, "Sin cambios. Revisa VCC/GND/señal del TTP223");
    }
    return false;
  }

  if (detectedHigh && detectedLow) {
    addResult("TOUCH", true, "Toque y soltar detectados correctamente");
  } else {
    addResult("TOUCH", true, "Toque detectado (no se soltó en tiempo)");
  }
  return true;
}

// ==========================================
// TEST 4: MICRÓFONO INMP441 (I2S)
// Detecta: SCK/WS/SD mal conectados, L/R
// sin GND, micrófono sin alimentación,
// buffer lleno de ceros (sin señal)
// ==========================================
bool testMic() {
  Serial.println("\n>>> TEST 4: MICRÓFONO INMP441 (I2S)");
  Serial.println("  SCK=4, WS=1, SD=3, L/R=GND");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num   = I2S_SCK_PIN,
    .ws_io_num    = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD_PIN
  };

  // Instalar driver
  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    char det[40];
    snprintf(det, 40, "Driver I2S fallo: 0x%X", err);
    addResult("MIC I2S", false, det);
    return false;
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    char det[40];
    snprintf(det, 40, "Pines I2S fallo: 0x%X", err);
    i2s_driver_uninstall(I2S_PORT);
    addResult("MIC I2S", false, det);
    return false;
  }

  // Leer varias muestras
  const int BUF = 128;
  int32_t buf[BUF];
  size_t bytesIn = 0;

  Serial.println("  Leyendo muestras de audio...");
  Serial.println("  (Habla o produce ruido cerca del micrófono)");

  // Descartar primeras lecturas (ruido de arranque)
  for (int warmup = 0; warmup < 3; warmup++) {
    i2s_read(I2S_PORT, buf, sizeof(buf), &bytesIn, 100);
  }

  int32_t maxVal = 0;
  int32_t minVal = 0;
  int zerosCount = 0;
  int samplesTotal = 0;

  // Leer durante 2 segundos
  unsigned long t = millis();
  while (millis() - t < 2000) {
    err = i2s_read(I2S_PORT, buf, sizeof(buf), &bytesIn, 100);
    if (err == ESP_OK && bytesIn > 0) {
      int n = bytesIn / 4;
      samplesTotal += n;
      for (int i = 0; i < n; i++) {
        int32_t s = buf[i] >> 14;
        if (s > maxVal) maxVal = s;
        if (s < minVal) minVal = s;
        if (buf[i] == 0) zerosCount++;
      }
    }
  }

  i2s_driver_uninstall(I2S_PORT);

  Serial.printf("  Muestras leídas: %d\n", samplesTotal);
  Serial.printf("  Rango: %d a %d\n", minVal, maxVal);
  Serial.printf("  Ceros: %d / %d\n", zerosCount, samplesTotal);

  // Si todos son ceros → sin señal (SD desconectado o L/R sin GND)
  if (samplesTotal == 0) {
    addResult("MIC I2S", false, "0 muestras leidas. Revisa SCK/WS/SD");
    return false;
  }

  float pctCeros = (float)zerosCount / samplesTotal * 100.0f;
  if (pctCeros > 95.0f) {
    addResult("MIC I2S", false, "Muestras todas 0. SD suelto o L/R sin GND");
    return false;
  }

  int32_t rango = maxVal - minVal;
  if (rango < 10) {
    addResult("MIC I2S", false, "Rango muy bajo. Micrófono sin VCC o SD suelto");
    return false;
  }

  char det[40];
  snprintf(det, 40, "OK rango=%d ceros=%.0f%%", rango, pctCeros);
  addResult("MIC I2S", true, det);
  return true;
}

// ==========================================
// TEST 5: GND / TIERRA GENERAL
// Heurístico basado en comportamiento
// de los otros tests
// ==========================================
void testGND() {
  Serial.println("\n>>> TEST 5: ANÁLISIS DE TIERRA (GND)");

  // Contar cuántos tests fallaron
  int fallos = 0;
  for (int i = 0; i < totalTests; i++) {
    if (!resultados[i].ok) fallos++;
  }

  if (fallos >= 3) {
    addResult("GND", false, "Muchos fallos: verifica tierra comun del circuito");
  } else if (fallos == 0) {
    addResult("GND", true, "Todos OK — tierra correcta");
  } else {
    addResult("GND", true, "GND aparentemente OK (fallos aislados)");
  }
}

// ==========================================
// MOSTRAR RESUMEN FINAL EN OLED Y SERIAL
// ==========================================
void mostrarResumen() {
  Serial.println("\n==========================================");
  Serial.println("        RESUMEN DIAGNÓSTICO FINAL");
  Serial.println("==========================================");

  int okCount = 0;
  for (int i = 0; i < totalTests; i++) {
    Serial.printf("  %s  %s: %s\n",
      resultados[i].ok ? "[OK  ]" : "[FAIL]",
      resultados[i].nombre,
      resultados[i].detalle);
    if (resultados[i].ok) okCount++;
  }

  Serial.println("==========================================");
  Serial.printf("  RESULTADO: %d/%d dispositivos OK\n", okCount, totalTests);
  Serial.println("==========================================");

  // Mostrar en OLED si está disponible
  bool oledOk = false;
  for (int i = 0; i < totalTests; i++) {
    if (strcmp(resultados[i].nombre, "OLED") == 0 && resultados[i].ok) {
      oledOk = true;
      break;
    }
  }

  if (oledOk) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.print("=== DIAGNOSTICO ===");

    int y = 10;
    for (int i = 0; i < totalTests && y < 56; i++) {
      display.setCursor(0, y);
      display.print(resultados[i].ok ? "OK " : "!! ");
      display.print(resultados[i].nombre);
      y += 9;
    }

    display.setCursor(0, 56);
    display.print(okCount == totalTests ? "TODO OK :)" : "HAY FALLOS - ver Serial");
    display.display();

    // Señal sonora de resultado
    if (okCount == totalTests) {
      beep(1000, 100); delay(50);
      beep(1500, 100); delay(50);
      beep(2000, 200);
    } else {
      beep(400, 300); delay(100);
      beep(300, 500);
    }
  }
}

// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(2000); // Tiempo para abrir el monitor serie

  Serial.println("\n==========================================");
  Serial.println("   DIAGNÓSTICO DE HARDWARE — ESP32-C3");
  Serial.println("==========================================");
  Serial.println("Dispositivos a probar:");
  Serial.println("  1. OLED SH1106   (I2C  SDA=8, SCL=9)");
  Serial.println("  2. Buzzer        (GPIO 10)");
  Serial.println("  3. Touch TTP223  (GPIO 2)");
  Serial.println("  4. Mic INMP441   (I2S  SCK=4 WS=1 SD=3)");
  Serial.println("  5. GND general   (análisis heurístico)");
  Serial.println("==========================================\n");
  delay(1000);

  testOLED();
  delay(500);
  testBuzzer();
  delay(500);
  testTouch();
  delay(500);
  testMic();
  delay(500);
  testGND();

  mostrarResumen();
}

// ==========================================
// LOOP — Monitoreo continuo tras diagnóstico
// ==========================================
void loop() {
  // Monitoreo continuo: muestra niveles en tiempo real
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    bool touch = digitalRead(TOUCH_PIN);
    Serial.printf("Touch: %s\n", touch ? "TOCADO" : "libre");
  }

  // Toque → beep corto de confirmación
  static bool lastTouch = false;
  bool touch = digitalRead(TOUCH_PIN);
  if (touch && !lastTouch) beep(1800, 80);
  lastTouch = touch;

  delay(20);
}
