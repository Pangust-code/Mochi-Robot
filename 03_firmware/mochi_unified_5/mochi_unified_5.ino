// ═══════════════════════════════════════════════════════════════════════
//  MOCHI ROBOT — ESP32-C6 Supermini — v6
//
//  Modos (toque LARGO >800ms para avanzar):
//    0 = Mascota   (ojos animados + micrófono reactivo)
//    1 = GIFs      (animaciones desde LittleFS)
//    2 = Reloj     (Wi-Fi + NTP, UTC-5 Ecuador)
//    3 = Pomodoro  (temporizador, control por touch)
//    4 = Micrófono (test: barra de volumen en tiempo real)
//
//  Modo Mascota : 1 tap = mood aleatorio | 3 taps = Tetris
//  Modo Pomodoro: 1 tap = inicio/pausa   | 3 taps = reinicio
//  Modo Mic Test: 1 tap = reinicia pico máximo
//
//  Pines ESP32-C6 Supermini:
//    OLED  SDA=6  SCL=7  (I2C, 0x3C)
//    Buzzer=19   Touch TTP223=2
//    INMP441: SCK=4  WS=5  SD=3  L/R→GND
//
//  TOUCH — FIX v6:
//    Se lee el pin UNA sola vez por ciclo de loop() y se pasan
//    dos valores al dispatcher: `touched` (estado actual) y
//    `tappedShort` / `heldLong` (eventos ya procesados).
//    Ningún modo escribe lastTouchState directamente; solo
//    procesTouch() lo hace. Elimina los flancos perdidos.
// ═══════════════════════════════════════════════════════════════════════

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <U8g2lib.h>
#include <math.h>
#include <WiFi.h>
#include <time.h>
#include <driver/i2s.h>
#include <LittleFS.h>

#define SSD1306_WHITE SH110X_WHITE
#define SSD1306_BLACK SH110X_BLACK

// ── Pines ──────────────────────────────────────────────────────────────
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define SDA_PIN          6
#define SCL_PIN          7
#define TOUCH_PIN        2
#define BUZZER_PIN      19
#define I2S_SCK_PIN      4
#define I2S_WS_PIN       5
#define I2S_SD_PIN       3
#define I2S_PORT   I2S_NUM_0

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// ── Wi-Fi / NTP ────────────────────────────────────────────────────────
const char* ssid               = "Pixel_4531";
const char* password           = "tigrillo";
const char* ntpServer          = "pool.ntp.org";
const long  gmtOffset_sec      = -18000;
const int   daylightOffset_sec = 0;
const char* diasSemana[] = {"Dom","Lun","Mar","Mie","Jue","Vie","Sab"};
const char* mesesAnio[]  = {"Ene","Feb","Mar","Abr","May","Jun",
                             "Jul","Ago","Sep","Oct","Nov","Dic"};
bool timeSynced = false;
bool wifiFailed = false;

