/*
  prueba_touch.ino
  Hardware: ESP32-C3 Super Mini + TTP223 + OLED SH1106 128×64

  Verifica el sensor táctil TTP223 mostrando en tiempo real:
    - Estado crudo (PRESIONADO / suelto)
    - Duración del toque en ms
    - Tipo de evento detectado: TAP simple, ráfaga (x2, x3+), HOLD largo
    - Contador total de eventos

  EVENTOS QUE DETECTA:
    Tap simple   → toque < 800ms, sin toques previos recientes
    Tap x2       → dos toques en menos de 400ms
    Tap x3+      → tres o más toques en menos de 400ms
    Hold largo   → toque mantenido ≥ 800ms (igual que en mochi_unified_5)

  Mismos umbrales que el firmware principal para que el comportamiento
  sea predecible al subir mochi_unified_5 después.

  PINES:
    Touch TTP223 = GPIO 2
    OLED SDA     = GPIO 8
    OLED SCL     = GPIO 9
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ── Pines ─────────────────────────────────────────────────────────────────
#define TOUCH_PIN   2
#define SDA_PIN     8
#define SCL_PIN     9
#define OLED_ADDR   0x3C

// ── Umbrales de tiempo (mismos que mochi_unified_5) ───────────────────────
#define HOLD_MS       800    // ms para considerar HOLD largo
#define MULTITAP_MS   400    // ventana para acumular taps consecutivos

// ── Display ───────────────────────────────────────────────────────────────
Adafruit_SH1106G display(128, 64, &Wire, -1);

// ── Estado del sensor ─────────────────────────────────────────────────────
bool     prevTouch     = false;
uint32_t pressStart    = 0;       // millis() al momento de presionar
uint32_t lastRelease   = 0;       // millis() del último soltar
int      tapContador   = 0;       // taps acumulados en la ventana actual
bool     esperandoMas  = false;   // true mientras la ventana multitap está abierta

// ── Para mostrar en pantalla ───────────────────────────────────────────────
String   ultimoEvento  = "---";
int      totalEventos  = 0;
bool     presionado    = false;
uint32_t duracionMs    = 0;

// ── Render del OLED ────────────────────────────────────────────────────────
void renderDisplay() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // ── Título ────────────────────────────────────────────────────────────
  display.setTextSize(1);
  display.setCursor(14, 0);
  display.print("PRUEBA TOUCH TTP223");

  // ── Línea separadora ──────────────────────────────────────────────────
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);

  // ── Último evento (texto grande) ─────────────────────────────────────
  display.setTextSize(2);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds(ultimoEvento.c_str(), 0, 0, &x1, &y1, &tw, &th);
  display.setCursor((128 - tw) / 2, 14);
  display.print(ultimoEvento);

  // ── Estado crudo del pin ──────────────────────────────────────────────
  display.setTextSize(1);
  display.setCursor(0, 38);
  if (presionado) {
    display.print("PRESIONADO  ");
    display.print(duracionMs);
    display.print("ms");
  } else {
    display.print("suelto");
  }

  // ── Línea separadora ──────────────────────────────────────────────────
  display.drawFastHLine(0, 48, 128, SH110X_WHITE);

  // ── Contador total ────────────────────────────────────────────────────
  display.setCursor(0, 52);
  display.print("Eventos: ");
  display.print(totalEventos);

  display.display();
}

// ── Emitir evento y actualizar pantalla ────────────────────────────────────
void emitirEvento(const String& nombre) {
  ultimoEvento = nombre;
  totalEventos++;
  Serial.printf("EVENTO  %-14s  total=%d\n", nombre.c_str(), totalEventos);
  renderDisplay();
}

// ── Procesar lógica del sensor ─────────────────────────────────────────────
void procesarTouch() {
  bool touched = digitalRead(TOUCH_PIN);
  uint32_t ahora = millis();

  // ── Flanco de bajada: soltó ───────────────────────────────────────────
  if (prevTouch && !touched) {
    duracionMs = ahora - pressStart;
    presionado = false;

    if (duracionMs >= HOLD_MS) {
      // Hold largo: emitir inmediatamente, no acumular con taps
      tapContador   = 0;
      esperandoMas  = false;
      emitirEvento("HOLD");
    } else {
      // Tap corto: acumular
      tapContador++;
      lastRelease  = ahora;
      esperandoMas = true;
    }
    renderDisplay();
  }

  // ── Flanco de subida: presionó ────────────────────────────────────────
  if (!prevTouch && touched) {
    pressStart = ahora;
    presionado = true;
    renderDisplay();
  }

  // ── Actualizar duración mientras está presionado ──────────────────────
  if (touched && presionado) {
    duracionMs = ahora - pressStart;
  }

  // ── Ventana multitap expiró: publicar N taps ──────────────────────────
  if (esperandoMas && !touched && (ahora - lastRelease) >= MULTITAP_MS) {
    esperandoMas = false;
    switch (tapContador) {
      case 1:  emitirEvento("TAP x1");  break;
      case 2:  emitirEvento("TAP x2");  break;
      default: emitirEvento("TAP x3+"); break;
    }
    tapContador = 0;
  }

  prevTouch = touched;
}

// ═══════════════════════════════════════════════════════════════════════════
//  setup / loop
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TOUCH_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA TOUCH — TTP223");
  Serial.println("========================================");
  Serial.printf("  Touch = GPIO %d\n", TOUCH_PIN);
  Serial.println("  Umbrales:");
  Serial.printf("    Hold largo  >= %d ms\n", HOLD_MS);
  Serial.printf("    Ventana tap <= %d ms\n", MULTITAP_MS);
  Serial.println("========================================");

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED no encontrado. Verifica SDA=8, SCL=9.");
    while (true) delay(1000);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(18, 10);
  display.print("PRUEBA TOUCH");
  display.setTextSize(2);
  display.setCursor(14, 28);
  display.print("Listo :)");
  display.setTextSize(1);
  display.setCursor(8, 52);
  display.print("Toca el sensor...");
  display.display();

  Serial.println("Listo. Toca el sensor y observa.");
  Serial.println();
  Serial.println("Evento          Total");
  Serial.println("--------------- -----");
}

void loop() {
  procesarTouch();
  delay(10);
}
