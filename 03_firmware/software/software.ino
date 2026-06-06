// ═══════════════════════════════════════════════════════════════════════
//  MOCHI ROBOT — mochi_v8.ino — ESP32-C6 Supermini
//
//  Firmware unificado producido a partir de:
//    11_codigo_funcional_transcripcion.ino  → I2S, FreeRTOS HTTP, Eye physics
//    08_prueba_clima.ino                    → Weather API, iconos, vistas
//    12_prueba_canvas.ino                   → drawBitmap (raw binary)
//    02_prueba_touch.ino                    → umbrales touch
//    06_prueba_littlefs.ino                 → GIF render pattern
//
//  NOTA de librería: usa Adafruit_SH1106G (Adafruit SH110X), no SSD1306.
//  El hardware real es SH1106. WS I2S = GPIO1 (confirmado en fuente).
// ═══════════════════════════════════════════════════════════════════════

#pragma GCC optimize("Os") // Optimizar para reducir el tamaño del binario

// ── 1. Includes ────────────────────────────────────────────────────────
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebSocketsClient_Generic.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <driver/i2s.h>
#include <time.h>
#include <math.h>

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

#define SSD1306_WHITE SH110X_WHITE
#define SSD1306_BLACK SH110X_BLACK

// ── Hardware defines ───────────────────────────────────────────────────
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define SDA_PIN          6
#define SCL_PIN          7
#define TOUCH_PIN        2
#define BUZZER_PIN      19
#define I2S_SCK_PIN      4
#define I2S_WS_PIN       1   // confirmado en 11_codigo_funcional_transcripcion
#define I2S_SD_PIN       3
#define I2S_PORT   I2S_NUM_0

// ── I2S / grabación (copiado de 11_codigo_funcional_transcripcion) ──────
#define REC_SAMPLE_RATE    8000
#define REC_DURATION_S        5
#define REC_AUDIO_BYTES  ((size_t)REC_DURATION_S * REC_SAMPLE_RATE * 2)  // 80000
#define CHUNK_SIZE         4096
#define GAIN_SHIFT           14
#define WARMUP_DURATION_MS  100

// ── Touch (copiado de 02_prueba_touch / 11_codigo_funcional) ───────────
#define HOLD_MS        800
#define TAP_WINDOW_MS  400
#define SHORT_TAP_MS   300

// ── GIF / LittleFS ─────────────────────────────────────────────────────
#define FRAME_SIZE      1024   // 128×64 / 8

// ── Clima ──────────────────────────────────────────────────────────────
#define WEATHER_UPDATE_MS 600000UL  // 10 minutos

// ══════════════════════════════════════════════════════════════════════
//  ZONA DE CONFIGURACIÓN — editar aquí
// ══════════════════════════════════════════════════════════════════════

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

#define SERVER_URL       "https://dasaimochiservidor-production.up.railway.app"
#define WS_HOST          "dasaimochiservidor-production.up.railway.app"
#define WS_PORT          443
#define DEVICE_ID        "mochi_v8"
#define OWM_API_KEY      "TU_API_KEY_OWM"
#define OWM_CITY         "Cuenca"
#define OWM_COUNTRY      "EC"
#define OWM_UNITS        "metric"
#define GMT_OFFSET       -18000L       // UTC-5 Ecuador

// ══════════════════════════════════════════════════════════════════════
//  FIN ZONA DE CONFIGURACIÓN
// ══════════════════════════════════════════════════════════════════════