// ── Bitmaps ────────────────────────────────────────────────────────────
const unsigned char bmp_heart[] PROGMEM = {
  0x00,0x00,0x0c,0x60,0x1e,0xf0,0x3f,0xf8,0x7f,0xfc,0x7f,0xfc,
  0x7f,0xfc,0x3f,0xf8,0x1f,0xf0,0x0f,0xe0,0x07,0xc0,0x03,0x80,
  0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_zzz[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,
  0x00,0x30,0x00,0x7e,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,
  0x30,0x00,0x7c,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_anger[] PROGMEM = {
  0x00,0x00,0x11,0x10,0x2a,0x90,0x44,0x40,0x80,0x20,0x80,0x20,
  0x44,0x40,0x2a,0x90,0x11,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

// ── Moods ──────────────────────────────────────────────────────────────
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

int  currentMood     = MOOD_NORMAL;
int  moodBeforeNoise = MOOD_NORMAL;
bool isStartled      = false;
unsigned long lastLoudNoiseTime = 0;

// ── Struct Eye ─────────────────────────────────────────────────────────
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
    velX=(velX+(targetX-x)*k)*d;  velY=(velY+(targetY-y)*k)*d;
    velW=(velW+(targetW-w)*k)*d;  velH=(velH+(targetH-h)*k)*d;
    x+=velX; y+=velY; w+=velW; h+=velH;
    pVelX=(pVelX+(targetPupilX-pupilX)*pk)*pd;
    pVelY=(pVelY+(targetPupilY-pupilY)*pk)*pd;
    pupilX+=pVelX; pupilY+=pVelY;
  }
};

Eye leftEye, rightEye;
unsigned long lastSaccade     = 0;
unsigned long saccadeInterval = 3000;
float breathVal = 0;

// ── Estado global ──────────────────────────────────────────────────────
#define NUM_MODES 5
int appMode = 0;

// ── TOUCH — estado centralizado ────────────────────────────────────────
// Solo procesTouch() escribe estas variables. Ningún modo las toca.
bool          tc_last      = false;   // estado anterior del pin
unsigned long tc_downAt    = 0;       // millis() al bajar el flanco
unsigned long tc_lastTapAt = 0;       // millis() del último tap corto
int           tc_tapCount  = 0;       // taps acumulados en la ventana
bool          tc_held      = false;   // true mientras se mantiene >HOLD_MS

#define HOLD_MS        800
#define TAP_WINDOW_MS  900
#define TAPS_FOR_SONG    3

// Eventos de touch que procesTouch() publica cada ciclo:
bool ev_shortTap  = false;  // flanco subida < HOLD_MS
bool ev_longHold  = false;  // flanco subida >= HOLD_MS
bool ev_tapsReady = false;  // ventana de taps expiró
int  ev_tapCount  = 0;      // cuántos taps había en la ventana
bool ev_isHolding = false;  // sigue presionado ≥ HOLD_MS ahora mismo

// Pomodoro (estado propio, no toca las variables tc_*)
int  pomoState   = 0;
int  pomoSeconds = 25 * 60;
unsigned long lastPomoTick = 0;
int  pomoTapCount  = 0;
unsigned long lastPomoTapTime = 0;

// Mic test
int micPeak   = 0;
int micSmooth = 0;

// Control de pantalla activa
bool u8g2Active = false;

// GIF buffer
#define FRAME_SIZE 1024
static uint8_t frameBuffer[FRAME_SIZE];

// LittleFS
bool fsOk = false;

// Prototipos
void renderFrame();
void activarAdafruit();

// ═══════════════════════════════════════════════════════════════════════
//  TOUCH — único punto de lectura
//
//  Se llama UNA VEZ al inicio de cada loop().
//  Publica eventos en ev_* que los modos consumen de forma pasiva.
//  Ningún modo llama digitalRead(TOUCH_PIN) directamente.
// ═══════════════════════════════════════════════════════════════════════

void procesTouch() {
  unsigned long now = millis();
  bool cur = digitalRead(TOUCH_PIN);

  // Resetear eventos de un solo ciclo
  ev_shortTap  = false;
  ev_longHold  = false;
  ev_tapsReady = false;
  ev_tapCount  = 0;

  // ── Flanco de bajada (pressed) ──────────────────────────────────────
  if (cur && !tc_last) {
    tc_downAt = now;
    tc_held   = false;
  }

  // ── Mientras se mantiene presionado ────────────────────────────────
  if (cur && (now - tc_downAt >= HOLD_MS)) {
    tc_held      = true;
    ev_isHolding = true;
  } else if (!cur) {
    ev_isHolding = false;
  }

  // ── Flanco de subida (released) ─────────────────────────────────────
  if (!cur && tc_last) {
    unsigned long held = now - tc_downAt;
    if (held >= HOLD_MS) {
      ev_longHold = true;           // toque largo
      tc_tapCount = 0;              // cancelar taps acumulados
    } else {
      ev_shortTap   = true;         // toque corto
      tc_tapCount++;
      tc_lastTapAt  = now;
    }
    tc_held = false;
  }

  // ── Ventana de taps expiró ──────────────────────────────────────────
  if (tc_tapCount > 0 && !cur && (now - tc_lastTapAt > TAP_WINDOW_MS)) {
    ev_tapsReady = true;
    ev_tapCount  = tc_tapCount;
    tc_tapCount  = 0;
  }

  tc_last = cur;
}

// ═══════════════════════════════════════════════════════════════════════
//  CAMBIO DE MODO (toque largo)
//  Retorna true si hubo cambio → el loop() debe hacer return.
// ═══════════════════════════════════════════════════════════════════════

bool handleModeChange() {
  if (!ev_longHold) return false;

  int prev = appMode;
  appMode = (appMode + 1) % NUM_MODES;

  if (prev == 2) { timeSynced = false; wifiFailed = false; }
  if (prev == 1) activarAdafruit();
  if (prev == 4) { micPeak = 0; micSmooth = 0; }

  tone(BUZZER_PIN, 1000, 80);
  Serial.printf("-> Modo %d\n", appMode);
  return true;
}

// ═══════════════════════════════════════════════════════════════════════
//  LITTLEFS
// ═══════════════════════════════════════════════════════════════════════

void initFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS: error. Sube los .bin primero.");
    return;
  }
  fsOk = true;
  Serial.printf("LittleFS OK — %u/%u KB\n",
    (unsigned)(LittleFS.usedBytes()/1024),
    (unsigned)(LittleFS.totalBytes()/1024));
}

