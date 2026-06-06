/*
  firmware_canvas_test.ino — Prueba del modo Canvas OLED para Dasai Mochi

  Qué hace:
    1. Conecta WiFi
    2. Abre un WebSocket persistente con el servidor Railway en /ws
    3. Recibe mensajes JSON del servidor:
         {"type":"canvas_update","bitmap":"<base64 1024 bytes>"}  → dibuja en SH1106
         {"type":"canvas_exit"}                                    → vuelve a pantalla de estado
    4. En modo CANVAS el OLED queda bajo control total del servidor.
       En modo NORMAL muestra IP y estado de conexión.

  Hardware:
    - ESP32-C6 (o cualquier ESP32)
    - Display SH1106 128×64 por I2C (SDA=6, SCL=7)

  Librerías — instalar en Arduino IDE (Library Manager):
    · "WebSockets"  de Markus Sattler  →  WebSocketsClient
    · "ArduinoJson" de Benoit Blanchon →  ArduinoJson
    · "U8g2"        de oliver          →  U8g2
    · WiFi y Wire vienen incluidas con el core ESP32.

  Cómo configurar:
    1. Cambia DEVICE_ID con un nombre único para este ESP32
    2. Ajusta WIFI_SSID / WIFI_PASS
    3. Ajusta WS_HOST con tu dominio de Railway (sin https://)
       Ejemplo: "dasaimochiservidor-production.up.railway.app"
*/

#include <WiFi.h>
#include <WebSocketsClient_Generic.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>

// ── CONFIGURACIÓN PERSONAL ──────────────────────────────────────────────────
#define DEVICE_ID  "especito"          // ← nombre único de este dispositivo

const char* WIFI_SSID = "TU_RED_WIFI";
const char* WIFI_PASS = "TU_CONTRASEÑA";

const char* WS_HOST = "dasaimochiservidor-production.up.railway.app";
const int   WS_PORT = 443;
// ────────────────────────────────────────────────────────────────────────────

static char WS_PATH[64];   // construido en setup() a partir de DEVICE_ID

// ─── OLED SH1106 128×64 ───────────────────────────────────────────────────────
#define OLED_SDA 6
#define OLED_SCL 7
// _F_ = full frame buffer: permite getBufferPtr() + sendBuffer()
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// ─── WebSocket ────────────────────────────────────────────────────────────────
WebSocketsClient webSocket;
bool wsConnected = false;

// ─── Estado del modo canvas ───────────────────────────────────────────────────
enum Mode { MODE_NORMAL, MODE_CANVAS };
Mode currentMode = MODE_NORMAL;

// Buffer para el bitmap recibido: 8 páginas × 128 columnas = 1024 bytes
static uint8_t canvasBitmap[1024];

// ─── Base64 decode (sin librerías externas) ───────────────────────────────────
static int b64val(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1; // '=' (padding) u otro carácter inválido
}

// Decodifica base64 en 'in' hacia 'out'. Devuelve bytes escritos.
size_t base64Decode(const char* in, uint8_t* out, size_t outMax) {
  size_t outIdx = 0;
  size_t inLen  = strlen(in);
  for (size_t i = 0; i + 3 < inLen && outIdx < outMax; i += 4) {
    int v0 = b64val(in[i]);
    int v1 = b64val(in[i + 1]);
    int v2 = b64val(in[i + 2]);
    int v3 = b64val(in[i + 3]);
    if (v0 < 0 || v1 < 0) break;
    out[outIdx++] = (uint8_t)((v0 << 2) | (v1 >> 4));
    if (v2 >= 0 && outIdx < outMax) out[outIdx++] = (uint8_t)((v1 << 4) | (v2 >> 2));
    if (v3 >= 0 && outIdx < outMax) out[outIdx++] = (uint8_t)((v2 << 6) | v3);
  }
  return outIdx;
}

// ─── Dibujar bitmap en el SH1106 ─────────────────────────────────────────────
// Formato del bitmap (igual al que genera canvas.html):
//   bitmap[page*128 + col]  — bit 0 = pixel superior de la página, bit 7 = inferior
// El buffer interno de U8g2 en modo F usa exactamente el mismo layout,
// por eso basta con memcpy + sendBuffer.
void drawBitmap(const uint8_t* bitmap) {
  uint8_t* buf = u8g2.getBufferPtr();
  memcpy(buf, bitmap, 1024);
  u8g2.sendBuffer();
}