// ── Display ────────────────────────────────────────────────────────────
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Canvas WebSocket (wss:// — copiado de 12_prueba_canvas) ──────────
WebSocketsClient webSocket;
static bool wsConnected  = false;
static bool canvasActive = false;   // true = bitmap activo, no sobreescribir
static char wsPath[64];
static uint8_t canvasBitmap[FRAME_SIZE];

// ═══════════════════════════════════════════════════════════════════════
// ── 2. Enums ───────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

enum Mode {
  MODE_MENU,
  MODE_MASCOTA,
  MODE_GIF,
  MODE_HORA,
  MODE_CLIMA,
  MODE_POMODORO,
  MODE_CANVAS,
  MODE_MIC
};

enum TouchEvent { EVT_NONE, EVT_TAP1, EVT_TAP2, EVT_TAP3, EVT_HOLD, EVT_SHORT_TAP };

enum MascotaState  { MASC_NORMAL, MASC_CARINHOSO, MASC_ENOJADO };
enum PomodoroState { POMO_STOPPED, POMO_RUNNING, POMO_ADJUST };

// ── Moods (copiados de 11_codigo_funcional_transcripcion) ───────────────
#define MOOD_NORMAL     0
#define MOOD_HAPPY      1
#define MOOD_SURPRISED  2
#define MOOD_SLEEPY     3
#define MOOD_ANGRY      4
#define MOOD_SAD        5
#define MOOD_EXCITED    6
#define MOOD_LOVE       7
#define MOOD_SUSPICIOUS 8
#define MOOD_DIZZY      9
#define MOOD_COUNT     10

const char* MOOD_NAMES[] = {
  "NORMAL","HAPPY","SURPRISED","SLEEPY","ANGRY",
  "SAD","EXCITED","LOVE","SUSPICIOUS","DIZZY"
};

// ═══════════════════════════════════════════════════════════════════════
// ── 3. Variables globales ──────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

// Estado de modos
Mode    currentMode = MODE_MASCOTA;
Mode    lastMode    = MODE_MASCOTA;
bool    fsOk        = false;
int     currentMood = MOOD_NORMAL;
float   breathVal   = 0;

// Moods (copiado de 11_codigo_funcional_transcripcion)
int  moodBeforeNoise = MOOD_NORMAL;
bool isStartled      = false;
unsigned long lastLoudNoiseTime = 0;

// Menú
int  menuCursor = 0;
const char* modeNames[] = {
  "Mascota","GIFs","Reloj","Clima","Pomodoro","Canvas","Mic"
};
// modeNames no incluye MODE_MENU; idx 0 = MODE_MASCOTA (offset +1)

// Touch — estado centralizado (copiado de 11_codigo_funcional_transcripcion)
bool          tc_last      = false;
unsigned long tc_downAt    = 0;
unsigned long tc_lastTapAt = 0;
int           tc_tapCount  = 0;
bool          tc_held      = false;

bool ev_shortTap  = false;
bool ev_longHold  = false;
bool ev_tapsReady = false;
int  ev_tapCount  = 0;
bool ev_isHolding = false;
bool ev_tap1      = false;
bool ev_tap2      = false;
bool ev_tap3      = false;

// Mascota
MascotaState  mascState       = MASC_NORMAL;
int           holdCount       = 0;
unsigned long firstHoldTime   = 0;
unsigned long mascStateUntil  = 0;

// GIF
static File   gifFile;
static bool   gifFileOpen  = false;
static bool   gifPaused    = false;
static int    gifIdx       = 0;
static unsigned long gifLastFrame = 0;
#define GIF_FPS 20
#define GIF_FRAME_DELAY (1000 / GIF_FPS)

#define MAX_GIFS 20
static char gifPaths[MAX_GIFS][32];
static int  NUM_GIFS = 0;

// Reloj
bool timeSynced  = false;
bool wifiFailed  = false;
bool showSeconds = false;
const char* diasSemana[] = {"Dom","Lun","Mar","Mie","Jue","Vie","Sab"};
const char* mesesAnio[]  = {"Ene","Feb","Mar","Abr","May","Jun",
                             "Jul","Ago","Sep","Oct","Nov","Dic"};

// Clima (copiado de 08_prueba_clima, sin DayForecast) ──────────────────
struct WeatherData {
  float temp, tempMin, tempMax, humidity, windSpeed;
  int   weatherId;
  bool  isNight;
  char  desc[40];
  char  city[24];
  char  timeStr[8];
  bool         valid    = false;
  unsigned long fetchedAt = 0;
};
WeatherData wx;
int weatherView = 0;

// Pomodoro
PomodoroState pomoState      = POMO_STOPPED;
int           pomoSeconds    = 25 * 60;
unsigned long lastPomoTick   = 0;
int           adjustOp       = 0;    // 0=sumar 1=restar
unsigned long lastAdjustAuto = 0;

// Micrófono (copiado de 11_codigo_funcional_transcripcion) ──────────────
bool           recRecording    = false;
bool           warmup_complete = false;
size_t         warmup_samples  = 0;
static size_t  _rec_len        = 0;
static size_t  _fs_len         = 0;
static char    _rec_sid[20]    = "";
static volatile bool _rec_done   = false;
static volatile int  _rec_status = 0;
static TaskHandle_t  _rec_task   = NULL;
static char    _rec_intent[64]  = "";

static bool holdMicStarted = false;

// Eye physics (copiado de 11_codigo_funcional_transcripcion) ────────────
struct Eye {
  float x,y,w,h,targetX,targetY,targetW,targetH;
  float pupilX,pupilY,targetPupilX,targetPupilY;
  float velX,velY,velW,velH,pVelX,pVelY;
  float k=0.12f,d=0.60f,pk=0.08f,pd=0.50f;
  bool  blinking=false;
  unsigned long lastBlink=0,nextBlinkTime=0;

  void init(float _x,float _y,float _w,float _h){
    x=targetX=_x; y=targetY=_y; w=targetW=_w; h=targetH=_h;
    pupilX=targetPupilX=pupilY=targetPupilY=0;
    velX=velY=velW=velH=pVelX=pVelY=0;
    nextBlinkTime=millis()+random(1000,4000);
  }
  void update(){
    velX=(velX+(targetX-x)*k)*d; velY=(velY+(targetY-y)*k)*d;
    velW=(velW+(targetW-w)*k)*d; velH=(velH+(targetH-h)*k)*d;
    x+=velX; y+=velY; w+=velW; h+=velH;
    pVelX=(pVelX+(targetPupilX-pupilX)*pk)*pd;
    pVelY=(pVelY+(targetPupilY-pupilY)*pk)*pd;
    pupilX+=pVelX; pupilY+=pVelY;
  }
};

Eye leftEye, rightEye;
unsigned long lastSaccade=0, saccadeInterval=3000;

// ═══════════════════════════════════════════════════════════════════════
// ── 4. Buffers estáticos de audio (BSS, no heap — spec crítico) ────────
// ═══════════════════════════════════════════════════════════════════════

static uint8_t acc_buffer[REC_AUDIO_BYTES];   // grabación
static uint8_t _fs_buf[REC_AUDIO_BYTES];      // copia para tarea HTTP

// ═══════════════════════════════════════════════════════════════════════
// ── 5. Prototipos ──────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

void loadGifList();
void detectTouchEvent();
void runMenuMode();
void runMascotaMode();
void runGifMode();
void runHoraMode();
void runClimaMode();
void runPomodoroMode();
void runCanvasMode();
void runMicMode();
void initWebSocket();
void onWsEvent(WStype_t type, uint8_t* payload, size_t length);
void handleWsText(uint8_t* payload, size_t length);
size_t base64Decode(const char* in, uint8_t* out, size_t outMax);
void startRecording();
void stopAndSend();
void buzzerBeep(int freq, int ms);
void showMsg(const char* l1, const char* l2 = nullptr);
int  weatherCategory(int id, bool isNight);
void drawIconLarge(int cx, int cy, int cat);
void drawIconSmall(int cx, int cy, int cat);
void drawEyelidMask(float x,float y,float w,float h,int mood,bool isLeft);
void drawEye(Eye& e, bool isLeft);
void updateEyes();
void renderEyes(const char* label);
void drawBitmap(const uint8_t* bitmap);
bool scanAndConnect();
bool connectWPA2Personal(const WiFiCredential& c);
bool connectWPA2Enterprise(const WiFiCredential& c);

// ═══════════════════════════════════════════════════════════════════════
// ── 6. setup() ─────────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("\n=== MOCHI v8 ===");

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(0x3C, true)) {
    Serial.println("ERROR: OLED no encontrado");
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  leftEye.init(18, 14, 36, 36);
  rightEye.init(74, 14, 36, 36);

  // LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS: error. Sube data/ primero.");
  } else {
    fsOk = true;
    Serial.printf("LittleFS OK — %u/%u KB\n",
      (unsigned)(LittleFS.usedBytes()/1024),
      (unsigned)(LittleFS.totalBytes()/1024));
    loadGifList();
  }

  // I2S micrófono (copiado de 11_codigo_funcional_transcripcion)
  {
    i2s_config_t cfg = {
      .mode              = (i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),
      .sample_rate       = REC_SAMPLE_RATE,
      .bits_per_sample   = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format    = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags  = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count     = 16,
      .dma_buf_len       = 256,
      .use_apll          = false,
      .tx_desc_auto_clear= false,
      .fixed_mclk        = 0
    };
    i2s_pin_config_t pins = {
      .bck_io_num   = I2S_SCK_PIN,
      .ws_io_num    = I2S_WS_PIN,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num  = I2S_SD_PIN
    };
    i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
    Serial.println("I2S: 8kHz/32bit OK");
  }

  // WiFi — multi-red con prioridad
  WiFi.mode(WIFI_STA);
  if (scanAndConnect()) {
    Serial.printf("WiFi OK: %s\n", WiFi.localIP().toString().c_str());
    configTime(GMT_OFFSET, 0, "pool.ntp.org");
  } else {
    Serial.println("WiFi: sin conexion (modos offline activos)");
  }

  initWebSocket();

  // Arranque: GIF random durante ~2 s (sin texto de debug)
  if (fsOk && NUM_GIFS > 0) {
    int startIdx = (int)(esp_random() % NUM_GIFS);
    gifOpenFile(startIdx);
    unsigned long bootStart = millis();
    while (millis() - bootStart < 2000) {
      if (!gifNextFrame()) {          // EOF → rebobinar mismo GIF
        gifFile.close(); gifFileOpen = false;
        gifOpenFile(startIdx);
        gifNextFrame();
      }
      delay(GIF_FRAME_DELAY);
    }
    if (gifFileOpen) { gifFile.close(); gifFileOpen = false; }
  } else {
    display.clearDisplay(); display.display();
  }
  buzzerBeep(1046, 80); delay(120); buzzerBeep(1200, 150);
  Serial.println("=== LISTO ===");
}

// ═══════════════════════════════════════════════════════════════════════
// ── 7. loop() — lee touch, despacha al modo activo ────────────────────
// ═══════════════════════════════════════════════════════════════════════

void loop() {
  if (!_rec_task) webSocket.loop();
  // canvasActive se gestiona exclusivamente en handleWsText / canvas_exit
  detectTouchEvent();

  // 3 taps → menú desde cualquier modo (excepto ya en menú)
  if (ev_tap3 && currentMode != MODE_MENU && currentMode != MODE_MIC) {
    lastMode    = currentMode;
    currentMode = MODE_MENU;
    menuCursor  = (int)currentMode - 1;   // pre-seleccionar modo actual
    if (menuCursor < 0) menuCursor = 0;
    buzzerBeep(1000, 60);
    return;
  }

  // hold → MODE_MIC desde todos los modos excepto MASCOTA (cariñoso)
  // y POMODORO en submodo ajuste (auto-incremento)
  bool modeInterceptsHold =
    (currentMode == MODE_MASCOTA) ||
    (currentMode == MODE_POMODORO && pomoState == POMO_ADJUST) ||
    (currentMode == MODE_MIC);

  // Primer hold → entrar a MIC_IDLE (sin grabar aún)
  if (ev_isHolding && !holdMicStarted && !modeInterceptsHold && _rec_task == NULL) {
    holdMicStarted = true;
    lastMode       = currentMode;
    currentMode    = MODE_MIC;
    buzzerBeep(880, 60);
    return;
  }
  // El primer hold terminó: limpiar flag (la grabación se inicia desde runMicMode)
  if (ev_longHold && holdMicStarted) {
    holdMicStarted = false;
  }

  // Reconexión WiFi pasiva
  static unsigned long lastWifiCheck = 0;
  if (millis() - lastWifiCheck > 30000) {
    lastWifiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) scanAndConnect();
  }

  switch (currentMode) {
    case MODE_MENU:     runMenuMode();     break;
    case MODE_MASCOTA:  runMascotaMode();  break;
    case MODE_GIF:      runGifMode();      break;
    case MODE_HORA:     runHoraMode();     break;
    case MODE_CLIMA:    runClimaMode();    break;
    case MODE_POMODORO: runPomodoroMode(); break;
    case MODE_CANVAS:   runCanvasMode();   break;
    case MODE_MIC:      runMicMode();      break;
    default: currentMode = MODE_MASCOTA;
  }
}

// ═══════════════════════════════════════════════════════════════════════
// ── 8. WiFi multi-red ─────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

bool connectWPA2Personal(const WiFiCredential& c) {
  Serial.printf("Conectando a %s (Personal)...\n", c.ssid);
  WiFi.disconnect(true); delay(200);
  WiFi.begin(c.ssid, c.password);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) delay(500);
  return WiFi.status() == WL_CONNECTED;
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
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) delay(500);
  return WiFi.status() == WL_CONNECTED;
#else
  (void)c; return false;
#endif
}