// ═══════════════════════════════════════════════════════════════════════
//  MICRÓFONO I2S
// ═══════════════════════════════════════════════════════════════════════

void initMicrophone() {
  i2s_config_t cfg = {
    .mode              = (i2s_mode_t)(I2S_MODE_MASTER|I2S_MODE_RX),
    .sample_rate       = 16000,
    .bits_per_sample   = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format    = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags  = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count     = 4,
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
}

int leerAmplitud() {
  int16_t buf[256];
  size_t  br = 0;
  i2s_read(I2S_PORT, buf, sizeof(buf), &br, 10);
  if (br == 0) return 0;
  int n = br/2, mx = -32768, mn = 32767;
  for (int i=0;i<n;i++){
    if(buf[i]>mx) mx=buf[i];
    if(buf[i]<mn) mn=buf[i];
  }
  return mx-mn;
}

void escucharEntorno() {
  int amp = leerAmplitud();
  if (amp > 15000 && !isStartled) {
    moodBeforeNoise = currentMood;
    int rx[] = {MOOD_SURPRISED,MOOD_ANGRY,MOOD_DIZZY,MOOD_SAD,MOOD_SUSPICIOUS};
    currentMood = rx[esp_random()%5];
    isStartled = true; lastLoudNoiseTime = millis();
  }
  if (isStartled && millis()-lastLoudNoiseTime>2000) {
    currentMood = moodBeforeNoise; isStartled = false;
  }
}

// ═══════════════════════════════════════════════════════════════════════
//  MODO 4 — TEST MICRÓFONO
// ═══════════════════════════════════════════════════════════════════════

void runMicTestMode() {
  // 1 tap corto = resetear pico
  if (ev_shortTap) { micPeak = 0; Serial.println("Pico reseteado"); }

  int raw = leerAmplitud();
  micSmooth = (micSmooth*6 + raw*2) / 8;
  if (raw > micPeak) micPeak = raw;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(18,0); display.print("TEST MICROFONO");

  display.drawRect(0,13,SCREEN_WIDTH,18,SSD1306_WHITE);
  int bw = constrain((int)map(micSmooth,0,40000,0,SCREEN_WIDTH-2),0,SCREEN_WIDTH-2);
  if (bw>0) display.fillRect(1,14,bw,16,SSD1306_WHITE);

  int tx = (int)map(15000,0,40000,0,SCREEN_WIDTH-2);
  for (int y=13;y<=30;y+=2) display.drawPixel(tx,y,SSD1306_WHITE);

  display.setCursor(0,35);  display.print("Val:");  display.print(raw);
  display.setCursor(68,35); display.print("Pico:"); display.print(micPeak);

  if (raw > 15000) {
    display.setTextSize(2); display.setCursor(24,48); display.print("LOUD!");
  } else {
    display.setCursor(0,48);  display.print("umbral:15000");
    display.setCursor(82,48); display.print(constrain(map(raw,0,15000,0,100),0,100));
    display.print("%");
  }
  display.display();
  Serial.printf("MIC raw=%5d smooth=%5d peak=%5d%s\n",
    raw,micSmooth,micPeak,raw>15000?" LOUD":"");
}

// ═══════════════════════════════════════════════════════════════════════
//  MÚSICA
// ═══════════════════════════════════════════════════════════════════════

static void nota(int freq,int dur){
  if(freq==0) delay(dur);
  else { tone(BUZZER_PIN,freq,(int)(dur*0.9f)); delay(dur); }
}

void tetrisTheme(){
  const int E5=659,B4=494,C5=523,D5=587,A4=440,F5=698,G5=784,A5=880;
  int mel[]={
    E5,250,B4,125,C5,125,D5,250,C5,125,B4,125,
    A4,250,A4,125,C5,125,E5,250,D5,125,C5,125,
    B4,375,C5,125,D5,250,E5,250,C5,250,A4,250,A4,250,0,250,
    D5,375,F5,125,A5,250,G5,125,F5,125,
    E5,375,C5,125,E5,250,D5,125,C5,125,
    B4,250,B4,125,C5,125,D5,250,E5,250,C5,250,A4,250,A4,250,0,250
  };
  int n=sizeof(mel)/sizeof(mel[0])/2; float spd=0.9f;
  for(int v=0;v<3;v++){
    for(int i=0;i<n;i++){ nota(mel[i*2],(int)(mel[i*2+1]*spd)); renderFrame(); }
    spd*=0.75f;
  }
}

// ═══════════════════════════════════════════════════════════════════════
//  OJOS (Adafruit)
// ═══════════════════════════════════════════════════════════════════════

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

void drawUltraProEye(Eye& e,bool isLeft){
  int ix=(int)e.x,iy=(int)e.y,iw=(int)e.w,ih=(int)e.h,r=(iw<20)?3:8;
  display.fillRoundRect(ix,iy,iw,ih,r,SSD1306_WHITE);
  int pw=(int)(iw/2.2f),ph=(int)(ih/2.2f),cx=ix+iw/2,cy=iy+ih/2;
  int px=constrain(cx+(int)e.pupilX-(pw/2),ix,ix+iw-pw);
  int py=constrain(cy+(int)e.pupilY-(ph/2),iy,iy+ih-ph);
  display.fillRoundRect(px,py,pw,ph,r/2,SSD1306_BLACK);
  if(iw>15&&ih>15) display.fillCircle(px+pw-4,py+4,2,SSD1306_WHITE);
  drawEyelidMask(e.x,e.y,e.w,e.h,currentMood,isLeft);
}

void updatePhysicsAndMood(){
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
    int dir=random(0,10); float lx=0,ly=0;
    if(dir==4){lx=-6;ly=-4;} else if(dir==5){lx=6;ly=-4;}
    else if(dir==6){lx=-6;ly=4;} else if(dir==7){lx=6;ly=4;}
    else if(dir==8){lx=8;ly=0;}  else if(dir==9){lx=-8;ly=0;}
    leftEye.targetPupilX=lx;  leftEye.targetPupilY=ly;
    rightEye.targetPupilX=lx; rightEye.targetPupilY=ly;
    leftEye.targetX=18+(lx*0.3f); leftEye.targetY=14+(ly*0.3f);
    rightEye.targetX=74+(lx*0.3f);rightEye.targetY=14+(ly*0.3f);
  }
  if(!leftEye.blinking){
    float bW=36,bH=36+breathVal;
    switch(currentMood){
      case MOOD_NORMAL:     leftEye.targetW=bW; leftEye.targetH=bH; rightEye.targetW=bW; rightEye.targetH=bH; break;
      case MOOD_HAPPY: case MOOD_LOVE:
                            leftEye.targetW=40; leftEye.targetH=32; rightEye.targetW=40; rightEye.targetH=32; break;
      case MOOD_SURPRISED:  leftEye.targetW=30; leftEye.targetH=45; rightEye.targetW=30; rightEye.targetH=45; break;
      case MOOD_SLEEPY:     leftEye.targetW=38; leftEye.targetH=30; rightEye.targetW=38; rightEye.targetH=30; break;
      case MOOD_ANGRY:      leftEye.targetW=34; leftEye.targetH=32; rightEye.targetW=34; rightEye.targetH=32; break;
      case MOOD_SAD:        leftEye.targetW=34; leftEye.targetH=40; rightEye.targetW=34; rightEye.targetH=40; break;
      case MOOD_SUSPICIOUS: leftEye.targetW=36; leftEye.targetH=20; rightEye.targetW=36; rightEye.targetH=42; break;
      case MOOD_DIZZY:
        leftEye.targetW=34+sinf(now/80.0f)*6;  leftEye.targetH=34+cosf(now/80.0f)*6;
        rightEye.targetW=34+cosf(now/80.0f)*6; rightEye.targetH=34+sinf(now/80.0f)*6;
        leftEye.targetPupilX=sinf(now/150.0f)*12;      leftEye.targetPupilY=cosf(now/150.0f)*12;
        rightEye.targetPupilX=sinf((now/150.0f)+PI)*12;rightEye.targetPupilY=cosf((now/150.0f)+PI)*12;
        break;
    }
  }
  leftEye.update(); rightEye.update();
}

