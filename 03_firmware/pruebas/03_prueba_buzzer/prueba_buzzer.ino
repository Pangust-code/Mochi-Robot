/*
  prueba_buzzer.ino
  Hardware: ESP32-C6 Supermini + Buzzer pasivo + OLED SH1106 + TTP223

  Verifica el buzzer y reproduce los mismos tonos que usa mochi_unified_5,
  para que el estudiante los reconozca cuando suba el firmware completo.

  TESTS AUTOMÁTICOS (al encender):
    1. Sweep   — sube de 200 Hz a 3000 Hz y baja
                 Si el pitch no cambia → probablemente es buzzer ACTIVO (no sirve)
    2. Escala  — Do Re Mi Fa Sol La Si Do (confirmación auditiva de tonos)
    3. Tonos del firmware — los 3 tonos de feedback que usa mochi_unified_5:
         · Tap de ánimo    (800 Hz, 60 ms)
         · Cambio de modo  (1000 Hz, 80 ms)
         · Pomodoro listo  (880 → 1046 Hz)

  MODO INTERACTIVO (después de los tests):
    TAP x1   → tono de tap  (igual que al cambiar ánimo en el robot)
    TAP x3+  → tema de Tetris (igual que al dar 3 taps en modo Mascota)
    HOLD     → tono de cambio de modo (igual que al mantener el sensor)

  PINES:
    Buzzer   = GPIO 19
    Touch    = GPIO 2
    OLED SDA = GPIO 6  /  SCL = GPIO 7
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ── Pines ─────────────────────────────────────────────────────────────────
#define BUZZER_PIN  19
#define TOUCH_PIN    2
#define SDA_PIN      6
#define SCL_PIN      7
#define OLED_ADDR  0x3C

// ── Umbrales touch (mismos que mochi_unified_5) ───────────────────────────
#define HOLD_MS      800
#define MULTITAP_MS  400

// ── Display ───────────────────────────────────────────────────────────────
Adafruit_SH1106G display(128, 64, &Wire, -1);

// ── Notas musicales (mismas que el firmware) ──────────────────────────────
const int A4=440, B4=494, C5=523, D5=587, E5=659, F5=698, G5=784, A5=880;

// ── Estado del OLED ───────────────────────────────────────────────────────
String lineaTitulo  = "";
String lineaDetalle = "";

void mostrar(const String& titulo, const String& detalle = "") {
  lineaTitulo  = titulo;
  lineaDetalle = detalle;

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(20, 0);
  display.print("PRUEBA BUZZER");
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print(lineaTitulo);

  display.setTextSize(2);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds(lineaDetalle.c_str(), 0, 0, &x1, &y1, &tw, &th);
  display.setCursor((128 - tw) / 2, 28);
  display.print(lineaDetalle);

  display.setTextSize(1);
  display.setCursor(0, 55);
  display.print("x1=tono x3=Tetris H=modo");

  display.display();
}

// ═══════════════════════════════════════════════════════════════════════════
//  Funciones de sonido (igual que mochi_unified_5)
// ═══════════════════════════════════════════════════════════════════════════

void nota(int freq, int dur) {
  if (freq == 0) delay(dur);
  else { tone(BUZZER_PIN, freq, (int)(dur * 0.9f)); delay(dur); }
}

void tonoTap() {
  tone(BUZZER_PIN, 800, 60);
  mostrar("TAP x1", "800Hz");
  delay(150);
}

void tonoModo() {
  tone(BUZZER_PIN, 1000, 80);
  mostrar("HOLD", "1000Hz");
  delay(150);
}

void tonoPomodoro() {
  mostrar("Pomodoro", "880Hz");
  tone(BUZZER_PIN, 880, 200); delay(250);
  mostrar("Pomodoro", "1046Hz");
  tone(BUZZER_PIN, 1046, 500); delay(550);
}

void tetrisTheme() {
  int mel[] = {
    E5,250, B4,125, C5,125, D5,250, C5,125, B4,125,
    A4,250, A4,125, C5,125, E5,250, D5,125, C5,125,
    B4,375, C5,125, D5,250, E5,250, C5,250, A4,250, A4,250, 0,250,
    D5,375, F5,125, A5,250, G5,125, F5,125,
    E5,375, C5,125, E5,250, D5,125, C5,125,
    B4,250, B4,125, C5,125, D5,250, E5,250, C5,250, A4,250, A4,250, 0,250
  };
  int n = sizeof(mel) / sizeof(mel[0]) / 2;
  float spd = 0.9f;
  for (int v = 0; v < 3; v++) {
    for (int i = 0; i < n; i++) {
      nota(mel[i * 2], (int)(mel[i * 2 + 1] * spd));
      // Muestra la nota actual en la pantalla
      display.fillRect(0, 28, 128, 20, SH110X_BLACK);
      display.setTextSize(2);
      display.setCursor(30, 28);
      display.printf("%d Hz", mel[i * 2]);
      display.display();
    }
    spd *= 0.75f;  // cada vuelta un poco más rápido (igual que el robot)
  }
}

// ═══════════════════════════════════════════════════════════════════════════
//  Tests automáticos
// ═══════════════════════════════════════════════════════════════════════════

void testSweep() {
  Serial.println("[TEST 1] Sweep de frecuencias (200 Hz → 3000 Hz → 200 Hz)");
  Serial.println("         Si el tono no cambia de pitch: es buzzer ACTIVO, no sirve.");

  mostrar("Test 1: Sweep", "subiendo...");
  for (int f = 200; f <= 3000; f += 50) {
    tone(BUZZER_PIN, f);
    display.fillRect(20, 28, 90, 18, SH110X_BLACK);
    display.setTextSize(2);
    display.setCursor(20, 28);
    display.printf("%4dHz", f);
    display.display();
    delay(18);
  }
  mostrar("Test 1: Sweep", "bajando...");
  for (int f = 3000; f >= 200; f -= 50) {
    tone(BUZZER_PIN, f);
    display.fillRect(20, 28, 90, 18, SH110X_BLACK);
    display.setTextSize(2);
    display.setCursor(20, 28);
    display.printf("%4dHz", f);
    display.display();
    delay(18);
  }
  noTone(BUZZER_PIN);
  mostrar("Test 1: Sweep", "OK");
  Serial.println("         Sweep completo.");
  delay(600);
}

void testEscala() {
  Serial.println("[TEST 2] Escala musical (Do Re Mi Fa Sol La Si Do)");

  const char* nombres[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI", "DO"};
  const int   freqs[]   = { 262,  294,  330,  349,  392,  440,  494,  523};

  for (int i = 0; i < 8; i++) {
    Serial.printf("         %s  %d Hz\n", nombres[i], freqs[i]);
    mostrar("Test 2: Escala", nombres[i]);
    nota(freqs[i], 300);
    delay(80);
  }
  mostrar("Test 2: Escala", "OK");
  Serial.println("         Escala completa.");
  delay(500);
}

void testTonosFirmware() {
  Serial.println("[TEST 3] Tonos del firmware mochi_unified_5");

  Serial.println("         Tono tap de animo (800 Hz, 60 ms)...");
  mostrar("Test 3: Firmware", "tap animo");
  tone(BUZZER_PIN, 800, 60); delay(400);

  Serial.println("         Tono cambio de modo (1000 Hz, 80 ms)...");
  mostrar("Test 3: Firmware", "cambio modo");
  tone(BUZZER_PIN, 1000, 80); delay(400);

  Serial.println("         Tono Pomodoro listo (880 + 1046 Hz)...");
  mostrar("Test 3: Firmware", "pomodoro");
  tone(BUZZER_PIN, 880, 200); delay(250);
  tone(BUZZER_PIN, 1046, 500); delay(600);

  mostrar("Test 3: Firmware", "OK");
  Serial.println("         Tonos del firmware completos.");
  delay(500);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Modo interactivo — touch
// ═══════════════════════════════════════════════════════════════════════════

bool     prevTouch   = false;
uint32_t pressStart  = 0;
uint32_t lastRelease = 0;
int      tapCount    = 0;
bool     waitingMore = false;

void procesarTouch() {
  bool touched = digitalRead(TOUCH_PIN);
  uint32_t now = millis();

  if (prevTouch && !touched) {
    uint32_t dur = now - pressStart;
    if (dur >= HOLD_MS) {
      tapCount    = 0;
      waitingMore = false;
      Serial.println("INTERACTIVO  HOLD → tono cambio de modo");
      tonoModo();
    } else {
      tapCount++;
      lastRelease = now;
      waitingMore = true;
    }
  }

  if (!prevTouch && touched) {
    pressStart = now;
  }

  if (waitingMore && !touched && (now - lastRelease) >= MULTITAP_MS) {
    waitingMore = false;
    if (tapCount == 1) {
      Serial.println("INTERACTIVO  TAP x1 → tono tap");
      tonoTap();
    } else if (tapCount == 2) {
      Serial.println("INTERACTIVO  TAP x2 → tono tap x2");
      tonoTap(); delay(100); tonoTap();
    } else {
      Serial.println("INTERACTIVO  TAP x3+ → Tetris theme");
      mostrar("TAP x3+", "Tetris!");
      tetrisTheme();
    }
    tapCount = 0;
    mostrar("Interactivo", "Listo");
  }

  prevTouch = touched;
}

// ═══════════════════════════════════════════════════════════════════════════
//  setup / loop
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED no encontrado.");
    while (true) delay(1000);
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA BUZZER — pasivo GPIO 19");
  Serial.println("========================================");
  Serial.println("  Si el tono no cambia en el sweep:");
  Serial.println("  -> tu buzzer es ACTIVO, debes cambiarlo.");
  Serial.println("========================================");
  Serial.println();

  mostrar("Iniciando...", "");
  delay(800);

  testSweep();
  testEscala();
  testTonosFirmware();

  Serial.println();
  Serial.println("Tests automaticos completos.");
  Serial.println("Modo interactivo:");
  Serial.println("  TAP x1  → tono tap");
  Serial.println("  TAP x3+ → Tetris theme");
  Serial.println("  HOLD    → tono cambio de modo");

  mostrar("Interactivo", "Listo");
}

void loop() {
  procesarTouch();
  delay(10);
}