bool scanAndConnect() {
  Serial.println("Escaneando redes WiFi...");
  int found = WiFi.scanNetworks();
  Serial.printf("Encontradas: %d redes\n", found);
  for (int ni = 0; ni < NUM_NETWORKS; ni++) {
    for (int si = 0; si < found; si++) {
      if (WiFi.SSID(si) == String(networks[ni].ssid)) {
        showMsg("Conectando...", networks[ni].ssid);
        bool ok = networks[ni].enterprise
          ? connectWPA2Enterprise(networks[ni])
          : connectWPA2Personal(networks[ni]);
        WiFi.scanDelete();
        if (ok) return true;
        break;  // red encontrada pero falló → siguiente en lista
      }
    }
  }
  WiFi.scanDelete();
  Serial.println("Ninguna red disponible");
  return false;
}

// ═══════════════════════════════════════════════════════════════════════
// ── 9. detectTouchEvent() — máquina de estados del touch ──────────────
//  Timings: tap < SHORT_TAP_MS   window = TAP_WINDOW_MS   hold ≥ HOLD_MS
//  Copiado de 11_codigo_funcional_transcripcion + spec (300ms/400ms)
// ═══════════════════════════════════════════════════════════════════════

void detectTouchEvent() {
  unsigned long now = millis();
  bool cur = digitalRead(TOUCH_PIN);

  ev_shortTap = ev_longHold = ev_tapsReady = false;
  ev_tapCount = 0;
  ev_tap1 = ev_tap2 = ev_tap3 = false;

  if (cur && !tc_last) {
    tc_downAt = now;
    tc_held   = false;
  }

  if (cur && (now - tc_downAt >= HOLD_MS)) {
    tc_held = true; ev_isHolding = true;
  } else if (!cur) {
    ev_isHolding = false;
  }

  if (!cur && tc_last) {
    unsigned long held = now - tc_downAt;
    if (held >= HOLD_MS) {
      ev_longHold = true;
      tc_tapCount = 0;
    } else {
      if (held < SHORT_TAP_MS) ev_shortTap = true;
      tc_tapCount++;
      tc_lastTapAt = now;
    }
    tc_held = false;
  }

  if (tc_tapCount > 0 && !cur && (now - tc_lastTapAt > TAP_WINDOW_MS)) {
    ev_tapsReady = true;
    ev_tapCount  = tc_tapCount;
    tc_tapCount  = 0;
    ev_tap1 = (ev_tapCount == 1);
    ev_tap2 = (ev_tapCount == 2);
    ev_tap3 = (ev_tapCount >= 3);
  }

  tc_last = cur;
}

// ═══════════════════════════════════════════════════════════════════════
// ── 9. runMenuMode() ──────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