void renderFrame(){
  display.clearDisplay();
  updatePhysicsAndMood();
  if     (currentMood==MOOD_LOVE)   display.drawBitmap(56, 0,bmp_heart,16,16,SSD1306_WHITE);
  else if(currentMood==MOOD_SLEEPY) display.drawBitmap(110,0,bmp_zzz,  16,16,SSD1306_WHITE);
  else if(currentMood==MOOD_ANGRY)  display.drawBitmap(56, 0,bmp_anger,16,16,SSD1306_WHITE);
  drawUltraProEye(leftEye,true);
  drawUltraProEye(rightEye,false);
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════
//  MODO 0 — MASCOTA
// ═══════════════════════════════════════════════════════════════════════

void runMascotaMode(){
  escucharEntorno();
  // Taps cortos: evaluar cuando expira la ventana
  if (ev_tapsReady) {
    if (ev_tapCount >= TAPS_FOR_SONG) {
      int prev = currentMood; currentMood = MOOD_HAPPY;
      tetrisTheme(); currentMood = prev;
    } else {
      int nuevo;
      do { nuevo = esp_random() % MOOD_COUNT; } while (nuevo == currentMood);
      currentMood = nuevo;
      Serial.printf("Tap -> mood %d\n", currentMood);
    }
  }
  renderFrame();
}

// ═══════════════════════════════════════════════════════════════════════
//  GIFs desde LittleFS (U8g2)
// ═══════════════════════════════════════════════════════════════════════

void activarU8g2(){
  if(u8g2Active) return;
  u8g2.begin(); u8g2.setI2CAddress(0x3C*2);
  u8g2.clearBuffer(); u8g2.sendBuffer();
  u8g2Active=true;
}
void activarAdafruit(){
  if(!u8g2Active) return;
  display.begin(0x3C,true);
  display.clearDisplay(); display.display();
  display.setTextColor(SSD1306_WHITE);
  u8g2Active=false;
}

bool playAnimationFS(const char* path,int fps){
  if(!fsOk) return false;
  File f=LittleFS.open(path,"r");
  if(!f){ Serial.printf("No encontrado: %s\n",path); return false; }
  int fd=1000/fps;
  int tf=(int)(f.size()/FRAME_SIZE);
  for(int i=0;i<tf;i++){
    // Salir si cambió el modo
    if(appMode!=1){ f.close(); return true; }

    if(f.read(frameBuffer,FRAME_SIZE)<FRAME_SIZE) break;
    memcpy(u8g2.getBufferPtr(),frameBuffer,FRAME_SIZE);
    u8g2.sendBuffer();

    // Espera no bloqueante: sigue procesando el touch durante el delay
    unsigned long frameStart = millis();
    while(millis() - frameStart < (unsigned long)fd){
      procesTouch();                  // detecta flancos mientras espera
      if(handleModeChange()){         // toque largo → salir del GIF
        f.close(); return true;
      }
      delay(1);                       // cede 1ms al sistema
    }
  }
  f.close();
  return true;
}

void runGifMode() {
  if (!fsOk) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0,10);
    display.print("LittleFS no montado.");

    display.setCursor(0,25);
    display.print("Sube los .bin con el");

    display.setCursor(0,38);
    display.print("plugin LittleFS");

    display.setCursor(0,51);
    display.print("y reinicia.");

    display.display();
    delay(3000);
    return;
  }

  activarU8g2();

  const char* arch[] = {
    "/0.bin",
    "/star.bin",
    "/sung_nuoc.bin",
    "/sushi.bin",
    "/bmo.bin",
    "/guess.bin",
    "/sung_nuoc_2.bin",
    "/speed.bin",
    "/bee.bin",
    "/angry.bin",
    "/yakura.bin",
    "/gian_du.bin",
    "/cuoi_khinh_bi.bin",
    "/angry3.bin"
  };

  int totalArchivos = sizeof(arch) / sizeof(arch[0]);

  for (int i = 0; i < totalArchivos; i++) {
    if (appMode != 1) break;

    playAnimationFS(arch[i], 20);

    u8g2.clearBuffer();
    u8g2.sendBuffer();

    delay(80);
  }
}

