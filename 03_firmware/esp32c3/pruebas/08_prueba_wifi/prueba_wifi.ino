/*
  prueba_wifi.ino
  Hardware: ESP32-C3 Super Mini + OLED SH1106 + TTP223 + Buzzer

  Verifica WiFi y NTP usando el OLED, touch y buzzer como feedback.
  Reproduce el comportamiento exacto del Modo 2 (Reloj) de mochi_unified_5.

  SECUENCIA AUTOMÁTICA:
    1. Escaneo  — lista redes disponibles en el OLED y serial
    2. Conexión — conecta a la red configurada con animación de progreso
    3. NTP      — sincroniza la hora con pool.ntp.org (UTC-5 Ecuador)
    4. Reloj    — muestra hora y fecha en vivo, igual que el firmware

  CONTROLES TOUCH (mientras muestra el reloj):
    TAP x1   → alterna entre vista reloj y detalles de red (IP, RSSI, gateway)
    TAP x3+  → vuelve a escanear redes
    HOLD     → desconecta y reconecta desde cero

  FEEDBACK BUZZER:
    ●  Conectado a WiFi   → tono ascendente (doble beep)
    ●  NTP sincronizado   → triple beep
    ●  Error / timeout    → tono grave descendente

  CONFIGURACIÓN — edita las dos líneas de credenciales abajo.

  PINES:
    OLED SDA = GPIO 8  /  SCL = GPIO 9
    Touch    = GPIO 2
    Buzzer   = GPIO 10
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <time.h>

// ── ENABLE_ENTERPRISE: true = incluye stack WPA2-Enterprise (UPS) ───────
// ── Dejar false si solo usas redes WPA2-Personal — ahorra ~21 KB flash ──
#define ENABLE_ENTERPRISE true

#if ENABLE_ENTERPRISE
#  if __has_include(<esp_eap_client.h>)
#    include <esp_eap_client.h>
#    define EAP_ENABLE()          esp_wifi_sta_enterprise_enable()
#    define EAP_SET_IDENTITY(u,n) esp_eap_client_set_identity((const uint8_t*)(u),(n))
#    define EAP_SET_USERNAME(u,n) esp_eap_client_set_username((const uint8_t*)(u),(n))
#    define EAP_SET_PASSWORD(p,n) esp_eap_client_set_password((const uint8_t*)(p),(n))
#  else
#    include <esp_wpa2.h>
#    define EAP_ENABLE()          esp_wifi_sta_wpa2_ent_enable()
#    define EAP_SET_IDENTITY(u,n) esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)(u),(n))
#    define EAP_SET_USERNAME(u,n) esp_wifi_sta_wpa2_ent_set_username((uint8_t*)(u),(n))
#    define EAP_SET_PASSWORD(p,n) esp_wifi_sta_wpa2_ent_set_password((uint8_t*)(p),(n))
#  endif
#else
#  define EAP_ENABLE()
#  define EAP_SET_IDENTITY(u,n)
#  define EAP_SET_USERNAME(u,n)
#  define EAP_SET_PASSWORD(p,n)
#endif

// ── Credenciales — EDITAR ─────────────────────────────────────────────────
struct WiFiCredential {
  const char* ssid;
  const char* password;
  bool        enterprise;    // true = WPA2-Enterprise (EAP-TTLS)
  const char* eap_identity;  // solo si enterprise = true
  const char* eap_password;  // solo si enterprise = true
};

WiFiCredential networks[] = {
  // WPA2-Personal (redes domésticas — mayor prioridad primero)
  {"TU_RED_WIFI",      "TU_CONTRASEÑA",          false, "", ""},
  // WPA2-Enterprise (red universitaria)
  {"TU_RED_ENTERPRISE", "",                  true,
   "tu_usuario@universidad.edu", "tu_contraseña_eap"},
};
const int NUM_NETWORKS = sizeof(networks) / sizeof(networks[0]);

// ── NTP (mismos valores que mochi_unified_5) ──────────────────────────────
const char* NTP_SERVER       = "pool.ntp.org";
const long  GMT_OFFSET_SEC   = -18000;   // UTC-5 Ecuador
const int   DAYLIGHT_SEC     = 0;

// ── Fecha y hora en español (igual que el firmware) ───────────────────────
const char* DIAS[]   = {"Dom","Lun","Mar","Mie","Jue","Vie","Sab"};
const char* MESES[]  = {"Ene","Feb","Mar","Abr","May","Jun",
                         "Jul","Ago","Sep","Oct","Nov","Dic"};

// ── Pines ─────────────────────────────────────────────────────────────────
#define SDA_PIN     8
#define SCL_PIN     9
#define OLED_ADDR   0x3C
#define TOUCH_PIN   2
#define BUZZER_PIN  10

// ── Umbrales touch ────────────────────────────────────────────────────────
#define HOLD_MS      800
#define MULTITAP_MS  400

// ── Display ───────────────────────────────────────────────────────────────
Adafruit_SH1106G display(128, 64, &Wire, -1);

// ── Estado de la app ──────────────────────────────────────────────────────
enum Vista { ESCANEANDO, CONECTANDO, RELOJ, DETALLES };
Vista vistaActual = ESCANEANDO;
bool  ntpSincronizado = false;

// ── Touch ─────────────────────────────────────────────────────────────────
bool     prevTouch   = false;
uint32_t pressStart  = 0;
uint32_t lastRelease = 0;
int      tapCount    = 0;
bool     waitingMore = false;

// ═══════════════════════════════════════════════════════════════════════════
//  Buzzer
// ═══════════════════════════════════════════════════════════════════════════

void beepConectado() {
  tone(BUZZER_PIN, 880,  100); delay(150);
  tone(BUZZER_PIN, 1046, 200); delay(250);
}

void beepNTP() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1200, 80);
    delay(150);
  }
}

void beepError() {
  tone(BUZZER_PIN, 400, 150); delay(180);
  tone(BUZZER_PIN, 280, 400); delay(450);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Pantallas
// ═══════════════════════════════════════════════════════════════════════════

void cabecera(const char* titulo, bool ok = false) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(titulo);
  if (ok) {
    display.setCursor(100, 0);
    display.print("OK");
  }
  display.drawFastHLine(0, 10, 128, SH110X_WHITE);
}

// ── Pantalla: lista de redes ───────────────────────────────────────────────
void mostrarRedes(int n) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("ESCANEO WIFI");
  display.setTextSize(1);

  if (n == 0) {
    display.setCursor(10, 24);
    display.print("Sin redes detectadas");
    display.display();
    return;
  }

  // Muestra hasta 4 redes (limitado por el OLED)
  int mostrar = min(n, 4);
  for (int i = 0; i < mostrar; i++) {
    display.setCursor(0, 13 + i * 13);
    display.printf("%-16s %ddBm", WiFi.SSID(i).substring(0, 16).c_str(), WiFi.RSSI(i));
  }
  if (n > 4) {
    display.setCursor(0, 56);
    display.printf("...y %d mas", n - 4);
  }
  display.display();
}

// ── Pantalla: conectando con animación de puntos ───────────────────────────
void mostrarConectando(const char* ssid, int intento, int maxIntentos) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("CONECTANDO...");

  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print("Red: ");
  display.print(ssid);

  // Barra de progreso con puntos
  int puntosFull = intento * 10 / maxIntentos;
  display.setCursor(0, 28);
  for (int i = 0; i < 10; i++) {
    display.print(i < puntosFull ? "●" : "○");
  }

  display.setCursor(0, 42);
  display.printf("Intento %d / %d", intento, maxIntentos);

  display.display();
}

// ── Pantalla: reloj (igual que mochi_unified_5 Modo 2) ────────────────────
void mostrarReloj() {
  struct tm ti;
  if (!getLocalTime(&ti)) return;

  char hora[6], fecha[20];
  strftime(hora, sizeof(hora), "%H:%M", &ti);
  sprintf(fecha, "%s %02d %s", DIAS[ti.tm_wday], ti.tm_mday, MESES[ti.tm_mon]);

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("PRUEBA WIFI", true);

  display.setTextSize(3);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds(hora, 0, 0, &x1, &y1, &tw, &th);
  display.setCursor((128 - tw) / 2, 14);
  display.print(hora);

  display.setTextSize(1);
  display.getTextBounds(fecha, 0, 0, &x1, &y1, &tw, &th);
  display.setCursor((128 - tw) / 2, 46);
  display.print(fecha);

  display.setCursor(0, 56);
  display.printf("RSSI:%ddBm  x1=info x3=scan", WiFi.RSSI());

  display.display();
}

// ── Pantalla: detalles de red ──────────────────────────────────────────────
void mostrarDetalles() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("DETALLES RED");

  display.setTextSize(1);
  display.setCursor(0, 13);
  display.printf("IP:   %s", WiFi.localIP().toString().c_str());
  display.setCursor(0, 24);
  display.printf("GW:   %s", WiFi.gatewayIP().toString().c_str());
  display.setCursor(0, 35);
  display.printf("DNS:  %s", WiFi.dnsIP().toString().c_str());
  display.setCursor(0, 46);
  display.printf("RSSI: %d dBm", WiFi.RSSI());
  display.setCursor(0, 56);
  display.print("x1=reloj  HOLD=reconectar");

  display.display();
}

// ── Pantalla: error ────────────────────────────────────────────────────────
void mostrarError(const char* msg) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("ERROR WIFI");
  display.setTextSize(1);
  display.setCursor(0, 18);
  display.print(msg);
  display.setCursor(0, 40);
  display.print("HOLD = reintentar");
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════════
//  WiFi + NTP
// ═══════════════════════════════════════════════════════════════════════════

bool connectWPA2Personal(const WiFiCredential& c) {
  Serial.printf("Conectando a %s...\n", c.ssid);
  WiFi.disconnect(true); delay(100);
  WiFi.begin(c.ssid, c.password);
  const int MAX = 20;
  for (int i = 1; i <= MAX; i++) {
    mostrarConectando(c.ssid, i, MAX);
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(500);
  }
  return false;
}

bool connectWPA2Enterprise(const WiFiCredential& c) {
#if ENABLE_ENTERPRISE
  Serial.printf("Conectando a %s (Enterprise)...\n", c.ssid);
  WiFi.disconnect(true); delay(100);
  EAP_SET_IDENTITY(c.eap_identity, strlen(c.eap_identity));
  EAP_SET_USERNAME(c.eap_identity, strlen(c.eap_identity));
  EAP_SET_PASSWORD(c.eap_password, strlen(c.eap_password));
  EAP_ENABLE();
  WiFi.begin(c.ssid);
  const int MAX = 30;
  for (int i = 1; i <= MAX; i++) {
    mostrarConectando(c.ssid, i, MAX);
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(500);
  }
  return false;
#else
  return false;
#endif
}

bool conectarWiFi() {
  WiFi.mode(WIFI_STA);
  for (int n = 0; n < NUM_NETWORKS; n++) {
    WiFiCredential& c = networks[n];
    bool ok = c.enterprise ? connectWPA2Enterprise(c) : connectWPA2Personal(c);
    if (ok) {
      Serial.printf("Conectado a %s. IP: %s  RSSI: %d dBm\n",
        c.ssid, WiFi.localIP().toString().c_str(), WiFi.RSSI());
      beepConectado();
      return true;
    }
    Serial.printf("Timeout en red %d/%d.\n", n+1, NUM_NETWORKS);
  }
  Serial.println("ERROR: no se pudo conectar a ninguna red.");
  beepError();
  return false;
}

bool sincronizarNTP() {
  Serial.println("Sincronizando NTP...");

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("PRUEBA WIFI", true);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.printf("IP: %s", WiFi.localIP().toString().c_str());
  display.setCursor(20, 36);
  display.print("Sincronizando NTP");
  display.display();

  configTime(GMT_OFFSET_SEC, DAYLIGHT_SEC, NTP_SERVER);

  struct tm ti;
  if (getLocalTime(&ti, 8000)) {
    char buf[30];
    strftime(buf, sizeof(buf), "%H:%M:%S  %d/%m/%Y", &ti);
    Serial.printf("Hora sincronizada: %s\n", buf);
    beepNTP();
    return true;
  }

  Serial.println("ERROR: NTP no respondió.");
  beepError();
  return false;
}

void escanearRedes() {
  Serial.println("Escaneando redes WiFi...");

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  cabecera("ESCANEO WIFI");
  display.setTextSize(1);
  display.setCursor(30, 28);
  display.print("Escaneando...");
  display.display();

  int n = WiFi.scanNetworks();
  Serial.printf("%d red(es) encontrada(s):\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("  [%d] %-24s  %d dBm\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }

  mostrarRedes(n);
  delay(3000);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Touch
// ═══════════════════════════════════════════════════════════════════════════

void procesarTouch() {
  bool touched = digitalRead(TOUCH_PIN);
  uint32_t now = millis();

  if (prevTouch && !touched) {
    uint32_t dur = now - pressStart;
    if (dur >= HOLD_MS) {
      tapCount    = 0;
      waitingMore = false;
      Serial.println("TOUCH  HOLD → reconectar");
      // Desconectar y volver a conectar desde cero
      WiFi.disconnect(true);
      ntpSincronizado = false;
      vistaActual = CONECTANDO;
      if (conectarWiFi()) {
        ntpSincronizado = sincronizarNTP();
      } else {
        mostrarError("No se pudo conectar.");
      }
      vistaActual = ntpSincronizado ? RELOJ : ESCANEANDO;
    } else {
      tapCount++;
      lastRelease = now;
      waitingMore = true;
    }
  }

  if (!prevTouch && touched) pressStart = now;

  if (waitingMore && !touched && (now - lastRelease) >= MULTITAP_MS) {
    waitingMore = false;
    if (tapCount == 1) {
      // Alternar entre reloj y detalles
      vistaActual = (vistaActual == RELOJ) ? DETALLES : RELOJ;
      Serial.printf("TOUCH  TAP x1 → vista %s\n", vistaActual == RELOJ ? "RELOJ" : "DETALLES");
    } else {
      Serial.println("TOUCH  TAP x3+ → escanear redes");
      escanearRedes();
      vistaActual = RELOJ;
    }
    tapCount = 0;
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
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED no encontrado.");
    while (true) delay(1000);
  }

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA WIFI + NTP");
  Serial.println("========================================");
  Serial.printf("  Redes configuradas: %d\n", NUM_NETWORKS);
  for (int i = 0; i < NUM_NETWORKS; i++)
    Serial.printf("    [%d] %s%s\n", i+1, networks[i].ssid, networks[i].enterprise ? " (Enterprise)" : "");
  Serial.printf("  NTP:    %s  UTC-5\n", NTP_SERVER);
  Serial.println("========================================");
  Serial.println();

  // ── Test 1: escaneo ─────────────────────────────────────────────────────
  escanearRedes();

  // ── Test 2: conexión ────────────────────────────────────────────────────
  vistaActual = CONECTANDO;
  if (!conectarWiFi()) {
    mostrarError("Timeout. Verifica SSID y clave.");
    return;   // setup termina aquí; loop no hace nada útil
  }

  // ── Test 3: NTP ─────────────────────────────────────────────────────────
  ntpSincronizado = sincronizarNTP();
  if (!ntpSincronizado) {
    mostrarError("WiFi OK pero NTP fallo.");
    // Continúa igual, al menos podemos mostrar detalles de red
  }

  vistaActual = ntpSincronizado ? RELOJ : DETALLES;

  Serial.println();
  Serial.println("Setup completo. Controles touch:");
  Serial.println("  TAP x1  → alternar reloj / detalles de red");
  Serial.println("  TAP x3+ → escanear redes de nuevo");
  Serial.println("  HOLD    → desconectar y reconectar");
}

void loop() {
  procesarTouch();

  // Actualizar vista cada 500 ms
  static uint32_t lastRender = 0;
  if (millis() - lastRender >= 500) {
    lastRender = millis();
    if      (vistaActual == RELOJ)    mostrarReloj();
    else if (vistaActual == DETALLES) mostrarDetalles();
  }

  delay(10);
}