void runMenuMode() {
  // tap1 → siguiente opción, tap2 → acceder, hold → MIC (ya manejado en loop)
  if (ev_tap1) menuCursor = (menuCursor + 1) % 7;  // 7 modos navegables
  if (ev_tap2) {
    Mode destMode = (Mode)(menuCursor + 1);  // +1 porque MODE_MENU=0
    currentMode = destMode;
    buzzerBeep(1000, 80);
    Serial.printf("Menu → modo %d\n", (int)destMode);
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(45, 0); display.print("MENU");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  int visible = 5;
  int start   = max(0, min(menuCursor - 1, 7 - visible));
  for (int i = 0; i < visible && (start + i) < 7; i++) {
    int idx = start + i;
    int y   = 12 + i * 10;
    if (idx == menuCursor) {
      display.fillRect(0, y - 1, 127, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(4, y);
    display.print(modeNames[idx]);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 57); display.print("1t:nav  2t:sel");
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════
// ── 10. Eye physics + runMascotaMode() ────────────────────────────────
//  Eye struct copiado exactamente de 11_codigo_funcional_transcripcion
// ═══════════════════════════════════════════════════════════════════════

// — Copia exacta de drawEyelidMask de 11_codigo_funcional_transcripcion —
void drawEyelidMask(float x,float y,float w,float h,int mood,bool isLeft){
  int ix=(int)x,iy=(int)y,iw=(int)w,ih=(int)h;
  if(mood==MOOD_ANGRY){
    if(isLeft) for(int i=0;i<16;i++) display.drawLine(ix,iy+i,  ix+iw,iy-6+i,SSD1306_BLACK);
    else       for(int i=0;i<16;i++) display.drawLine(ix,iy-6+i,ix+iw,iy+i,  SSD1306_BLACK);
  } else if(mood==MOOD_SAD){
    if(isLeft) for(int i=0;i<16;i++) display.drawLine(ix,iy-6+i,ix+iw,iy+i,  SSD1306_BLACK);
    else       for(int i=0;i<16;i++) display.drawLine(ix,iy+i,  ix+iw,iy-6+i,SSD1306_BLACK);
  } else if(mood==MOOD_HAPPY||mood==MOOD_LOVE||mood==MOOD_EXCITED){
    display.fillRect(ix,iy+ih-12,iw,14,SSD1306_BLACK);
    display.fillCircle(ix+iw/2,iy+ih+6,(int)(iw/1.3f),SSD1306_BLACK);
  } else if(mood==MOOD_SLEEPY){
    display.fillRect(ix,iy,iw,ih/2+2,SSD1306_BLACK);
  } else if(mood==MOOD_SUSPICIOUS){
    if(isLeft) display.fillRect(ix,iy,      iw,ih/2-2,SSD1306_BLACK);
    else       display.fillRect(ix,iy+ih-8, iw,8,     SSD1306_BLACK);
  }
}

// — Copia exacta de drawEye de 11_codigo_funcional_transcripcion —
void drawEye(Eye& e,bool isLeft){
  int ix=(int)e.x,iy=(int)e.y,iw=(int)e.w,ih=(int)e.h,r=(iw<20)?3:8;
  display.fillRoundRect(ix,iy,iw,ih,r,SSD1306_WHITE);
  int pw=(int)(iw/2.2f),ph=(int)(ih/2.2f),cx=ix+iw/2,cy=iy+ih/2;
  int px=constrain(cx+(int)e.pupilX-(pw/2),ix,ix+iw-pw);
  int py=constrain(cy+(int)e.pupilY-(ph/2),iy,iy+ih-ph);
  display.fillRoundRect(px,py,pw,ph,r/2,SSD1306_BLACK);
  if(iw>15&&ih>15) display.fillCircle(px+pw-4,py+4,2,SSD1306_WHITE);
  drawEyelidMask(e.x,e.y,e.w,e.h,currentMood,isLeft);
}

// — Copia exacta de updateEyes de 11_codigo_funcional_transcripcion —
void updateEyes(){
  unsigned long now=millis();
  breathVal=sinf(now/800.0f)*1.5f;

  if(now>leftEye.nextBlinkTime){
    leftEye.blinking=rightEye.blinking=true;
    leftEye.lastBlink=now;
    leftEye.nextBlinkTime=now+random(2000,6000);
  }
  if(leftEye.blinking){
    leftEye.targetH=rightEye.targetH=2;
    if(now-leftEye.lastBlink>120) leftEye.blinking=rightEye.blinking=false;
  }

  if(!leftEye.blinking&&(now-lastSaccade)>saccadeInterval&&currentMood!=MOOD_DIZZY){
    lastSaccade=now; saccadeInterval=random(500,3000);
    float lx=(random(0,3)-1)*6.0f, ly=(random(0,3)-1)*4.0f;
    leftEye.targetPupilX=lx;  leftEye.targetPupilY=ly;
    rightEye.targetPupilX=lx; rightEye.targetPupilY=ly;
  }

  if(!leftEye.blinking){
    float bW=36,bH=36+breathVal;
    switch(currentMood){
      case MOOD_NORMAL:                leftEye.targetW=bW; leftEye.targetH=bH; rightEye.targetW=bW; rightEye.targetH=bH; break;
      case MOOD_HAPPY: case MOOD_LOVE: leftEye.targetW=40; leftEye.targetH=32; rightEye.targetW=40; rightEye.targetH=32; break;
      case MOOD_SURPRISED:             leftEye.targetW=30; leftEye.targetH=45; rightEye.targetW=30; rightEye.targetH=45; break;
      case MOOD_SLEEPY:                leftEye.targetW=38; leftEye.targetH=30; rightEye.targetW=38; rightEye.targetH=30; break;
      case MOOD_ANGRY:                 leftEye.targetW=34; leftEye.targetH=32; rightEye.targetW=34; rightEye.targetH=32; break;
      case MOOD_SAD:                   leftEye.targetW=34; leftEye.targetH=40; rightEye.targetW=34; rightEye.targetH=40; break;
      case MOOD_SUSPICIOUS:            leftEye.targetW=36; leftEye.targetH=20; rightEye.targetW=36; rightEye.targetH=42; break;
      case MOOD_EXCITED:               leftEye.targetW=42; leftEye.targetH=38; rightEye.targetW=42; rightEye.targetH=38; break;
      case MOOD_DIZZY:
        leftEye.targetW=34+sinf(now/80.0f)*6;  leftEye.targetH=34+cosf(now/80.0f)*6;
        rightEye.targetW=34+cosf(now/80.0f)*6; rightEye.targetH=34+sinf(now/80.0f)*6;
        leftEye.targetPupilX=sinf(now/150.0f)*12;       leftEye.targetPupilY=cosf(now/150.0f)*12;
        rightEye.targetPupilX=sinf((now/150.0f)+PI)*12; rightEye.targetPupilY=cosf((now/150.0f)+PI)*12;
        break;
    }
  }
  leftEye.update(); rightEye.update();
}

// — Copia exacta de renderEyes de 11_codigo_funcional_transcripcion —
void renderEyes(const char* label){
  display.clearDisplay();
  updateEyes();
  drawEye(leftEye, true);
  drawEye(rightEye, false);
  display.fillRect(0, 54, 128, 10, SSD1306_BLACK);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  int lw = strlen(label)*6;
  display.setCursor((128-lw)/2, 55);
  display.print(label);
  display.display();
}

// CAMBIO PERMITIDO #4: hold → cariñoso inmediato (usando ev_isHolding)
void runMascotaMode() {
  holdMicStarted = false;  // limpiar residuo de hold en otro modo
  // Reaccionar a ruido fuerte
  if (!isStartled) {
    int32_t buf[128]; size_t br = 0;
    i2s_read(I2S_PORT, buf, sizeof(buf), &br, 0);
    if (br > 0) {
      int n=br/4; int32_t mx=INT_MIN, mn=INT_MAX;
      for(int i=0;i<n;i++){
        int32_t v=buf[i]>>GAIN_SHIFT;
        if(v>mx) mx=v; if(v<mn) mn=v;
      }
      int amp = mx - mn;
      if (amp > 4000) {
        moodBeforeNoise = currentMood;
        int r[] = {MOOD_SURPRISED,MOOD_ANGRY,MOOD_DIZZY};
        currentMood = r[esp_random()%3];
        isStartled = true; lastLoudNoiseTime = millis();
      }
    }
  }
  if (isStartled && millis() - lastLoudNoiseTime > 2000) {
    currentMood = moodBeforeNoise; isStartled = false;
  }

  // CAMBIO PERMITIDO #4: hold (ev_isHolding) activa cariñoso inmediato
  if (ev_isHolding && mascState == MASC_NORMAL) {
    mascState      = MASC_CARINHOSO;
    mascStateUntil = millis() + 3000;
    currentMood    = MOOD_LOVE;

    unsigned long now = millis();
    if (now - firstHoldTime > 60000) { holdCount = 0; firstHoldTime = now; }
    holdCount++;
    if (holdCount >= 5) {
      mascState      = MASC_ENOJADO;
      mascStateUntil = millis() + 3000;
      currentMood    = MOOD_ANGRY;
      holdCount      = 0;
      buzzerBeep(300, 400);
    }
  }

  if ((mascState == MASC_CARINHOSO || mascState == MASC_ENOJADO) &&
      millis() > mascStateUntil) {
    mascState   = MASC_NORMAL;
    currentMood = MOOD_NORMAL;
  }

  // tap1 → cambiar mood
  if (ev_tap1 && mascState == MASC_NORMAL) {
    int n; do { n = esp_random() % MOOD_COUNT; } while (n == currentMood);
    currentMood = n;
  }

  renderEyes("");
}

// ═══════════════════════════════════════════════════════════════════════
// ── 11. runGifMode() — CAMBIO PERMITIDO #3: control en runGifMode ──────
//  Archivo se cierra antes de entrar al loop de espera.
//  playNextFrame() solo lee un frame; no maneja touch.
// ═══════════════════════════════════════════════════════════════════════

void loadGifList() {
  NUM_GIFS = 0;
  File root = LittleFS.open("/");
  if (!root || !root.isDirectory()) return;
  File f = root.openNextFile();
  while (f && NUM_GIFS < MAX_GIFS) {
    String name = String(f.name());
    // f.name() puede venir como "cafe.bin" o "/cafe.bin"
    if (name.endsWith(".bin")) {
      if (!name.startsWith("/")) name = "/" + name;
      strlcpy(gifPaths[NUM_GIFS], name.c_str(), sizeof(gifPaths[NUM_GIFS]));
      Serial.printf("GIF[%d]: %s\n", NUM_GIFS, gifPaths[NUM_GIFS]);
      NUM_GIFS++;
    }
    f = root.openNextFile();
  }
  Serial.printf("GIFs encontrados: %d\n", NUM_GIFS);
}

void gifOpenFile(int idx) {
  if (gifFileOpen) { gifFile.close(); gifFileOpen = false; }
  if (!fsOk) return;
  gifFile = LittleFS.open(gifPaths[idx], "r");
  gifFileOpen = (bool)gifFile;
  if (!gifFileOpen) Serial.printf("GIF no encontrado: %s\n", gifPaths[idx]);
}

// Lee y renderiza un frame. Devuelve false si llegó al EOF.
bool gifNextFrame() {
  if (!gifFileOpen || !gifFile) return false;
  uint8_t buf[FRAME_SIZE];
  size_t n = gifFile.read(buf, FRAME_SIZE);
  if (n < FRAME_SIZE) return false;
  // SH1106G no expone getBuffer() igual que SSD1306 — usar drawPixel
  display.clearDisplay();
  for (int p = 0; p < 8; p++)
    for (int c = 0; c < 128; c++) {
      uint8_t byte = buf[p * 128 + c];
      for (int b = 0; b < 8; b++)
        if (byte & (1 << b))
          display.drawPixel(c, p * 8 + b, SSD1306_WHITE);
    }
  display.display();
  return true;
}

void runGifMode() {
  if (!fsOk) {
    showMsg("LittleFS no montado", "Sube data/ primero");
    return;
  }

  // tap1 → pausar / reanudar
  if (ev_tap1) {
    if (!gifPaused) {
      // CAMBIO PERMITIDO #3: cerrar limpiamente antes de esperar
      if (gifFileOpen) { gifFile.close(); gifFileOpen = false; }
      gifPaused = true;
    } else {
      gifOpenFile(gifIdx);   // reabrir desde frame 0
      gifPaused = false;
    }
  }

  // tap2 → siguiente GIF
  if (ev_tap2) {
    gifIdx = (gifIdx + 1) % NUM_GIFS;
    gifOpenFile(gifIdx);
    gifPaused = false;
  }

  if (gifPaused) {
    static unsigned long lastPauseRefresh = 0;
    if (millis() - lastPauseRefresh > 250) {
      lastPauseRefresh = millis();
      display.clearDisplay();
      display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
      display.setCursor(42, 24); display.print("PAUSA");
      display.setCursor(4,  42); display.print("1t:play  2t:siguiente");
      display.display();
    }
    return;
  }

  if (!gifFileOpen) gifOpenFile(gifIdx);
  if (!gifFileOpen) { showMsg("Sin GIF", gifPaths[gifIdx]); return; }

  if (millis() - gifLastFrame >= GIF_FRAME_DELAY) {
    gifLastFrame = millis();
    if (!gifNextFrame()) {
      // EOF: avanzar al siguiente
      gifIdx = (gifIdx + 1) % NUM_GIFS;
      gifOpenFile(gifIdx);
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════
// ── 12. runHoraMode() ─────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

void runHoraMode() {
  if (ev_tap1) showSeconds = !showSeconds;

  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);

  if (!timeSynced && WiFi.status() == WL_CONNECTED) {
    struct tm ti;
    if (getLocalTime(&ti, 2000)) timeSynced = true;
  }

  if (timeSynced) {
    struct tm ti;
    if (getLocalTime(&ti)) {
      char tb[12], db[24];
      if (showSeconds) snprintf(tb, sizeof(tb), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
      else             snprintf(tb, sizeof(tb), "%02d:%02d",      ti.tm_hour, ti.tm_min);
      snprintf(db, sizeof(db), "%s %02d %s",
        diasSemana[ti.tm_wday], ti.tm_mday, mesesAnio[ti.tm_mon]);
      int tsize = showSeconds ? 2 : 3;
      display.setTextSize(tsize);
      int tw = (int)strlen(tb) * (tsize == 3 ? 18 : 12);
      display.setCursor((SCREEN_WIDTH - tw) / 2, 8);
      display.print(tb);
      display.setTextSize(1);
      int dw = (int)strlen(db) * 6;
      display.setCursor((SCREEN_WIDTH - dw) / 2, 52);
      display.print(db);
    }
  } else {
    display.setTextSize(1);
    display.setCursor(5, 20); display.print(WiFi.status() == WL_CONNECTED ?
      "Sincronizando NTP..." : "Sin WiFi");
    display.setCursor(5, 36); display.print("1t: toggle segundos");
  }
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════
// ── 13. Clima — copiado de 08_prueba_clima.ino ────────────────────────
//  CAMBIO PERMITIDO #1: eliminado fetchForecast() y draw3DayView().
//  Solo vista "actual" y vista "hoy".
//  JSON: StaticJsonDocument con filtro (reducción de memoria).
// ═══════════════════════════════════════════════════════════════════════

// — Copia exacta de weatherCategory —
int weatherCategory(int id, bool isNight) {
  if (id == 800)               return isNight ? 1 : 0;
  if (id == 801 || id == 802)  return isNight ? 1 : 2;
  if (id == 803 || id == 804)  return 3;
  if (id >= 200 && id < 300)   return 6;
  if (id >= 300 && id < 600)   return 4;
  if (id >= 600 && id < 700)   return 5;
  return 3;
}

// — Copia exacta de drawIconLarge de 08_prueba_clima —
void drawIconLarge(int cx, int cy, int cat) {
  switch (cat) {
    case 0:
      display.fillCircle(cx, cy, 8, SSD1306_WHITE);
      for (int a=0;a<360;a+=45){ float r=a*PI/180.0f;
        display.drawLine(cx+(int)(11*cosf(r)),cy+(int)(11*sinf(r)),
                         cx+(int)(15*cosf(r)),cy+(int)(15*sinf(r)),SSD1306_WHITE); }
      break;
    case 1:
      display.fillCircle(cx-1,cy,11,SSD1306_WHITE);
      display.fillCircle(cx+5,cy-4,9,SSD1306_BLACK); break;
    case 2:
      display.fillCircle(cx+7,cy-8,6,SSD1306_WHITE);
      for (int a=0;a<360;a+=60){ float r=a*PI/180.0f;
        display.drawLine((cx+7)+(int)(8*cosf(r)),(cy-8)+(int)(8*sinf(r)),
                         (cx+7)+(int)(11*cosf(r)),(cy-8)+(int)(11*sinf(r)),SSD1306_WHITE); }
      display.fillRoundRect(cx-14,cy+1,28,12,5,SSD1306_WHITE);
      display.fillCircle(cx-4,cy+2,8,SSD1306_WHITE);
      display.fillCircle(cx+5,cy-1,7,SSD1306_WHITE); break;
    case 3:
      display.fillRoundRect(cx-12,cy-1,26,12,5,SSD1306_WHITE);
      display.fillCircle(cx-3,cy-2,9,SSD1306_WHITE);
      display.fillCircle(cx+6,cy-5,7,SSD1306_WHITE); break;
    case 4:
      display.fillRoundRect(cx-12,cy-8,26,11,4,SSD1306_WHITE);
      display.fillCircle(cx-4,cy-9,7,SSD1306_WHITE);
      display.fillCircle(cx+6,cy-11,6,SSD1306_WHITE);
      for (int i=0;i<4;i++){ int dx=cx-11+i*8;
        display.drawLine(dx,cy+5,dx-2,cy+12,SSD1306_WHITE); } break;
    case 5:
      display.drawLine(cx,cy-13,cx,cy+13,SSD1306_WHITE);
      display.drawLine(cx-13,cy,cx+13,cy,SSD1306_WHITE);
      display.drawLine(cx-9,cy-9,cx+9,cy+9,SSD1306_WHITE);
      display.drawLine(cx+9,cy-9,cx-9,cy+9,SSD1306_WHITE);
      display.fillCircle(cx,cy,3,SSD1306_WHITE);
      for(int dx=-12;dx<=12;dx+=12) display.fillCircle(cx+dx,cy,2,SSD1306_WHITE);
      for(int dy=-12;dy<=12;dy+=12) display.fillCircle(cx,cy+dy,2,SSD1306_WHITE);
      break;
    case 6:
      display.fillRoundRect(cx-12,cy-11,26,11,4,SSD1306_WHITE);
      display.fillCircle(cx-4,cy-12,7,SSD1306_WHITE);
      display.fillCircle(cx+6,cy-14,6,SSD1306_WHITE);
      display.drawLine(cx-1,cy+2,cx-6,cy+11,SSD1306_WHITE);
      display.drawLine(cx-6,cy+11,cx+2,cy+9,SSD1306_WHITE);
      display.drawLine(cx+2,cy+9,cx-3,cy+17,SSD1306_WHITE); break;
  }
}

// — Copia exacta de drawIconSmall de 08_prueba_clima —
void drawIconSmall(int cx, int cy, int cat) {
  switch (cat) {
    case 0:
      display.fillCircle(cx,cy,4,SSD1306_WHITE);
      for(int a=0;a<360;a+=45){float r=a*PI/180.0f;
        display.drawLine(cx+(int)(6*cosf(r)),cy+(int)(6*sinf(r)),
                         cx+(int)(8*cosf(r)),cy+(int)(8*sinf(r)),SSD1306_WHITE);} break;
    case 1:
      display.fillCircle(cx,cy,5,SSD1306_WHITE);
      display.fillCircle(cx+3,cy-2,4,SSD1306_BLACK); break;
    case 2:
      display.fillCircle(cx+3,cy-4,3,SSD1306_WHITE);
      display.fillRoundRect(cx-6,cy,12,6,2,SSD1306_WHITE);
      display.fillCircle(cx-2,cy+1,4,SSD1306_WHITE);
      display.fillCircle(cx+3,cy-1,3,SSD1306_WHITE); break;
    case 3:
      display.fillRoundRect(cx-6,cy-2,12,7,2,SSD1306_WHITE);
      display.fillCircle(cx-2,cy-2,4,SSD1306_WHITE);
      display.fillCircle(cx+3,cy-4,3,SSD1306_WHITE); break;
    case 4:
      display.fillRoundRect(cx-5,cy-5,10,5,2,SSD1306_WHITE);
      display.fillCircle(cx-2,cy-5,3,SSD1306_WHITE);
      display.drawLine(cx-3,cy+2,cx-4,cy+6,SSD1306_WHITE);
      display.drawLine(cx,cy+2,cx-1,cy+6,SSD1306_WHITE);
      display.drawLine(cx+3,cy+2,cx+2,cy+6,SSD1306_WHITE); break;
    case 5:
      display.drawLine(cx,cy-6,cx,cy+6,SSD1306_WHITE);
      display.drawLine(cx-6,cy,cx+6,cy,SSD1306_WHITE);
      display.drawLine(cx-4,cy-4,cx+4,cy+4,SSD1306_WHITE);
      display.drawLine(cx+4,cy-4,cx-4,cy+4,SSD1306_WHITE);
      display.fillCircle(cx,cy,2,SSD1306_WHITE); break;
    case 6:
      display.fillRoundRect(cx-5,cy-6,10,5,2,SSD1306_WHITE);
      display.fillCircle(cx-2,cy-6,3,SSD1306_WHITE);
      display.drawLine(cx,cy+1,cx-2,cy+5,SSD1306_WHITE);
      display.drawLine(cx-2,cy+5,cx+1,cy+4,SSD1306_WHITE);
      display.drawLine(cx+1,cy+4,cx-1,cy+8,SSD1306_WHITE); break;
  }
}

// — fetchCurrent: CAMBIO PERMITIDO #1: StaticJsonDocument con filtro —
bool fetchCurrent() {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClient client;
  HTTPClient http;
  char url[280];
  snprintf(url, sizeof(url),
    "http://api.openweathermap.org/data/2.5/weather"
    "?q=%s,%s&appid=%s&units=%s&lang=es",
    OWM_CITY, OWM_COUNTRY, OWM_API_KEY, OWM_UNITS);

  http.begin(client, url); http.setTimeout(8000);
  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String body = http.getString(); http.end();

  // Filtro ArduinoJson: solo campos necesarios (reducción de RAM)
  StaticJsonDocument<200> filter;
  filter["main"]["temp"] = true;
  filter["main"]["temp_min"] = true;
  filter["main"]["temp_max"] = true;
  filter["main"]["humidity"] = true;
  filter["wind"]["speed"]    = true;
  filter["weather"][0]["id"] = true;
  filter["weather"][0]["description"] = true;
  filter["weather"][0]["icon"] = true;
  filter["name"] = true;
  filter["dt"]   = true;
  filter["timezone"] = true;

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
  if (deserializeJson(doc, body.c_str(),
        DeserializationOption::Filter(filter)) != DeserializationError::Ok) return false;
#else
  DynamicJsonDocument doc(512);
  if (deserializeJson(doc, body.c_str(),
        DeserializationOption::Filter(filter)) != DeserializationError::Ok) return false;
#endif

  wx.temp      = doc["main"]["temp"]     | 0.0f;
  wx.tempMin   = doc["main"]["temp_min"] | 0.0f;
  wx.tempMax   = doc["main"]["temp_max"] | 0.0f;
  wx.humidity  = doc["main"]["humidity"] | 0.0f;
  wx.windSpeed = doc["wind"]["speed"]    | 0.0f;
  wx.weatherId = doc["weather"][0]["id"] | 800;

  const char* rawDesc = doc["weather"][0]["description"] | "---";
  strlcpy(wx.desc, rawDesc, sizeof(wx.desc));
  if (wx.desc[0] >= 'a') wx.desc[0] -= 32;

  strlcpy(wx.city, doc["name"] | OWM_CITY, sizeof(wx.city));

  const char* icon = doc["weather"][0]["icon"] | "01d";
  wx.isNight = (strlen(icon) >= 3 && icon[2] == 'n');

  long dt = doc["dt"] | 0L, tz = doc["timezone"] | GMT_OFFSET;
  long local = dt + tz;
  snprintf(wx.timeStr, sizeof(wx.timeStr), "%02d:%02d",
           (int)((local/3600)%24), (int)((local/60)%60));

  wx.valid = true; wx.fetchedAt = millis();
  return true;
}

// — Copia exacta de drawCurrentView de 08_prueba_clima (adaptada a SSD1306_WHITE) —
void drawCurrentView() {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  int cat = weatherCategory(wx.weatherId, wx.isNight);
  drawIconLarge(20, 26, cat);
  display.setTextSize(1);
  char city[13]; strlcpy(city, wx.city, sizeof(city));
  display.setCursor(44, 1); display.print(city);
  display.setTextSize(2);
  display.setCursor(44, 13); display.print((int)roundf(wx.temp));
  display.setTextSize(1);    display.print("\xB0""C");
  display.setTextSize(1);
  display.setCursor(44, 32); display.print(wx.desc);
  display.drawFastHLine(0, 46, SCREEN_WIDTH, SSD1306_WHITE);
  display.setCursor(0, 53);  display.print(wx.timeStr);
  display.setCursor(38, 53); display.print("\xB7 "); display.print((int)wx.humidity); display.print("%");
  char wind[10]; snprintf(wind, sizeof(wind), "\xB7 %.1fm/s", wx.windSpeed);
  display.setCursor(76, 53); display.print(wind);
  display.display();
}

// — Copia exacta de drawTodayView de 08_prueba_clima —
void drawTodayView() {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  int cat = weatherCategory(wx.weatherId, false);
  display.setTextSize(1); display.setCursor(0, 1);
  display.print("HOY  "); display.print(wx.city);
  drawIconLarge(64, 28, cat);
  char mm[24];
  snprintf(mm, sizeof(mm), "Min %.0f\xB0   Max %.0f\xB0", wx.tempMin, wx.tempMax);
  display.setCursor(0, 47); display.print(mm);
  display.setCursor(0, 57); display.print(wx.desc);
  display.display();
}

void runClimaMode() {
  if (ev_tap1) weatherView = (weatherView + 1) % 2;  // solo 2 vistas (sin 3 días)

  // Auto-refrescar cada 10 min
  if (millis() - wx.fetchedAt > WEATHER_UPDATE_MS || !wx.valid) {
    showMsg("Obteniendo clima...");
    fetchCurrent();
  }

  if (!wx.valid) {
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(10, 18); display.print("Sin datos del clima");
    display.setCursor(0,  32); display.print("Verifica OWM_API_KEY");
    display.setCursor(0,  46); display.print("y WIFI_SSID");
    display.display();
    return;
  }

  if (weatherView == 0) drawCurrentView();
  else                  drawTodayView();
}

// ═══════════════════════════════════════════════════════════════════════
// ── 14. runPomodoroMode() ─────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

void runPomodoroMode() {
  // Submodo ajuste
  if (pomoState == POMO_ADJUST) {
    if (ev_tap1) {
      if (adjustOp == 0) pomoSeconds += 60;
      else               pomoSeconds = max(60, pomoSeconds - 60);
    }
    if (ev_tap2) adjustOp ^= 1;
    if (ev_tap3) { pomoState = POMO_STOPPED; return; }

    // hold auto-incremento (POMO_ADJUST intercepta hold en loop())
    if (ev_isHolding && millis() - lastAdjustAuto > 300) {
      if (adjustOp == 0) pomoSeconds += 60;
      else               pomoSeconds = max(60, pomoSeconds - 60);
      lastAdjustAuto = millis();
    }

    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(15, 0); display.print("AJUSTE DE TIEMPO");
    display.setTextSize(2);
    char tb[8]; snprintf(tb, sizeof(tb), "%02d:%02d", pomoSeconds/60, pomoSeconds%60);
    display.setCursor(20, 18); display.print(tb);
    display.setTextSize(1);
    display.setCursor(0, 44);
    display.print(adjustOp == 0 ? "> SUMAR   restar" : "  sumar  > RESTAR");
    display.setCursor(0, 55); display.print("1t:+/-  2t:op  3t:ok");
    display.display();
    return;
  }

  // tap1 → pausar/reanudar
  if (ev_tap1) {
    if (pomoState == POMO_STOPPED) { pomoState = POMO_RUNNING; lastPomoTick = millis(); }
    else                           { pomoState = POMO_STOPPED; }
  }
  // tap2 → entrar a ajuste
  if (ev_tap2) { pomoState = POMO_ADJUST; adjustOp = 0; lastAdjustAuto = millis(); return; }

  // Temporizador
  if (pomoState == POMO_RUNNING) {
    unsigned long now = millis();
    if (now - lastPomoTick >= 1000) {
      lastPomoTick = now;
      if (pomoSeconds > 0) {
        pomoSeconds--;
      } else {
        pomoState   = POMO_STOPPED;
        pomoSeconds = 25 * 60;
        // BUZZER: 3 beeps cortos (spec)
        buzzerBeep(880, 150); delay(180);
        buzzerBeep(880, 150); delay(180);
        buzzerBeep(1046, 250);
      }
    }
  }

  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(25, 4); display.print("MODO ENFOQUE");
  char tb[8]; snprintf(tb, sizeof(tb), "%02d:%02d", pomoSeconds/60, pomoSeconds%60);
  display.setTextSize(3);
  display.setCursor((SCREEN_WIDTH-(int)strlen(tb)*18)/2, 20); display.print(tb);
  display.setTextSize(1);
  if (pomoState == POMO_STOPPED) { display.setCursor(42,55); display.print("PAUSADO"); }
  else                           { display.setCursor(35,55); display.print("ENFOCADO"); }
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════
// ── 15. Canvas via WebSocket (copiado de 12_prueba_canvas) ────────────
//  wss://dasaimochiservidor.../ws?device_id=DEVICE_ID  puerto 443
//  Mensajes: {"type":"canvas_update","bitmap":"<base64 1024 bytes>"}
//            {"type":"canvas_exit"}
// ═══════════════════════════════════════════════════════════════════════

void drawBitmap(const uint8_t* bitmap) {
  display.clearDisplay();
  for (int page = 0; page < 8; page++) {
    for (int col = 0; col < 128; col++) {
      uint8_t byte = bitmap[page * 128 + col];
      for (int bit = 0; bit < 8; bit++) {
        if (byte & (1 << bit)) {
          display.drawPixel(col, page * 8 + bit, SSD1306_WHITE);
        }
      }
    }
  }
  display.display();
}

// Copia exacta de b64val / base64Decode de 12_prueba_canvas
static int b64val(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}
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

void handleWsText(uint8_t* payload, size_t length) {
  char* p = (char*)payload;

  if (strstr(p, "\"canvas_update\"")) {
    if (currentMode != MODE_CANVAS) return;  // guard restaurado
    char* b64start = strstr(p, "\"bitmap\":\"");
    if (!b64start) return;
    b64start += 10;
    char* b64end = strchr(b64start, '"');
    if (!b64end) return;
    *b64end = '\0';

    size_t decoded = base64Decode(b64start, canvasBitmap, sizeof(canvasBitmap));
    Serial.printf("[WS] canvas_update — %d bytes decodificados\n", decoded);
    if (decoded == FRAME_SIZE) {
      lastMode     = currentMode;   // guardar modo actual antes de cambiar
      canvasActive = true;
      currentMode  = MODE_CANVAS;   // forzar modo canvas
      drawBitmap(canvasBitmap);
    } else {
      Serial.printf("[WS] bitmap incorrecto: esperaba %d, recibí %d\n", FRAME_SIZE, decoded);
    }
    return;
  }

  if (strstr(p, "\"canvas_exit\"")) {
    Serial.println("[WS] canvas_exit — restaurando modo anterior");
    canvasActive = false;
    currentMode  = (lastMode != MODE_CANVAS) ? lastMode : MODE_MASCOTA;
  }
}

// Copia exacta de onWsEvent de 12_prueba_canvas
void onWsEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      wsConnected = false;
      Serial.println("[WS] Desconectado");
      break;
    case WStype_CONNECTED:
      wsConnected = true;
      Serial.printf("[WS] Conectado a wss://%s%s\n", WS_HOST, wsPath);
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

// Copia exacta de initWebSocket de 12_prueba_canvas
void initWebSocket() {
  // Despertar el servidor antes de conectar WebSocket
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure tls; tls.setInsecure();
    HTTPClient http;
    http.begin(tls, SERVER_URL);
    http.setTimeout(10000);
    int code = http.GET();
    Serial.printf("Wake server: %d\n", code);
    http.end();
  }

  snprintf(wsPath, sizeof(wsPath), "/ws?device_id=%s", DEVICE_ID);
  webSocket.onEvent(onWsEvent);
  webSocket.beginSSL(WS_HOST, WS_PORT, wsPath);
  webSocket.setReconnectInterval(5000);
  Serial.printf("WebSocket → wss://%s:%d%s\n", WS_HOST, WS_PORT, wsPath);
}

void runCanvasMode() {
  if (canvasActive) return;  // bitmap activo — no sobreescribir con pantalla de estado
  static unsigned long lastRefresh = 0;
  if (millis() - lastRefresh > 500) {
    lastRefresh = millis();
    display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
    display.setCursor(25, 4);  display.print("OLED CANVAS");
    display.drawFastHLine(0, 14, 128, SSD1306_WHITE);
    const char* wsLine = wsConnected ? "WS: conectado" : "WS: esperando...";
    display.setCursor(0, 24); display.print(wsLine);
    if (WiFi.status() == WL_CONNECTED) {
      display.setCursor(0, 36); display.print(WiFi.localIP().toString().c_str());
    }
    display.setCursor(0, 54); display.print("usa canvas.html");
    display.display();
  }
}

// ═══════════════════════════════════════════════════════════════════════
// ── 16. runMicMode() + sendAudioTask() ────────────────────────────────
//  Copiado de 11_codigo_funcional_transcripcion con buffers renombrados
// ═══════════════════════════════════════════════════════════════════════

// — Copia de parseMoodId / parseIntent de 11_codigo_funcional_transcripcion —
int parseMoodId(const char* json) {
  const char* p = strstr(json, "\"mood_id\":");
  if (!p) return -1;
  p += 10; char* end;
  int v = strtol(p, &end, 10);
  return (end > p) ? v : -1;
}
void parseIntent(const char* json, char* out, size_t outLen) {
  const char* p = strstr(json, "\"intent\":\"");
  if (!p) { strlcpy(out, "?", outLen); return; }
  p += 10;
  const char* q = strchr(p, '"');
  if (!q || q <= p) { strlcpy(out, "?", outLen); return; }
  size_t n = q - p; if (n >= outLen) n = outLen - 1;
  memcpy(out, p, n); out[n] = 0;
}

// — parseCommand: sincronizado con intent_map_v2.py ─────────────────────
void parseCommand(const char* cmd) {
  Serial.printf("Comando voz: [%s]\n", cmd);

  // ── Navegación de modos ───────────────────────────────────────────────
  if      (strcmp(cmd,"GOTO_MENU")     == 0) { currentMode = MODE_MENU;     return; }
  else if (strcmp(cmd,"GOTO_MASCOTA")  == 0) { currentMode = MODE_MASCOTA;  return; }
  else if (strcmp(cmd,"GOTO_GIF")      == 0) { currentMode = MODE_GIF;      return; }
  else if (strcmp(cmd,"GOTO_HORA")     == 0) { currentMode = MODE_HORA;     return; }
  else if (strcmp(cmd,"GOTO_CLIMA")    == 0) { currentMode = MODE_CLIMA;    return; }
  else if (strcmp(cmd,"GOTO_POMODORO") == 0) { currentMode = MODE_POMODORO; return; }
  else if (strcmp(cmd,"GOTO_CANVAS")   == 0) { currentMode = MODE_CANVAS;   return; }

  // ── GIF ───────────────────────────────────────────────────────────────
  else if (strcmp(cmd,"GIF_PAUSE") == 0) {
    if (!gifPaused) { if (gifFileOpen) { gifFile.close(); gifFileOpen = false; } gifPaused = true; }
    currentMode = MODE_GIF;
  }
  else if (strcmp(cmd,"GIF_RESUME") == 0) {
    if (gifPaused) { gifOpenFile(gifIdx); gifPaused = false; }
    currentMode = MODE_GIF;
  }
  else if (strcmp(cmd,"GIF_NEXT") == 0) {
    gifIdx = (gifIdx + 1) % NUM_GIFS; gifOpenFile(gifIdx); gifPaused = false;
    currentMode = MODE_GIF;
  }

  // ── Hora ──────────────────────────────────────────────────────────────
  else if (strcmp(cmd,"HORA_SHOW_SECONDS")   == 0) { showSeconds = true;          currentMode = MODE_HORA; }
  else if (strcmp(cmd,"HORA_HIDE_SECONDS")   == 0) { showSeconds = false;         currentMode = MODE_HORA; }
  else if (strcmp(cmd,"HORA_TOGGLE_SECONDS") == 0) { showSeconds = !showSeconds;  currentMode = MODE_HORA; }

  // ── Clima ─────────────────────────────────────────────────────────────
  else if (strcmp(cmd,"CLIMA_VIEW_NOW")   == 0) { weatherView = 0; currentMode = MODE_CLIMA; }
  else if (strcmp(cmd,"CLIMA_VIEW_TODAY") == 0) { weatherView = 1; currentMode = MODE_CLIMA; }
  else if (strcmp(cmd,"CLIMA_REFRESH")    == 0) { wx.fetchedAt = 0; currentMode = MODE_CLIMA; }

  // ── Pomodoro ──────────────────────────────────────────────────────────
  else if (strcmp(cmd,"POMODORO_START") == 0) {
    if (pomoState != POMO_RUNNING) { pomoState = POMO_RUNNING; lastPomoTick = millis(); }
    currentMode = MODE_POMODORO;
  }
  else if (strcmp(cmd,"POMODORO_PAUSE") == 0) {
    if (pomoState == POMO_RUNNING) pomoState = POMO_STOPPED;
    currentMode = MODE_POMODORO;
  }
  else if (strcmp(cmd,"POMODORO_RESUME") == 0) {
    if (pomoState == POMO_STOPPED && pomoSeconds > 0) { pomoState = POMO_RUNNING; lastPomoTick = millis(); }
    currentMode = MODE_POMODORO;
  }
  else if (strcmp(cmd,"POMODORO_RESET") == 0) {
    pomoState = POMO_STOPPED; pomoSeconds = 25 * 60; currentMode = MODE_POMODORO;
  }
  else if (strcmp(cmd,"POMODORO_QUERY")    == 0) { currentMode = MODE_POMODORO; }
  else if (strcmp(cmd,"POMODORO_ADD_TIME") == 0) { pomoSeconds += 60;                        currentMode = MODE_POMODORO; }
  else if (strcmp(cmd,"POMODORO_SUB_TIME") == 0) { pomoSeconds = max(60, pomoSeconds - 60);  currentMode = MODE_POMODORO; }

  // ── Moods ─────────────────────────────────────────────────────────────
  else if (strcmp(cmd,"MOOD_NORMAL")     == 0) currentMood = MOOD_NORMAL;
  else if (strcmp(cmd,"MOOD_HAPPY")      == 0) currentMood = MOOD_HAPPY;
  else if (strcmp(cmd,"MOOD_SURPRISED")  == 0) currentMood = MOOD_SURPRISED;
  else if (strcmp(cmd,"MOOD_SLEEPY")     == 0) currentMood = MOOD_SLEEPY;
  else if (strcmp(cmd,"MOOD_ANGRY")      == 0) currentMood = MOOD_ANGRY;
  else if (strcmp(cmd,"MOOD_SAD")        == 0) currentMood = MOOD_SAD;
  else if (strcmp(cmd,"MOOD_EXCITED")    == 0) currentMood = MOOD_EXCITED;
  else if (strcmp(cmd,"MOOD_LOVE")       == 0) currentMood = MOOD_LOVE;
  else if (strcmp(cmd,"MOOD_SUSPICIOUS") == 0) currentMood = MOOD_SUSPICIOUS;
  else if (strcmp(cmd,"MOOD_DIZZY")      == 0) currentMood = MOOD_DIZZY;

  else Serial.printf("Comando no reconocido: [%s]\n", cmd);
}

// — Copia de httpSendTask de 11_codigo_funcional_transcripcion —
// (usa _fs_buf y _fs_len en lugar de _rec_buf/_rec_len)
void sendAudioTask(void*) {
  Serial.printf("sendAudioTask: %u bytes, session=%s\n", _fs_len, _rec_sid);
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure tls; tls.setInsecure();
    HTTPClient http;
    http.begin(tls, SERVER_URL "/audio/pcm16"); http.setTimeout(15000);
    http.addHeader("device-id",    DEVICE_ID);
    http.addHeader("session-id",   _rec_sid);
    http.addHeader("end",          "true");
    http.addHeader("Content-Type", "application/octet-stream");
    _rec_status = http.sendRequest("POST", _fs_buf, _fs_len);
    if (_rec_status >= 200 && _rec_status < 300) {
      String payload = http.getString();
      int mid = parseMoodId(payload.c_str());
      parseIntent(payload.c_str(), _rec_intent, sizeof(_rec_intent));
      if (mid >= 0 && mid < MOOD_COUNT) currentMood = mid;
      Serial.printf("OK mood=%d intent=%s\n", mid, _rec_intent);
    }
    http.end();
  } else {
    _rec_status = -4;
  }
  _rec_done = true; _rec_task = NULL; vTaskDelete(NULL);
}

// — Copia de startRecording de 11_codigo_funcional_transcripcion —
void startRecording() {
  _rec_len       = 0;
  snprintf(_rec_sid, sizeof(_rec_sid), "%lu", millis());
  warmup_complete = false;
  warmup_samples  = 0;
  recRecording    = true;
  currentMood     = MOOD_SUSPICIOUS;
  Serial.printf("Grabando... (session=%s)\n", _rec_sid);
  buzzerBeep(880, 80);
}

// — Copia de stopAndSend de 11_codigo_funcional_transcripcion —
// ADICIÓN: copia acc_buffer → _fs_buf antes de lanzar tarea (spec)
void stopAndSend() {
  recRecording = false;
  _fs_len      = _rec_len;
  memcpy(_fs_buf, acc_buffer, _fs_len);   // doble buffer: acc libre para próxima grabación
  _rec_len     = 0;
  _rec_done    = false; _rec_status = 0; _rec_intent[0] = 0;
  currentMood  = MOOD_DIZZY;
  webSocket.disconnect();
  delay(100);
  xTaskCreatePinnedToCore(sendAudioTask, "sendAudio", 8192, NULL, 1, &_rec_task, 0);
  Serial.printf("Enviando %u bytes...\n", _fs_len);
}

// — convert_i2s32_to_pcm16: copia exacta de 11_codigo_funcional_transcripcion —
size_t convert_i2s32_to_pcm16(const uint8_t* input, size_t input_len, uint8_t* output) {
  const int32_t* samples32 = (const int32_t*)input;
  int16_t* samples16 = (int16_t*)output;
  size_t num_samples = input_len / 4;
  if (num_samples == 0) return 0;

  static int64_t dc_ema = 0;
  for (size_t i = 0; i < num_samples; i++) {
    int64_t s = (int64_t)samples32[i];
    dc_ema = ((dc_ema * 63) + s) >> 6;
    int32_t centered = (int32_t)(s - dc_ema);
    int32_t v = centered >> GAIN_SHIFT;
    if (v >  32767) v =  32767;
    if (v < -32768) v = -32768;
    samples16[i] = (int16_t)v;
  }
  return num_samples * 2;
}

enum MicState { MIC_IDLE, MIC_RECORDING, MIC_SENDING, MIC_RESULT, MIC_ERROR };
static MicState micState       = MIC_IDLE;
static unsigned long resultAt  = 0;

void runMicMode() {
  // ── Determinar estado ─────────────────────────────────────────────────
  if      (recRecording) micState = MIC_RECORDING;
  else if (_rec_task)    micState = MIC_SENDING;
  else if (_rec_done && _rec_status >= 200 && _rec_status < 300) micState = MIC_RESULT;
  else if (_rec_done)    micState = MIC_ERROR;
  else                   micState = MIC_IDLE;

  // ── tap1 en IDLE → cancelar, volver al modo anterior ─────────────────
  if (ev_tap1 && micState == MIC_IDLE) {
    holdMicStarted = false;
    currentMode    = lastMode;
    return;
  }

  // ── Segundo hold (primer hold ya terminó) → iniciar grabación ─────────
  if (ev_isHolding && !holdMicStarted && micState == MIC_IDLE && _rec_task == NULL)
    startRecording();

  // ── Fin del segundo hold → detener y enviar ───────────────────────────
  if (ev_longHold && !holdMicStarted && recRecording)
    stopAndSend();

  // ── Leer I2S y acumular (solo mientras graba) ─────────────────────────
  if (recRecording) {
    static int32_t sampleBuf[CHUNK_SIZE / 4];
    static uint8_t pcm16Buf[CHUNK_SIZE / 2];
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, sampleBuf, sizeof(sampleBuf), &bytesRead, pdMS_TO_TICKS(1000));
    if (bytesRead > 0) {
      size_t pcm16Len = convert_i2s32_to_pcm16((const uint8_t*)sampleBuf, bytesRead, pcm16Buf);
      if (pcm16Len > 0) {
        if (!warmup_complete) {
          size_t warmupNeeded = (REC_SAMPLE_RATE * WARMUP_DURATION_MS / 1000) * 2;
          warmup_samples += pcm16Len;
          if (warmup_samples >= warmupNeeded) warmup_complete = true;
        } else {
          size_t space  = REC_AUDIO_BYTES - _rec_len;
          size_t toCopy = pcm16Len < space ? pcm16Len : space;
          memcpy(acc_buffer + _rec_len, pcm16Buf, toCopy);
          _rec_len += toCopy;
          if (_rec_len >= REC_AUDIO_BYTES) stopAndSend();  // auto-stop a los 5s
        }
      }
    }
  }

  // ── Procesar resultado completado ─────────────────────────────────────
  if (_rec_done) {
    _rec_done = false;
    resultAt  = millis();
    if (_rec_status >= 200 && _rec_status < 300 && strlen(_rec_intent) > 0) {
      parseCommand(_rec_intent);
      buzzerBeep(1046, 150);
    } else {
      currentMood = MOOD_SAD;
      buzzerBeep(300, 400);
    }
  }

  // ── Volver a lastMode tras 2s de mostrar resultado ────────────────────
  if ((micState == MIC_RESULT || micState == MIC_ERROR) &&
      millis() - resultAt > 2000 && resultAt > 0) {
    currentMood = MOOD_NORMAL;
    currentMode = lastMode;
    resultAt    = 0;
    return;
  }

  // ── Render ────────────────────────────────────────────────────────────
  switch (micState) {
    case MIC_IDLE: {
      // holdMicStarted=true: primer hold activo (decirle que suelte)
      // holdMicStarted=false: esperando segundo hold para grabar
      renderEyes(holdMicStarted ? "suelta" : "hold:REC");
      break;
    }
    case MIC_RECORDING: {
      int32_t buf[128]; size_t br = 0;
      i2s_read(I2S_PORT, buf, sizeof(buf), &br, 0);
      int amp = 0;
      if (br > 0) { int n=br/4; for(int i=0;i<n;i++) { int a=abs(buf[i]>>GAIN_SHIFT); if(a>amp) amp=a; } }
      int bar = constrain(map(amp, 0, 30000, 0, 124), 0, 124);
      renderEyes("REC");
      display.drawRect(2, 54, 124, 9, SSD1306_WHITE);
      if (bar > 0) display.fillRect(3, 55, bar, 7, SSD1306_WHITE);
      int pct = (int)((_rec_len * 100) / REC_AUDIO_BYTES);
      display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
      if (bar < 60) { display.setCursor(0, 55); display.printf("REC %d%%", pct); }
      display.display();
      break;
    }
    case MIC_SENDING: {
      unsigned long t = millis();
      char dots[5] = ""; int nd = (t / 400) % 4;
      for (int i = 0; i < nd; i++) strncat(dots, ".", 2);
      char label[20]; snprintf(label, sizeof(label), "env%s", dots);
      renderEyes(label);
      break;
    }
    case MIC_RESULT: renderEyes(_rec_intent); break;
    case MIC_ERROR: {
      char label[20]; snprintf(label, sizeof(label), "err%d", _rec_status);
      renderEyes(label);
      break;
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════
// ── 17. Utilidades ────────────────────────────────────────────────────
// ═══════════════════════════════════════════════════════════════════════

void buzzerBeep(int freq, int ms) {
  tone(BUZZER_PIN, freq, ms);
}

void showMsg(const char* l1, const char* l2) {
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  display.setCursor(0, 18); display.print(l1);
  if (l2) { display.setCursor(0, 34); display.print(l2); }
  display.display();
}