// ═══════════════════════════════════════════════════════════════════════
//  RELOJ (Wi-Fi + NTP)
// ═══════════════════════════════════════════════════════════════════════

void runClockMode(){
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  if(!timeSynced&&!wifiFailed){
    display.setTextSize(1); display.setCursor(10,28);
    display.print("Conectando Wi-Fi..."); display.display();
    WiFi.mode(WIFI_STA); WiFi.begin(ssid,password);
    int t=0; while(WiFi.status()!=WL_CONNECTED&&t<10){delay(500);t++;}
    if(WiFi.status()==WL_CONNECTED){
      configTime(gmtOffset_sec,daylightOffset_sec,ntpServer);
      struct tm ti;
      if(getLocalTime(&ti,5000)) timeSynced=true;
    }
    if(!timeSynced){
      wifiFailed=true; display.clearDisplay();
      display.setCursor(15,28); display.print("Error Wi-Fi");
      display.display(); delay(2000);
    }
    WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
  }
  if(timeSynced){
    struct tm ti;
    if(getLocalTime(&ti)){
      char tb[8],db[24];
      strftime(tb,sizeof(tb),"%H:%M",&ti);
      sprintf(db,"%s %02d %s",diasSemana[ti.tm_wday],ti.tm_mday,mesesAnio[ti.tm_mon]);
      display.setTextSize(3);
      display.setCursor((SCREEN_WIDTH-(int)strlen(tb)*18)/2,10); display.print(tb);
      display.setTextSize(1);
      display.setCursor((SCREEN_WIDTH-(int)strlen(db)*6)/2,50);  display.print(db);
    }
  } else {
    display.setTextSize(1);
    display.setCursor(5,20); display.print("Sin conexion Wi-Fi");
    display.setCursor(5,38); display.print("Toque largo: sig. modo");
  }
  display.display(); delay(100);
}