// ─── Pantalla de estado (modo normal) ────────────────────────────────────────
void drawStatusScreen() {
  u8g2.clearBuffer();

  // Título
  u8g2.setFont(u8g2_font_ncenB08_tr);
  const char* title = "Mochi Canvas";
  int tw = u8g2.getStrWidth(title);
  u8g2.drawStr((128 - tw) / 2, 14, title);
  u8g2.drawHLine(0, 17, 128);

  // Estado WiFi y WebSocket
  u8g2.setFont(u8g2_font_5x7_tr);

  char wifiLine[40];
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(wifiLine, sizeof(wifiLine), "WiFi: %s", WiFi.localIP().toString().c_str());
  } else {
    snprintf(wifiLine, sizeof(wifiLine), "WiFi: sin conexion");
  }
  u8g2.drawStr(0, 28, wifiLine);

  const char* wsLine = wsConnected ? "WS:  conectado" : "WS:  esperando...";
  u8g2.drawStr(0, 38, wsLine);

  u8g2.drawHLine(0, 41, 128);

  // Hint inferior centrado
  const char* hint = wsConnected ? "usa canvas.html" : "conectando...";
  int hw = u8g2.getStrWidth(hint);
  u8g2.drawStr((128 - hw) / 2, 53, hint);

  u8g2.sendBuffer();
}

// ─── Procesar mensaje WebSocket JSON ─────────────────────────────────────────
void handleWsText(uint8_t* payload, size_t length) {
  // Parseo in-place: ArduinoJson guarda punteros al payload (no copia el bitmap)
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, reinterpret_cast<char*>(payload));
  if (err) {
    Serial.printf("[WS] JSON error: %s\n", err.c_str());
    return;
  }

  const char* type = doc["type"];
  if (!type) return;

  if (strcmp(type, "canvas_update") == 0) {
    const char* b64 = doc["bitmap"];
    if (!b64) return;

    size_t decoded = base64Decode(b64, canvasBitmap, sizeof(canvasBitmap));
    Serial.printf("[WS] canvas_update — %d bytes decodificados\n", decoded);

    if (decoded == 1024) {
      currentMode = MODE_CANVAS;
      drawBitmap(canvasBitmap);
    } else {
      Serial.printf("[WS] bitmap incorrecto: esperaba 1024, recibí %d\n", decoded);
    }

  } else if (strcmp(type, "canvas_exit") == 0) {
    Serial.println("[WS] canvas_exit — restaurando modo normal");
    currentMode = MODE_NORMAL;
    drawStatusScreen();

  }
  // El resto de tipos (transcription, intent, etc.) se ignoran en este test.
}

// ─── Callback de eventos WebSocket ────────────────────────────────────────────
void onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("[WS] Desconectado");
      if (currentMode == MODE_NORMAL) drawStatusScreen();
      break;

    case WStype_CONNECTED:
      wsConnected = true;
      Serial.printf("[WS] Conectado a wss://%s%s\n", WS_HOST, WS_PATH);
      if (currentMode == MODE_NORMAL) drawStatusScreen();
      break;

    case WStype_TEXT:
      handleWsText(payload, length);
      break;

    case WStype_ERROR:
      Serial.printf("[WS] Error — length=%d\n", length);
      break;

    default:
      break;
  }
}

// ─── setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  drawStatusScreen();

  // WiFi
  Serial.printf("Conectando a %s...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print('.');
    tries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi: no se pudo conectar");
  }
  drawStatusScreen();

  // Construye la ruta incluyendo el device_id para que el servidor lo identifique
  snprintf(WS_PATH, sizeof(WS_PATH), "/ws?device_id=%s", DEVICE_ID);

  webSocket.onEvent(onWsEvent);
#ifdef DEBUG_LOCAL
  webSocket.begin(WS_HOST, WS_PORT, WS_PATH);        // sin TLS — servidor local
  Serial.printf("WebSocket → ws://%s:%d%s\n", WS_HOST, WS_PORT, WS_PATH);
#else
  webSocket.beginSSL(WS_HOST, WS_PORT, WS_PATH);     // TLS — Railway
  Serial.printf("WebSocket → wss://%s:%d%s\n", WS_HOST, WS_PORT, WS_PATH);
#endif
  webSocket.setReconnectInterval(5000);
}

// ─── loop ─────────────────────────────────────────────────────────────────────
void loop() {
  // Mantiene la conexión WS y despacha mensajes entrantes — llamar muy frecuente.
  webSocket.loop();

  // En modo normal refresca la pantalla de estado cada 3 s (no bloqueante).
  static unsigned long lastRefresh = 0;
  if (currentMode == MODE_NORMAL && millis() - lastRefresh > 3000) {
    lastRefresh = millis();
    drawStatusScreen();
  }
}
