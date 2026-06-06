/*
  codigo_funcional_transcripcion.ino  —  Test de integración completo con OLED
  Prueba el ciclo: toque → grabar → enviar → recibir mood_id → setMood → ojos

  Fases visibles en pantalla:
    LISTO      → ojos NORMAL  + "Toca para grabar"
    GRABANDO   → ojos SUSPICIOUS + barra de nivel
    ENVIANDO   → ojos DIZZY   + puntos animados
    RESULTADO  → ojos según mood_id recibido + intent en texto
    ERROR      → ojos SAD     + código HTTP

  Diferencias respecto a transcripcion.ino:
    - Envía PCM16 crudo (sin cabecera WAV) a un servidor que devuelve mood_id + intent
    - La respuesta no es texto sino un comando que cambia el estado de ánimo del robot
    - Muestra ojos animados con los 10 moods de mochi_unified_5

  Pines (ESP32-C6 Supermini — mismo que mochi_v6):
    OLED  SDA=6  SCL=7  (0x3C)
    Touch TTP223=2
    Buzzer=19
    I2S:  SCK=4  WS=1  SD=3

  CONFIGURACIÓN OBLIGATORIA (editar antes de subir):
    WIFI_SSID  / WIFI_PASS  → nombre y contraseña de tu red WiFi
    SERVER_URL              → URL del servidor que procesa el audio
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <driver/i2s.h>
#include <math.h>

#define SSD1306_WHITE SH110X_WHITE
#define SSD1306_BLACK SH110X_BLACK

// ── Pines ───────────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define SDA_PIN         6
#define SCL_PIN         7
#define TOUCH_PIN       2
#define BUZZER_PIN     19
#define I2S_SCK_PIN     4
#define I2S_WS_PIN      1
#define I2S_SD_PIN      3
#define I2S_PORT  I2S_NUM_0

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Red / Servidor ─────────────────────────────────────────────────────
// EDITAR ESTAS TRES LÍNEAS ANTES DE SUBIR
const char* WIFI_SSID  = "TU_RED_WIFI";
const char* WIFI_PASS  = "TU_CONTRASENA";
const char* SERVER_URL = "https://dasaimochiservidor-production.up.railway.app/audio/pcm16";
const char* DEVICE_ID  = "esp32_01";

// ── Moods ────────────────────────────────────────────────────────────────
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

int  currentMood = MOOD_NORMAL;
float breathVal  = 0;

// ── Eye struct (igual que mochi_v6) ──────────────────────────────────────
struct Eye {
  float x,y,w,h, targetX,targetY,targetW,targetH;
  float pupilX,pupilY, targetPupilX,targetPupilY;
  float velX,velY,velW,velH, pVelX,pVelY;
  float k=0.12f,d=0.60f,pk=0.08f,pd=0.50f;
  bool  blinking=false;
  unsigned long lastBlink=0, nextBlinkTime=0;

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

// ── Grabación ──────────────────────────────────────────────────────────
#define REC_SAMPLE_RATE    8000
#define REC_DURATION_S     5
#define REC_AUDIO_BYTES    ((size_t)REC_DURATION_S * REC_SAMPLE_RATE * 2)
#define CHUNK_SIZE         4096
#define GAIN_SHIFT         14
#define WARMUP_DURATION_MS 100

static uint8_t       _rec_buf[REC_AUDIO_BYTES];
static size_t        _rec_len    = 0;
static String        _rec_sid    = "";
static volatile bool _rec_done   = false;
static volatile int  _rec_status = 0;
static TaskHandle_t  _rec_task   = NULL;
static String        _rec_intent = "";
bool   recRecording    = false;
bool   warmup_complete = false;
size_t warmup_samples  = 0;

// ── Touch ─────────────────────────────────────────────────────────────────
bool          tc_last      = false;
unsigned long tc_downAt    = 0;
unsigned long tc_lastTapAt = 0;
int           tc_tapCount  = 0;
bool          tc_held      = false;

#define HOLD_MS       800
#define TAP_WINDOW_MS 600

bool ev_shortTap   = false;
bool ev_longHold   = false;
bool ev_tapsReady  = false;
int  ev_tapCount   = 0;
bool ev_isHolding  = false;

// ── Modos y estado ────────────────────────────────────────────────────────
#define NUM_MODES 3
int appMode = 0;

enum AppState { ST_WIFI, ST_LISTO, ST_GRABANDO, ST_ENVIANDO, ST_RESULTADO, ST_ERROR };
AppState appState = ST_WIFI;
unsigned long resultShowAt = 0;

// ═══════════════════════════════════════════════════════════════════════════
//  PARSING
// ═══════════════════════════════════════════════════════════════════════════

int parseMoodId(const String& json) {
  int idx = json.indexOf("\"mood_id\":");
  if (idx < 0) return -1;
  int start = idx + 10, end = start;
  while (end < (int)json.length() && (isdigit(json[end]) || json[end] == '-')) end++;
  return (end == start) ? -1 : json.substring(start, end).toInt();
}

String parseIntent(const String& json) {
  int idx = json.indexOf("\"intent\":\"");
  if (idx < 0) return "?";
  int start = idx + 10;
  int end = json.indexOf("\"", start);
  return (end <= start) ? "?" : json.substring(start, end);
}

// ═══════════════════════════════════════════════════════════════════════════
//  setMood
// ═══════════════════════════════════════════════════════════════════════════

void setMood(int mood_id) {
  if (mood_id < 0 || mood_id >= MOOD_COUNT) return;
  currentMood = mood_id;
  Serial.printf("setMood -> %d (%s)\n", mood_id, MOOD_NAMES[mood_id]);
}

// ═══════════════════════════════════════════════════════════════════════════
//  OJOS
// ═══════════════════════════════════════════════════════════════════════════

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

void renderEyes(const char* label){
  display.clearDisplay();
  updateEyes();
  drawEye(leftEye, true);
  drawEye(rightEye, false);

  display.fillRect(0, 54, 128, 10, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int lw = strlen(label) * 6;
  display.setCursor((128 - lw) / 2, 55);
  display.print(label);
  display.display();
}

void renderText(const char* line1, const char* line2="", const char* line3=""){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10); display.print(line1);
  display.setCursor(0, 26); display.print(line2);
  display.setCursor(0, 42); display.print(line3);
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════════
//  I2S — 8 kHz / 32-bit
// ═══════════════════════════════════════════════════════════════════════════

void initMicrophone(){
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

int leerAmplitud(){
  int32_t buf[128]; size_t br=0;
  i2s_read(I2S_PORT, buf, sizeof(buf), &br, 10);
  if(br==0) return 0;
  int n=br/4, mx=INT_MIN, mn=INT_MAX;
  for(int i=0;i<n;i++){
    int32_t v=buf[i]>>GAIN_SHIFT;
    if(v>mx) mx=v;
    if(v<mn) mn=v;
  }
  return mx-mn;
}

size_t convert_i2s32_to_pcm16(const uint8_t* input, size_t input_len, uint8_t* output) {
  const int32_t* samples32 = (const int32_t*)input;
  int16_t* samples16 = (int16_t*)output;
  size_t num_samples = input_len / 4;
  if (num_samples == 0) return 0;

  static int64_t dc_ema = 0;
  double sum_sq = 0.0;
  int32_t max_abs = 0;
  for (size_t i = 0; i < num_samples; i++) {
    int64_t s = (int64_t)samples32[i];
    dc_ema = ((dc_ema * 63) + s) >> 6;
    int32_t centered = (int32_t)(s - dc_ema);
    int32_t absv = centered < 0 ? -centered : centered;
    if (absv > max_abs) max_abs = absv;
    sum_sq += (double)centered * centered;
    int32_t v = centered >> GAIN_SHIFT;
    if (v >  32767) v =  32767;
    if (v < -32768) v = -32768;
    samples16[i] = (int16_t)v;
  }
  float rms = (num_samples > 0) ? sqrt(sum_sq / num_samples) / (1 << GAIN_SHIFT) : 0.0f;
  Serial.printf("peak_raw=%d rms=%.1f\n", max_abs, rms);
  return num_samples * 2;
}

// ═══════════════════════════════════════════════════════════════════════════
//  HTTP — tarea de fondo (Core 0)
// ═══════════════════════════════════════════════════════════════════════════

void httpSendTask(void*){
  Serial.printf("httpSendTask: %u bytes, session=%s\n", _rec_len, _rec_sid.c_str());
  if(WiFi.status()==WL_CONNECTED){
    WiFiClientSecure tls; tls.setInsecure();
    HTTPClient http;
    http.begin(tls, SERVER_URL);
    http.setTimeout(15000);
    http.addHeader("device-id",  DEVICE_ID);
    http.addHeader("session-id", _rec_sid.c_str());
    http.addHeader("end",        "true");
    http.addHeader("Content-Type", "application/octet-stream");
    _rec_status = http.sendRequest("POST", _rec_buf, _rec_len);
    if(_rec_status >= 200 && _rec_status < 300){
      String payload = http.getString();
      Serial.println(payload);
      int mid = parseMoodId(payload);
      _rec_intent = parseIntent(payload);
      setMood(mid);
    }
    http.end();
  } else {
    _rec_status = -4;
    Serial.println("httpSendTask: WiFi desconectado");
  }
  _rec_done = true; _rec_task = NULL; vTaskDelete(NULL);
}

void startRecording(){
  _rec_len       = 0;
  _rec_sid       = String(millis());
  warmup_complete = false;
  warmup_samples  = 0;
  recRecording   = true;
  setMood(MOOD_SUSPICIOUS);
  appState = ST_GRABANDO;
  Serial.printf("Grabando... (session=%s)\n", _rec_sid.c_str());
  tone(BUZZER_PIN, 880, 80);
}

void stopAndSend(){
  recRecording = false;
  _rec_done = false; _rec_status = 0; _rec_intent = "";
  setMood(MOOD_DIZZY);
  appState = ST_ENVIANDO;
  xTaskCreatePinnedToCore(httpSendTask, "httpSend", 8192, NULL, 1, &_rec_task, 0);
  Serial.printf("Enviando %u bytes...\n", _rec_len);
}

// ═══════════════════════════════════════════════════════════════════════════
//  TOUCH
// ═══════════════════════════════════════════════════════════════════════════

void procesarTouch() {
  unsigned long now = millis();
  bool cur = digitalRead(TOUCH_PIN);

  ev_shortTap  = false;
  ev_longHold  = false;
  ev_tapsReady = false;
  ev_tapCount  = 0;
  ev_isHolding = false;

  if (cur && !tc_last) {
    tc_downAt = now;
    tc_held   = false;
  }

  if (cur && (now - tc_downAt >= HOLD_MS)) {
    tc_held      = true;
    ev_isHolding = true;
  }

  if (!cur && tc_last) {
    unsigned long held = now - tc_downAt;
    if (held >= HOLD_MS) {
      ev_longHold = true;
      tc_tapCount = 0;
    } else {
      ev_shortTap  = true;
      tc_tapCount++;
      tc_lastTapAt = now;
    }
    tc_held = false;
  }

  if (tc_tapCount > 0 && !cur && (now - tc_lastTapAt > TAP_WINDOW_MS)) {
    ev_tapsReady = true;
    ev_tapCount  = tc_tapCount;
    tc_tapCount  = 0;
  }

  tc_last = cur;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════════════

void setup(){
  Serial.begin(115200); delay(500);
  Serial.println("\n=== codigo_funcional_transcripcion ===");

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Wire.begin(SDA_PIN, SCL_PIN); Wire.setClock(400000);

  if(!display.begin(0x3C, true)){
    Serial.println("ERROR: OLED no encontrado"); for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  leftEye.init(18,14,36,36);
  rightEye.init(74,14,36,36);

  renderText("MOCHI HTTP TEST", "Iniciando...");
  delay(800);

  renderText("WiFi...", WIFI_SSID);
  WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID, WIFI_PASS);
  int t=0;
  while(WiFi.status()!=WL_CONNECTED && t<20){ delay(500); t++; }
  if(WiFi.status()!=WL_CONNECTED){
    renderText("WiFi: FALLO", "Reinicia el ESP32");
    Serial.println("WiFi: FALLO"); for(;;) delay(1000);
  }
  String ip = WiFi.localIP().toString();
  Serial.printf("WiFi OK: %s\n", ip.c_str());
  renderText("WiFi: OK", ip.c_str());
  delay(1000);

  initMicrophone();

  appState = ST_LISTO;
  setMood(MOOD_NORMAL);
  Serial.println("Listo. Mantén para grabar, 3 taps para cambiar modo.");
}

// ═══════════════════════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════════════════════

void loop(){
  procesarTouch();

  static bool holdMicStarted = false;

  if (ev_isHolding && !holdMicStarted && !recRecording && _rec_task == NULL) {
    holdMicStarted = true;
    startRecording();
  }
  if (ev_longHold) {
    if (holdMicStarted && recRecording) stopAndSend();
    holdMicStarted = false;
  }

  if (ev_tapsReady && ev_tapCount >= 3) {
    appMode = (appMode + 1) % NUM_MODES;
    setMood(MOOD_NORMAL);
    appState = ST_LISTO;
    tone(BUZZER_PIN, 1200, 80);
    Serial.printf("Modo -> %d\n", appMode);
  }

  if (recRecording) {
    static int32_t sampleBuffer[CHUNK_SIZE / 4];
    static uint8_t buffer_pcm16[CHUNK_SIZE / 2];
    size_t bytesRead = 0;
    esp_err_t r = i2s_read(I2S_PORT, sampleBuffer, sizeof(sampleBuffer),
                            &bytesRead, pdMS_TO_TICKS(1000));
    if (r == ESP_OK && bytesRead > 0) {
      size_t pcm16_len = convert_i2s32_to_pcm16(
          (const uint8_t*)sampleBuffer, bytesRead, buffer_pcm16);

      if (pcm16_len > 0) {
        if (!warmup_complete) {
          size_t warmup_needed = (REC_SAMPLE_RATE * WARMUP_DURATION_MS / 1000) * 2;
          warmup_samples += pcm16_len;
          if (warmup_samples >= warmup_needed) {
            warmup_complete = true;
            Serial.println("Warmup OK");
          }
        } else {
          size_t space   = REC_AUDIO_BYTES - _rec_len;
          size_t to_copy = pcm16_len < space ? pcm16_len : space;
          memcpy(_rec_buf + _rec_len, buffer_pcm16, to_copy);
          _rec_len += to_copy;
          if (_rec_len >= REC_AUDIO_BYTES) stopAndSend();
        }
      }
    }
  }

  if(_rec_done){
    _rec_done = false;
    resultShowAt = millis();
    if(_rec_status >= 200 && _rec_status < 300){
      appState = ST_RESULTADO;
      tone(BUZZER_PIN, 1046, 150);
      Serial.printf("OK — mood_id=%d intent=%s\n", currentMood, _rec_intent.c_str());
    } else {
      setMood(MOOD_SAD);
      appState = ST_ERROR;
      tone(BUZZER_PIN, 300, 400);
      Serial.printf("FALLO HTTP %d\n", _rec_status);
    }
  }

  if((appState==ST_RESULTADO || appState==ST_ERROR) &&
     (millis()-resultShowAt > 4000)){
    setMood(MOOD_NORMAL);
    appState = ST_LISTO;
  }

  switch(appState){

    case ST_LISTO: {
      char label[20];
      snprintf(label, sizeof(label), "hold=mic  3tap=M%d", appMode);
      renderEyes(label);
      break;
    }

    case ST_GRABANDO: {
      int amp = leerAmplitud();
      int bar = constrain(map(amp, 0, 30000, 0, 124), 0, 124);
      display.clearDisplay();
      updateEyes();
      drawEye(leftEye, true);
      drawEye(rightEye, false);
      display.drawRect(2, 54, 124, 9, SSD1306_WHITE);
      if(bar>0) display.fillRect(3, 55, bar, 7, SSD1306_WHITE);
      int pct = (int)((_rec_len * 100) / REC_AUDIO_BYTES);
      display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
      display.setCursor(0,55);
      if(bar < 60){ display.printf("REC %d%%", pct); }
      display.display();
      break;
    }

    case ST_ENVIANDO: {
      unsigned long t = millis();
      String dots = "";
      int nd = (t/400) % 4;
      for(int i=0;i<nd;i++) dots+=".";
      char label[20]; snprintf(label, sizeof(label), "enviando%s", dots.c_str());
      renderEyes(label);
      break;
    }

    case ST_RESULTADO: {
      char label[24];
      snprintf(label, sizeof(label), "%s", _rec_intent.c_str());
      renderEyes(label);
      break;
    }

    case ST_ERROR: {
      char label[24];
      snprintf(label, sizeof(label), "HTTP %d", _rec_status);
      renderEyes(label);
      break;
    }

    default: break;
  }
}
