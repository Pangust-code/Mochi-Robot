/*
  prueba_littlefs.ino
  Hardware: ESP32-C6 Supermini + OLED SH1106 + TTP223 + Buzzer

  Verifica que LittleFS esté montado y que los archivos .bin de animación
  se puedan leer y reproducir correctamente.

  SECUENCIA AUTOMÁTICA:
    1. Monta LittleFS y reporta espacio total / usado
    2. Lista todos los archivos .bin encontrados (nombre, tamaño, frames)
    3. Reproduce cada animación en orden, mostrando el nombre en pantalla

  CONTROLES TOUCH:
    TAP x1   → salta a la siguiente animación
    TAP x3+  → vuelve a la primera animación
    HOLD     → pausa / reanuda la reproducción

  PROBLEMAS COMUNES:
    "LittleFS ERROR"  → los .bin no han sido subidos al ESP32.
                        Sube la carpeta data/ con el plugin LittleFS.
    "0 archivos"      → el filesystem se montó pero está vacío.
    Frame count = 0   → el archivo existe pero está vacío o corrupto.

  PINES:
    OLED SDA = GPIO 6  /  SCL = GPIO 7
    Touch    = GPIO 2
    Buzzer   = GPIO 19

  LIBRERÍAS:
    Adafruit GFX, Adafruit SH110X, U8g2lib, LittleFS
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <U8g2lib.h>
#include <LittleFS.h>

// ── Pines ─────────────────────────────────────────────────────────────────
#define SDA_PIN     6
#define SCL_PIN     7
#define OLED_ADDR   0x3C
#define TOUCH_PIN   2
#define BUZZER_PIN  19

// ── Parámetros de reproducción (mismos que mochi_unified_5) ───────────────
#define FRAME_SIZE  1024   // 128×64 / 8 bytes
#define FPS           20

// ── Umbrales touch ────────────────────────────────────────────────────────
#define HOLD_MS      800
#define MULTITAP_MS  400

// ── Display ───────────────────────────────────────────────────────────────
Adafruit_SH1106G  display(128, 64, &Wire, -1);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

bool u8g2Activo = false;

void activarU8g2() {
  if (u8g2Activo) return;
  u8g2.begin();
  u8g2.setI2CAddress(OLED_ADDR * 2);
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  u8g2Activo = true;
}

void activarAdafruit() {
  if (!u8g2Activo) return;
  display.begin(OLED_ADDR, true);
  display.clearDisplay();
  display.display();
  display.setTextColor(SH110X_WHITE);
  u8g2Activo = false;
}

// ── Buffer de frame ───────────────────────────────────────────────────────
static uint8_t frameBuffer[FRAME_SIZE];

// ── Touch ─────────────────────────────────────────────────────────────────
bool     prevTouch   = false;
uint32_t pressStart  = 0;
uint32_t lastRelease = 0;
int      tapCount    = 0;
bool     waitingMore = false;

// Eventos de touch (procesados en reproducción)
volatile bool ev_siguiente = false;
volatile bool ev_reiniciar = false;
volatile bool ev_pausa     = false;

void procesarTouch() {
  bool touched = digitalRead(TOUCH_PIN);
  uint32_t now = millis();

  if (prevTouch && !touched) {
    uint32_t dur = now - pressStart;
    if (dur >= HOLD_MS) {
      tapCount    = 0;
      waitingMore = false;
      ev_pausa    = true;
    } else {
      tapCount++;
      lastRelease = now;
      waitingMore = true;
    }
  }
  if (!prevTouch && touched) pressStart = now;

  if (waitingMore && !touched && (now - lastRelease) >= MULTITAP_MS) {
    waitingMore = false;
    if (tapCount == 1)  ev_siguiente = true;
    else                ev_reiniciar = true;
    tapCount = 0;
  }

  prevTouch = touched;
}

// ═══════════════════════════════════════════════════════════════════════════
//  Pantallas informativas (Adafruit)
// ═══════════════════════════════════════════════════════════════════════════

void mostrarError(const char* linea1, const char* linea2 = "") {
  activarAdafruit();
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 0);   display.print("LITTLEFS");
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 18);   display.print(linea1);
  display.setCursor(0, 30);   display.print(linea2);
  display.setCursor(0, 50);   display.print("Ver README para solucion.");
  display.display();
}

void mostrarInfo(int total, size_t usado, size_t disponible) {
  activarAdafruit();
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 0);   display.print("PRUEBA LITTLEFS");
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  display.setCursor(0, 14);   display.printf("Archivos .bin: %d", total);
  display.setCursor(0, 26);   display.printf("Usado:  %u KB", (unsigned)(usado / 1024));
  display.setCursor(0, 38);   display.printf("Libre:  %u KB", (unsigned)(disponible / 1024));
  display.setCursor(0, 54);   display.print("Iniciando reproduccion...");
  display.display();
}

void mostrarNombreAnim(const char* nombre, int frameCount, int idx, int total) {
  activarAdafruit();
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 0);   display.print("PRUEBA LITTLEFS");
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 14);   display.print(nombre);
  display.setCursor(0, 26);   display.printf("%d frames  (%d KB)", frameCount, (frameCount * FRAME_SIZE) / 1024);
  display.setCursor(0, 40);   display.printf("Anim %d / %d", idx + 1, total);
  display.setCursor(0, 54);   display.print("x1=sig  x3=inicio  H=pausa");
  display.display();
  delay(1200);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Reproducción de animación (U8g2 — igual que mochi_unified_5)
// ═══════════════════════════════════════════════════════════════════════════

// Devuelve true si terminó normalmente, false si se interrumpió con touch
bool reproducirAnimacion(const char* path) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.printf("  [ERROR] No se pudo abrir: %s\n", path);
    return true;
  }

  int totalFrames = (int)(f.size() / FRAME_SIZE);
  int frameDelay  = 1000 / FPS;

  activarU8g2();

  for (int i = 0; i < totalFrames; i++) {
    procesarTouch();

    if (ev_siguiente || ev_reiniciar) {
      f.close();
      return false;
    }

    // Pausa: espera hasta otro hold
    while (ev_pausa) {
      ev_pausa = false;
      activarAdafruit();
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(2);
      display.setCursor(28, 22);
      display.print("PAUSA");
      display.setTextSize(1);
      display.setCursor(16, 48);
      display.print("HOLD = reanudar");
      display.display();

      while (!ev_pausa) { procesarTouch(); delay(20); }
      ev_pausa = false;
      activarU8g2();
    }

    if (f.read(frameBuffer, FRAME_SIZE) < FRAME_SIZE) break;
    memcpy(u8g2.getBufferPtr(), frameBuffer, FRAME_SIZE);
    u8g2.sendBuffer();

    delay(frameDelay);
  }

  f.close();
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  setup / loop
// ═══════════════════════════════════════════════════════════════════════════

// Lista de animaciones encontradas
String archivos[32];
int    totalArchivos = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED no encontrado.");
    while (true) delay(1000);
  }
  u8g2.begin();
  u8g2.setI2CAddress(OLED_ADDR * 2);
  activarAdafruit();

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA LITTLEFS");
  Serial.println("========================================");

  // ── Montar LittleFS ─────────────────────────────────────────────────────
  Serial.println("  Montando LittleFS...");
  if (!LittleFS.begin(true)) {
    Serial.println("  ERROR: LittleFS no se pudo montar.");
    Serial.println("  Sube la carpeta data/ con el plugin LittleFS.");
    mostrarError("LittleFS ERROR", "Sube data/ al ESP32.");
    tone(BUZZER_PIN, 300, 500);
    while (true) delay(1000);
  }

  size_t usado      = LittleFS.usedBytes();
  size_t disponible = LittleFS.totalBytes() - usado;
  Serial.printf("  LittleFS OK — Usado: %u KB / Total: %u KB\n",
    (unsigned)(usado / 1024), (unsigned)(LittleFS.totalBytes() / 1024));

  // ── Listar archivos .bin ─────────────────────────────────────────────────
  Serial.println("  Archivos encontrados:");
  File root = LittleFS.open("/");
  File f    = root.openNextFile();
  while (f && totalArchivos < 32) {
    String nombre = String(f.name());
    if (nombre.endsWith(".bin")) {
      int frames = (int)(f.size() / FRAME_SIZE);
      int resto  = (int)(f.size() % FRAME_SIZE);
      archivos[totalArchivos++] = nombre;
      Serial.printf("    %-22s  %3d frames  %5u KB%s\n",
        nombre.c_str(), frames, (unsigned)(f.size() / 1024),
        resto ? "  (tamaño no multiplo de 1024!)" : "");
    }
    f = root.openNextFile();
  }

  if (totalArchivos == 0) {
    Serial.println("  Sin archivos .bin. Sube data/ primero.");
    mostrarError("Sin archivos .bin", "Sube data/ al ESP32.");
    tone(BUZZER_PIN, 400, 300); delay(350); tone(BUZZER_PIN, 300, 500);
    while (true) delay(1000);
  }

  Serial.printf("  Total: %d archivo(s)\n", totalArchivos);
  Serial.println("========================================");
  Serial.println("  x1=siguiente  x3=reiniciar  HOLD=pausa");
  Serial.println("========================================");
  Serial.println();

  mostrarInfo(totalArchivos, usado, disponible);
  tone(BUZZER_PIN, 1046, 100); delay(150); tone(BUZZER_PIN, 1200, 200);
  delay(2000);
}

int animActual = 0;

void loop() {
  procesarTouch();

  if (ev_reiniciar) {
    ev_reiniciar = false;
    ev_siguiente = false;
    animActual   = 0;
    Serial.println("TOUCH x3+ → reiniciando desde la primera animación");
    tone(BUZZER_PIN, 800, 80);
  }

  if (animActual >= totalArchivos) animActual = 0;

  String path   = "/" + archivos[animActual];
  int frames    = (int)(LittleFS.open(path, "r").size() / FRAME_SIZE);

  Serial.printf("[%d/%d] %s  (%d frames)\n",
    animActual + 1, totalArchivos, archivos[animActual].c_str(), frames);

  mostrarNombreAnim(archivos[animActual].c_str(), frames, animActual, totalArchivos);

  ev_siguiente = false;
  bool termino = reproducirAnimacion(path.c_str());

  if (ev_siguiente || termino) {
    ev_siguiente = false;
    animActual++;
    if (animActual >= totalArchivos) {
      animActual = 0;
      Serial.println("Ciclo completo. Volviendo al inicio.");
      activarAdafruit();
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(1);
      display.setCursor(20, 20);  display.print("Ciclo completo");
      display.setCursor(16, 36);  display.print("Volviendo al inicio");
      display.display();
      tone(BUZZER_PIN, 880, 100); delay(150); tone(BUZZER_PIN, 1046, 200);
      delay(1500);
    }
  }
}
