/*
  transcripcion.ino
  Hardware: ESP32-C6 Supermini + INMP441

  Graba audio con el micrófono al tocar el sensor,
  lo envía como archivo WAV a un servidor local vía HTTP POST
  y muestra la transcripción recibida en JSON.

  FLUJO:
    1. Toca el sensor → beep corto → empieza a grabar
    2. [graba RECORD_SECS segundos]
    3. Beep largo → grabación terminada
    4. Envía el WAV al servidor local
    5. Servidor responde: {"text": "lo que dijiste"}
    6. Se imprime en el monitor serial

  CONFIGURACIÓN OBLIGATORIA (ver sección de constantes más abajo):
    - WIFI_SSID / WIFI_PASS
    - SERVER_URL (IP de tu computadora en la red local)

  DEPENDENCIAS (instalar con Arduino CLI o Library Manager):
    - ArduinoJson  (Benoit Blanchon, v7+)

  PINES ESP32-C6 Supermini:
    Touch TTP223 = GPIO 2
    Buzzer       = GPIO 19
    INMP441:  SCK=GPIO4  WS=GPIO5  SD=GPIO3  L/R→GND
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

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

// ── Configuración — EDITAR ANTES DE SUBIR ─────────────────────────────────
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

// IP de tu computadora en la red local (ver cómo obtenerla en README.md)
// Puerto 5000 es el que usa el servidor Python por defecto
const char* SERVER_URL = "http://192.168.1.100:5000/transcribir";

// ── Pines ─────────────────────────────────────────────────────────────────
#define TOUCH_PIN   2
#define BUZZER_PIN  19
#define I2S_SCK     4
#define I2S_WS      5
#define I2S_SD      3
#define I2S_PORT    I2S_NUM_0

// ── Parámetros de grabación ───────────────────────────────────────────────
#define SAMPLE_RATE    16000       // Hz — debe coincidir con el servidor
#define RECORD_SECS    3           // segundos a grabar por toque
#define PCM_BYTES      (SAMPLE_RATE * RECORD_SECS * 2)  // 16-bit = 2 bytes/muestra
#define WAV_HEADER     44
#define WAV_TOTAL      (WAV_HEADER + PCM_BYTES)          // ~96 KB para 3 s

// ── Estado interno ────────────────────────────────────────────────────────
enum Estado { ESPERANDO, GRABANDO, ENVIANDO };
Estado estado = ESPERANDO;

bool prevTouch = false;

// ═══════════════════════════════════════════════════════════════════════════
//  WAV
// ═══════════════════════════════════════════════════════════════════════════

void escribirU16(uint8_t* buf, int pos, uint16_t val) {
  buf[pos]   = val & 0xFF;
  buf[pos+1] = (val >> 8) & 0xFF;
}
void escribirU32(uint8_t* buf, int pos, uint32_t val) {
  buf[pos]   = val & 0xFF;
  buf[pos+1] = (val >> 8)  & 0xFF;
  buf[pos+2] = (val >> 16) & 0xFF;
  buf[pos+3] = (val >> 24) & 0xFF;
}

void construirCabeceraWAV(uint8_t* header, uint32_t pcmBytes) {
  uint32_t byteRate  = SAMPLE_RATE * 2;   // mono 16-bit
  uint32_t dataSize  = pcmBytes;
  uint32_t chunkSize = 36 + dataSize;

  memcpy(header,      "RIFF", 4);
  escribirU32(header,  4, chunkSize);
  memcpy(header +  8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);
  escribirU32(header, 16, 16);             // subchunk1 size (PCM = 16)
  escribirU16(header, 20, 1);             // PCM format
  escribirU16(header, 22, 1);             // mono
  escribirU32(header, 24, SAMPLE_RATE);
  escribirU32(header, 28, byteRate);
  escribirU16(header, 32, 2);             // block align
  escribirU16(header, 34, 16);            // bits per sample
  memcpy(header + 36, "data", 4);
  escribirU32(header, 40, dataSize);
}

// ═══════════════════════════════════════════════════════════════════════════
//  I2S — micrófono
// ═══════════════════════════════════════════════════════════════════════════

void iniciarMicrofono() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = 512,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pines = {
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pines);
}

// Lee PCM_BYTES de audio en el buffer dado. Devuelve true si tuvo éxito.
bool grabar(uint8_t* pcmBuf) {
  Serial.printf("Grabando %d segundos...\n", RECORD_SECS);
  size_t bytesLeidos = 0;
  // El timeout es (RECORD_SECS + 1) segundos para que el DMA llene el buffer completo
  esp_err_t r = i2s_read(I2S_PORT, pcmBuf, PCM_BYTES, &bytesLeidos,
                          pdMS_TO_TICKS((RECORD_SECS + 1) * 1000));
  if (r != ESP_OK) {
    Serial.printf("Error I2S al grabar: %d\n", r);
    return false;
  }
  Serial.printf("Grabados %d bytes (esperado %d)\n", bytesLeidos, PCM_BYTES);
  return bytesLeidos > 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  WiFi
// ═══════════════════════════════════════════════════════════════════════════

bool connectWPA2Personal(const WiFiCredential& c) {
  WiFi.disconnect(true); delay(100);
  WiFi.begin(c.ssid, c.password);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) { delay(500); intentos++; }
  return WiFi.status() == WL_CONNECTED;
}

bool connectWPA2Enterprise(const WiFiCredential& c) {
#if ENABLE_ENTERPRISE
  WiFi.disconnect(true); delay(100);
  EAP_SET_IDENTITY(c.eap_identity, strlen(c.eap_identity));
  EAP_SET_USERNAME(c.eap_identity, strlen(c.eap_identity));
  EAP_SET_PASSWORD(c.eap_password, strlen(c.eap_password));
  EAP_ENABLE();
  WiFi.begin(c.ssid);
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 30) { delay(500); intentos++; }
  return WiFi.status() == WL_CONNECTED;
#else
  return false;
#endif
}

bool conectarWiFi() {
  WiFi.mode(WIFI_STA);
  for (int n = 0; n < NUM_NETWORKS; n++) {
    WiFiCredential& c = networks[n];
    Serial.printf("Conectando a %s", c.ssid);
    bool ok = c.enterprise ? connectWPA2Enterprise(c) : connectWPA2Personal(c);
    if (ok) {
      Serial.printf("\nWiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }
    Serial.printf("\nTimeout en red %d/%d.\n", n+1, NUM_NETWORKS);
  }
  Serial.println("ERROR: no se pudo conectar a ninguna red.");
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//  HTTP — enviar WAV y recibir transcripción
// ═══════════════════════════════════════════════════════════════════════════

void enviarYTranscribir(uint8_t* wavBuf) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sin WiFi — reconectando...");
    if (!conectarWiFi()) return;
  }

  Serial.printf("Enviando %d bytes a %s\n", WAV_TOTAL, SERVER_URL);

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "audio/wav");
  http.setTimeout(30000);   // whisper puede tardar varios segundos

  int httpCode = http.POST(wavBuf, WAV_TOTAL);

  if (httpCode <= 0) {
    Serial.printf("Error de conexión: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  Serial.printf("Respuesta HTTP: %d\n", httpCode);

  if (httpCode == 200) {
    String body = http.getString();
    Serial.printf("JSON recibido: %s\n", body.c_str());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      Serial.printf("Error al parsear JSON: %s\n", err.c_str());
    } else {
      const char* texto = doc["text"] | "(sin texto)";
      Serial.println();
      Serial.println("┌─────────────────────────────┐");
      Serial.printf( "│ Transcripcion: %s\n", texto);
      Serial.println("└─────────────────────────────┘");
      Serial.println();
    }
  } else {
    Serial.printf("Error del servidor: HTTP %d\n", httpCode);
    Serial.println(http.getString());
  }

  http.end();
}

// ═══════════════════════════════════════════════════════════════════════════
//  setup / loop
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("=== TRANSCRIPCION DE VOZ — Mochi ===");
  Serial.printf("Heap libre al inicio: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Buffer WAV: %d bytes\n", WAV_TOTAL);

  if (WAV_TOTAL > (int)(ESP.getFreeHeap() * 0.8)) {
    Serial.println("ADVERTENCIA: poca memoria. Considera reducir RECORD_SECS.");
  }

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  iniciarMicrofono();
  conectarWiFi();

  Serial.println();
  Serial.println("Listo. Toca el sensor para grabar.");
  Serial.printf("Duracion: %d segundos por grabacion.\n", RECORD_SECS);
}

void loop() {
  bool touched = digitalRead(TOUCH_PIN);

  // Detecta flanco de subida (momento en que se toca)
  if (touched && !prevTouch && estado == ESPERANDO) {
    delay(30);  // debounce mínimo
    if (!digitalRead(TOUCH_PIN)) { prevTouch = false; return; }

    estado = GRABANDO;

    // Beep de inicio
    tone(BUZZER_PIN, 1200, 100);
    delay(150);

    // ── Allocar buffer completo (header + PCM) ──────────────────────────
    uint8_t* wavBuf = (uint8_t*)malloc(WAV_TOTAL);
    if (!wavBuf) {
      Serial.printf("ERROR: no hay memoria para el buffer (%d bytes).\n", WAV_TOTAL);
      Serial.printf("Heap libre: %d bytes\n", ESP.getFreeHeap());
      estado = ESPERANDO;
      return;
    }

    // ── Grabar audio al segmento PCM del buffer ─────────────────────────
    uint8_t* pcmBuf = wavBuf + WAV_HEADER;
    bool ok = grabar(pcmBuf);

    // Beep de fin de grabación
    tone(BUZZER_PIN, 800, 300);
    delay(350);

    if (ok) {
      // ── Escribir cabecera WAV antes del PCM ──────────────────────────
      construirCabeceraWAV(wavBuf, PCM_BYTES);

      // ── Enviar y mostrar resultado ───────────────────────────────────
      estado = ENVIANDO;
      enviarYTranscribir(wavBuf);
    } else {
      Serial.println("Grabación fallida. Intenta de nuevo.");
    }

    free(wavBuf);
    estado = ESPERANDO;
    Serial.println("Listo para la siguiente grabacion.");
  }

  prevTouch = touched;
  delay(10);
}