// ═══════════════════════════════════════════════════════════════════════
//  POMODORO
//  Usa ev_shortTap / ev_tapsReady para sus acciones.
//  No escribe tc_* ni lastTouchState.
// ═══════════════════════════════════════════════════════════════════════

void runPomodoroMode(){
  // Tap corto: inicio/pausa (primer tap de la ventana)
  if (ev_shortTap && pomoTapCount == 0) {
    if (pomoState==0) { pomoState=1; lastPomoTick=millis(); }
    else              { pomoState=0; }
  }

  // Contar taps de la ventana Pomodoro
  if (ev_shortTap) {
    pomoTapCount++;
    lastPomoTapTime = millis();
  }

  // Ventana de taps Pomodoro expiró: 3 taps = reinicio
  if (ev_tapsReady && ev_tapCount >= 3) {
    pomoState   = 0;
    pomoSeconds = 25 * 60;
    tone(BUZZER_PIN, 1200, 150);
  }
  // Resetear contador local cuando la ventana expira
  if (ev_tapsReady) pomoTapCount = 0;

  // Temporizador
  if (pomoState==1){
    unsigned long now=millis();
    if(now-lastPomoTick>=1000){
      lastPomoTick=now;
      if(pomoSeconds>0){ pomoSeconds--; }
      else {
        pomoState=0; pomoSeconds=25*60;
        tone(BUZZER_PIN,880,200); delay(250); tone(BUZZER_PIN,1046,500);
      }
    }
  }

  // Pantalla
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(25,5); display.print("MODO ENFOQUE");
  char tb[8]; sprintf(tb,"%02d:%02d",pomoSeconds/60,pomoSeconds%60);
  display.setTextSize(3);
  display.setCursor((SCREEN_WIDTH-(int)strlen(tb)*18)/2,22); display.print(tb);
  display.setTextSize(1);
  if(pomoState==0){display.setCursor(45,55);display.print("PAUSADO");}
  else            {display.setCursor(38,55);display.print("ENFOCADO");}
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════════

void setup(){
  Serial.begin(115200); delay(500);
  Serial.println("\n=== MOCHI v6 ESP32-C6 ===");

  pinMode(TOUCH_PIN,INPUT); pinMode(BUZZER_PIN,OUTPUT);
  Wire.begin(SDA_PIN,SCL_PIN); Wire.setClock(400000);

  if(!display.begin(0x3C,true)){
    Serial.println("ERROR: OLED no encontrado"); for(;;);
  }
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE);

  u8g2.begin(); u8g2.setI2CAddress(0x3C*2);
  u8g2.clearBuffer(); u8g2.sendBuffer();
  activarAdafruit();

  leftEye.init(18,14,36,36);
  rightEye.init(74,14,36,36);

  initMicrophone();
  initFS();

  display.clearDisplay(); display.setTextSize(1);
  display.setCursor(14,10); display.print("MOCHI v6 listo!");
  display.setCursor(0,24);  display.print(fsOk?"LittleFS: OK":"LittleFS: ERROR");
  display.setCursor(0,38);  display.print("Hold: cambiar modo");
  display.setCursor(0,50);  display.print("Pet|GIF|Clk|Pomo|Mic");
  display.display(); delay(2000);
  Serial.println("=== LISTO ===");
}

// ═══════════════════════════════════════════════════════════════════════
//  LOOP — lectura única del pin, luego dispatch
// ═══════════════════════════════════════════════════════════════════════

void loop(){
  // 1. Leer el pin UNA vez y publicar eventos
  procesTouch();

  // 2. Toque largo = cambio de modo (todos los modos)
  if (handleModeChange()) return;

  // 3. Dispatcher de modos
  switch(appMode){
    case 0: runMascotaMode();  break;
    case 1: runGifMode();      break;
    case 2: runClockMode();    break;
    case 3: runPomodoroMode(); break;
    case 4: runMicTestMode();  break;
  }
}
