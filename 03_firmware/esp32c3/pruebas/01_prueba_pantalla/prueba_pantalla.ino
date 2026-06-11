/*
  prueba_pantalla.ino
  Hardware: ESP32-C3 Super Mini + OLED SH1106 128×64

  Verifica que la pantalla esté correctamente conectada y funcione.
  Ejecuta una serie de tests visuales en orden automático.

  TESTS:
    1. Scanner I2C — confirma que el display responde en el bus
    2. Texto básico — "PANTALLA OK" en pantalla
    3. Tamaños de texto — 3 tamaños disponibles (1, 2, 3)
    4. Formas — rectángulo, círculo, líneas diagonales
    5. Píxeles — pantalla completa encendida (todos los píxeles ON)
    6. Inversión — prueba contraste con invertDisplay()
    7. Animación — barra que crece de izquierda a derecha

  Si la pantalla no aparece en el test 1 (Scanner I2C):
    → Verifica SDA = GPIO 8  y  SCL = GPIO 9
    → Verifica que la pantalla recibe 3.3V
    → Verifica la dirección I2C: la SH1106 puede estar en 0x3C o 0x3D

  LIBRERÍAS necesarias:
    Adafruit GFX Library
    Adafruit SH110X

  PINES:
    SDA = GPIO 8
    SCL = GPIO 9
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SDA_PIN       8
#define SCL_PIN       9
#define OLED_ADDR  0x3C
#define ANCHO        128
#define ALTO          64

Adafruit_SH1106G display(ANCHO, ALTO, &Wire, -1);

// ── Utilidades ─────────────────────────────────────────────────────────────

void titulo(const char* txt) {
  Serial.printf("\n[TEST] %s\n", txt);
}

void esperar(int ms, const char* motivo = nullptr) {
  if (motivo) Serial.printf("      (%s)\n", motivo);
  delay(ms);
}

// ── Test 1 — Scanner I2C ───────────────────────────────────────────────────

void testScannerI2C() {
  titulo("Scanner I2C");
  Serial.println("      Buscando dispositivos en el bus I2C...");

  int encontrados = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("      Dispositivo encontrado en 0x%02X", addr);
      if      (addr == 0x3C || addr == 0x3D) Serial.print("  ← OLED SH1106 ✓");
      else if (addr == 0x68 || addr == 0x69) Serial.print("  ← posible IMU");
      Serial.println();
      encontrados++;
    }
  }

  if (encontrados == 0) {
    Serial.println("      ERROR: no se encontró ningún dispositivo I2C.");
    Serial.println("      Verifica SDA=GPIO8, SCL=GPIO9 y alimentación 3.3V.");
  } else {
    Serial.printf("      Total: %d dispositivo(s) encontrado(s).\n", encontrados);
  }
}

// ── Test 2 — Texto básico ──────────────────────────────────────────────────

void testTextoBasico() {
  titulo("Texto basico");
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(14, 10);
  display.print("PANTALLA");
  display.setCursor(32, 34);
  display.print("OK :)");
  display.display();
  Serial.println("      Deberias ver: PANTALLA / OK :)");
  esperar(2500);
}

// ── Test 3 — Tamaños de texto ──────────────────────────────────────────────

void testTamanosTexto() {
  titulo("Tamanos de texto (1, 2, 3)");
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Tamano 1 - abcdefghijklmnop");

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print("Tamano 2");

  display.setTextSize(3);
  display.setCursor(0, 40);
  display.print("TAM 3");

  display.display();
  Serial.println("      Deberias ver 3 lineas con texto de distinto tamano.");
  esperar(2500);
}

// ── Test 4 — Formas geométricas ────────────────────────────────────────────

void testFormas() {
  titulo("Formas geometricas");
  display.clearDisplay();

  // Rectángulo sin relleno
  display.drawRect(0, 0, 60, 30, SH110X_WHITE);
  display.setCursor(5, 10);
  display.setTextSize(1);
  display.print("rect");

  // Rectángulo relleno
  display.fillRect(68, 0, 58, 30, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(72, 10);
  display.print("filled");
  display.setTextColor(SH110X_WHITE);

  // Círculo
  display.drawCircle(30, 50, 12, SH110X_WHITE);

  // Círculo relleno
  display.fillCircle(97, 50, 12, SH110X_WHITE);

  // Líneas diagonales de prueba
  display.drawLine(62, 34, 62, 64, SH110X_WHITE);  // separador vertical

  display.display();
  Serial.println("      Deberias ver: rect vacio, rect relleno, circulo, circulo relleno.");
  esperar(2500);
}

// ── Test 5 — Todos los píxeles ON ──────────────────────────────────────────

void testPixelesOn() {
  titulo("Todos los pixeles encendidos");
  display.clearDisplay();
  // Llenar todo el buffer de pantalla con blanco
  for (int y = 0; y < ALTO; y++) {
    for (int x = 0; x < ANCHO; x++) {
      display.drawPixel(x, y, SH110X_WHITE);
    }
  }
  display.display();
  Serial.println("      Deberias ver la pantalla completamente blanca.");
  Serial.println("      Si hay pixeles muertos aparecen como puntos negros.");
  esperar(2500);
}

// ── Test 6 — Inversión ─────────────────────────────────────────────────────

void testInversion() {
  titulo("Inversion de colores (invertDisplay)");

  // Texto de referencia
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.print("NORMAL");
  display.display();
  esperar(1200);

  // Invertir
  display.invertDisplay(true);
  display.setCursor(10, 20);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(8, 20);
  display.print("INVERSO");
  display.display();
  Serial.println("      Deberias ver texto negro sobre fondo blanco.");
  esperar(1500);

  // Volver a normal
  display.invertDisplay(false);
  esperar(300);
}

// ── Test 7 — Animación de barra ────────────────────────────────────────────

void testAnimacionBarra() {
  titulo("Animacion: barra de carga");
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 8);
  display.print("Cargando...");
  display.drawRect(4, 28, 120, 14, SH110X_WHITE);
  display.display();

  for (int w = 0; w <= 116; w += 4) {
    display.fillRect(5, 29, w, 12, SH110X_WHITE);
    // Porcentaje
    display.fillRect(50, 46, 40, 10, SH110X_BLACK);
    display.setCursor(50, 46);
    display.printf("%d%%", (int)map(w, 0, 116, 0, 100));
    display.display();
    delay(50);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 22);
  display.print("LISTO!");
  display.display();
  Serial.println("      Animacion completa.");
  esperar(1500);
}

// ═══════════════════════════════════════════════════════════════════════════
//  setup / loop
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA DE PANTALLA — SH1106 OLED");
  Serial.println("========================================");
  Serial.printf("  SDA=GPIO%d  SCL=GPIO%d  Dir=0x%02X\n", SDA_PIN, SCL_PIN, OLED_ADDR);
  Serial.println("========================================");

  // ── Test 1: Scanner I2C (sin display todavía) ───────────────────────────
  testScannerI2C();

  // ── Inicializar display ─────────────────────────────────────────────────
  Serial.println("\n[INIT] Inicializando display...");
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("  ERROR: display.begin() falló.");
    Serial.println("  Verifica la dirección I2C (0x3C o 0x3D) y el cableado.");
    Serial.println("  El sketch se detiene aquí.");
    while (true) delay(1000);
  }
  Serial.println("  display.begin() OK\n");

  display.setContrast(255);
  display.clearDisplay();
  display.display();

  // ── Ejecutar tests visuales ─────────────────────────────────────────────
  testTextoBasico();
  testTamanosTexto();
  testFormas();
  testPixelesOn();
  testInversion();
  testAnimacionBarra();

  // ── Resultado final ─────────────────────────────────────────────────────
  Serial.println("\n========================================");
  Serial.println("  Todos los tests completados.");
  Serial.println("  Si viste todos los graficos: pantalla OK.");
  Serial.println("========================================");

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(16, 20);
  display.print("Tests completados");
  display.setCursor(28, 36);
  display.print("Pantalla OK!");
  display.display();
}

void loop() {
  // Nada — todos los tests corren en setup()
}
