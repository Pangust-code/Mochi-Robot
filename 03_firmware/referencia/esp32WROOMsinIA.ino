#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <math.h>
#include <WiFi.h> // Librería para el Wi-Fi
#include <time.h> // Librería para el servidor de tiempo (NTP)
#include <BluetoothSerial.h> // ¡NUEVO! Librería nativa de Bluetooth
#include <driver/i2s.h> // ¡NUEVO! Librería nativa para leer el micrófono

// ── Objeto Bluetooth ──────────────────────────────────────────
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

// ── Aliases de color ──────────────────────────────────────────
#define SSD1306_WHITE SH110X_WHITE
#define SSD1306_BLACK SH110X_BLACK

// ── PINES ESP32 38 PINES ──────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define SDA_PIN        21 
#define SCL_PIN        22 
#define TOUCH_PIN      15 
#define BUZZER_PIN     19
#define MODE_BTN_PIN   4 

// ── Pines del Micrófono INMP441 (Cámbialos por los tuyos) ─────
#define I2S_WS  17  // Pin L/R (Word Select)
#define I2S_SCK 18  // Pin BCLK (Reloj)
#define I2S_SD  16  // Pin DIN (Datos)
#define I2S_PORT I2S_NUM_0

// ── Credenciales Wi-Fi y Configuración NTP (Ecuador) ──────────
const char* ssid       = "MARCO_1";
const char* password   = "dante0507";
const char* ntpServer  = "pool.ntp.org";
const long  gmtOffset_sec = -18000; // UTC-5 (Ecuador) en segundos
const int   daylightOffset_sec = 0; // Sin horario de verano

// ── Variables del Selector de Modos ───────────────────────────
int appMode = 0; // 0 = Mascota, 1 = Reloj, 2 = Pomodoro
int buttonState = HIGH;
bool lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 

// ── Variables Modo Pomodoro ───────────────────────────────
#define MINUTOS_ENFOQUE 25   
#define MINUTOS_DESCANSO 5   

int pomoState = 0; // 0: Detenido/Pausado, 1: Enfoque, 2: Descanso
int pomoSeconds = MINUTOS_ENFOQUE * 60; 
unsigned long lastPomoTick = 0;
bool lastPomoTouch = false;

// NUEVAS variables para contar toques en el Pomodoro
int pomoTapCount = 0;
unsigned long lastPomoTapTime = 0;

// Arrays en español para la fecha
const char* diasSemana[] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};
const char* mesesAnio[] = {"Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};
bool timeSynced = false; // ¿Ya descargamos la hora de internet?
bool wifiFailed = false; // ¿Falló el intento de conexión?

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Bitmaps (Iconos) ──────────────────────────────────────────
const unsigned char bmp_heart[] PROGMEM = { 0x00,0x00,0x0c,0x60,0x1e,0xf0,0x3f,0xf8,0x7f,0xfc,0x7f,0xfc,0x7f,0xfc,0x3f,0xf8,0x1f,0xf0,0x0f,0xe0,0x07,0xc0,0x03,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_zzz[]   PROGMEM = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,0x30,0x00,0x7e,0x00,0x00,0x3c,0x00,0x0c,0x00,0x18,0x00,0x30,0x00,0x7c,0x00,0x00,0x00,0x00,0x00 };
const unsigned char bmp_anger[] PROGMEM = { 0x00,0x00,0x11,0x10,0x2a,0x90,0x44,0x40,0x80,0x20,0x80,0x20,0x44,0x40,0x2a,0x90,0x11,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

// ── Moods (Estados de ánimo) ──────────────────────────────────
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
int currentMood = MOOD_NORMAL;

// ── Variables para la reacción al sonido ──────────────────────
unsigned long lastLoudNoiseTime = 0;
int moodBeforeNoise = MOOD_NORMAL;
bool isStartled = false;

// ── Struct Eye ────────────────────────────────────────────────
struct Eye {
  float x, y, w, h;
  float targetX, targetY, targetW, targetH;
  float pupilX, pupilY, targetPupilX, targetPupilY;
  float velX, velY, velW, velH, pVelX, pVelY;
  float k=0.12, d=0.60, pk=0.08, pd=0.50;
  bool blinking = false;
  unsigned long lastBlink = 0;
  unsigned long nextBlinkTime = 0;

  void init(float _x, float _y, float _w, float _h) {
    x=targetX=_x; y=targetY=_y; w=targetW=_w; h=targetH=_h;
    pupilX=targetPupilX=0; pupilY=targetPupilY=0;
    velX=velY=velW=velH=pVelX=pVelY=0;
    nextBlinkTime = millis() + 1000 + (esp_random() % 3000);
  }

  void update() {
    float ax=(targetX-x)*k, ay=(targetY-y)*k;
    float aw=(targetW-w)*k, ah=(targetH-h)*k;
    velX=(velX+ax)*d; velY=(velY+ay)*d;
    velW=(velW+aw)*d; velH=(velH+ah)*d;
    x+=velX; y+=velY; w+=velW; h+=velH;
    float pax=(targetPupilX-pupilX)*pk, pay=(targetPupilY-pupilY)*pk;
    pVelX=(pVelX+pax)*pd; pVelY=(pVelY+pay)*pd;
    pupilX+=pVelX; pupilY+=pVelY;
  }
};

Eye leftEye, rightEye;
unsigned long lastSaccade = 0;
unsigned long saccadeInterval = 3000;
float breathVal = 0;

void updatePhysicsAndMood() {
  unsigned long now = millis();
  breathVal = sin(now / 800.0) * 1.5;

  // 1. Control de parpadeos
  if (now > leftEye.nextBlinkTime) {
    leftEye.blinking = true; 
    leftEye.lastBlink = now; 
    rightEye.blinking = true;
    leftEye.nextBlinkTime = now + 2000 + (esp_random() % 4000); 
  }
  
  if (leftEye.blinking) {
    leftEye.targetH = 2; 
    rightEye.targetH = 2;
    if (now - leftEye.lastBlink > 120) { 
      leftEye.blinking = false; 
      rightEye.blinking = false; 
    }
  }

  // 2. Movimientos rápidos de pupila (Saccades)
  if (!leftEye.blinking && now - lastSaccade > saccadeInterval && currentMood != MOOD_DIZZY) {
    lastSaccade = now; 
    saccadeInterval = 500 + (esp_random() % 2500); 
    
    int dir = esp_random() % 10; 
    float lx = 0, ly = 0;
    
    if      (dir == 4) { lx = -6; ly = -4; } 
    else if (dir == 5) { lx = 6;  ly = -4; }
    else if (dir == 6) { lx = -6; ly = 4;  } 
    else if (dir == 7) { lx = 6;  ly = 4;  }
    else if (dir == 8) { lx = 8;  ly = 0;  } 
    else if (dir == 9) { lx = -8; ly = 0;  }
    
    leftEye.targetPupilX = lx;  
    leftEye.targetPupilY = ly;
    rightEye.targetPupilX = lx; 
    rightEye.targetPupilY = ly;
    
    leftEye.targetX  = 18 + (lx * 0.3); 
    leftEye.targetY  = 14 + (ly * 0.3);
    rightEye.targetX = 74 + (lx * 0.3); 
    rightEye.targetY = 14 + (ly * 0.3);
  }

  // 3. Físicas base por estado de ánimo
  if (!leftEye.blinking) {
    float baseW = 36, baseH = 36 + breathVal;
    
    switch(currentMood) {
      case MOOD_NORMAL:
        leftEye.targetW = baseW;  leftEye.targetH = baseH;
        rightEye.targetW = baseW; rightEye.targetH = baseH; 
        
        // Regresa la mirada al centro si no está haciendo un saccade
        if (now - lastSaccade > 500) {
          leftEye.targetY = 14; rightEye.targetY = 14;
          leftEye.targetPupilX = 0; rightEye.targetPupilX = 0;
          leftEye.targetPupilY = 0; rightEye.targetPupilY = 0;
        }
        break;
        
      case MOOD_HAPPY:
      case MOOD_LOVE:
        leftEye.targetW = 40;  leftEye.targetH = 32;
        rightEye.targetW = 40; rightEye.targetH = 32; 
        leftEye.targetY = 14;  rightEye.targetY = 14;
        leftEye.targetPupilY = 0; rightEye.targetPupilY = 0;
        break;
        
      case MOOD_SURPRISED:
        leftEye.targetW = 30 + (esp_random() % 2);  
        leftEye.targetH = 45 + (esp_random() % 2);
        rightEye.targetW = 30 + (esp_random() % 2); 
        rightEye.targetH = 45 + (esp_random() % 2); 
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
        
      case MOOD_SLEEPY:
        leftEye.targetW = 38;  leftEye.targetH = 30;
        rightEye.targetW = 38; rightEye.targetH = 30; 
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
        
      case MOOD_ANGRY:
        leftEye.targetW = 34;  leftEye.targetH = 32;
        rightEye.targetW = 34; rightEye.targetH = 32; 
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
        
      case MOOD_SAD:
        leftEye.targetW = 34;  leftEye.targetH = 40;
        rightEye.targetW = 34; rightEye.targetH = 40; 
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
        
      case MOOD_SUSPICIOUS:
        leftEye.targetW = 36;  leftEye.targetH = 20;
        rightEye.targetW = 36; rightEye.targetH = 42; 
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
        
      case MOOD_EXCITED:
        leftEye.targetW = 42;  leftEye.targetH = 38;
        rightEye.targetW = 42; rightEye.targetH = 38; 
        leftEye.targetPupilY = -5; rightEye.targetPupilY = -5; 
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
        
      case MOOD_DIZZY:
        leftEye.targetW  = 34 + sin(now / 80.0) * 6;
        leftEye.targetH  = 34 + cos(now / 80.0) * 6;
        rightEye.targetW = 34 + cos(now / 80.0) * 6;
        rightEye.targetH = 34 + sin(now / 80.0) * 6;
        
        leftEye.targetPupilX  = sin(now / 150.0) * 12;
        leftEye.targetPupilY  = cos(now / 150.0) * 12;
        rightEye.targetPupilX = sin((now / 150.0) + PI) * 12;
        rightEye.targetPupilY = cos((now / 150.0) + PI) * 12;
        
        leftEye.targetY = 14; rightEye.targetY = 14;
        break;
    }
  }
  
  // ¡El motor de físicas hace su magia aquí!
  leftEye.update(); 
  rightEye.update();
}

void drawEyelidMask(float x, float y, float w, float h, int mood, bool isLeft) {
  int ix=(int)x, iy=(int)y, iw=(int)w, ih=(int)h;
  if (mood == MOOD_ANGRY) {
    if (isLeft) for(int i=0;i<16;i++) display.drawLine(ix,iy-6+i,ix+iw,iy+i,SSD1306_BLACK);
    else        for(int i=0;i<16;i++) display.drawLine(ix,iy+i,ix+iw,iy-6+i,SSD1306_BLACK);
  } else if (mood == MOOD_SAD) {
    if (isLeft) for(int i=0;i<16;i++) display.drawLine(ix,iy+i,ix+iw,iy-6+i,SSD1306_BLACK);
    else        for(int i=0;i<16;i++) display.drawLine(ix,iy-6+i,ix+iw,iy+i,SSD1306_BLACK);
  } else if (mood==MOOD_HAPPY || mood==MOOD_LOVE || mood==MOOD_EXCITED) {
    display.fillRect(ix, iy+ih-12, iw, 14, SSD1306_BLACK);
    display.fillCircle(ix+iw/2, iy+ih+6, (int)(iw/1.3), SSD1306_BLACK);
  } else if (mood == MOOD_SLEEPY) {
    display.fillRect(ix, iy, iw, ih/2+2, SSD1306_BLACK);
  }
}

void drawUltraProEye(Eye& e, bool isLeft) {
  int ix=(int)e.x, iy=(int)e.y, iw=(int)e.w, ih=(int)e.h;
  int r=(iw<20)?3:8;
  display.fillRoundRect(ix, iy, iw, ih, r, SSD1306_WHITE);
  int cx=ix+iw/2, cy=iy+ih/2;
  int pw=(int)(iw/2.2), ph=(int)(ih/2.2);
  int px=cx+(int)e.pupilX-(pw/2), py=cy+(int)e.pupilY-(ph/2);
  if(px<ix) px=ix; if(px+pw>ix+iw) px=ix+iw-pw;
  if(py<iy) py=iy; if(py+ph>iy+ih) py=iy+ih-ph;
  display.fillRoundRect(px, py, pw, ph, r/2, SSD1306_BLACK);
  if(iw>15 && ih>15) display.fillCircle(px+pw-4, py+4, 2, SSD1306_WHITE);
  drawEyelidMask(e.x, e.y, e.w, e.h, currentMood, isLeft);
}

void renderFrame() {
  display.clearDisplay();
  updatePhysicsAndMood();
  if      (currentMood==MOOD_LOVE)   display.drawBitmap(56, 0, bmp_heart, 16, 16, SSD1306_WHITE);
  else if (currentMood==MOOD_SLEEPY) display.drawBitmap(110,0, bmp_zzz,   16, 16, SSD1306_WHITE);
  else if (currentMood==MOOD_ANGRY)  display.drawBitmap(56, 0, bmp_anger, 16, 16, SSD1306_WHITE);
  drawUltraProEye(leftEye, true);
  drawUltraProEye(rightEye, false);
  display.display();
}

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (appMode == 0) renderFrame(); // Solo animar si estamos en modo mascota
    delay(1);
  }
}

// ── Notas y Melodías ──────────────────────────────────────────
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_G4  392
#define NOTE_B5  988
#define NOTE_C6  1047

void n(float freq, int dur) {
  if (freq == 0) smartDelay(dur);
  else {
    tone(BUZZER_PIN, (int)freq, (int)(dur * 0.9));
    smartDelay((unsigned long)(dur * 1.0));
  }
}

void tetrisTheme() {
  float q = 300.0;
  float e = 150.0;
  for (int vuelta = 0; vuelta < 3; vuelta++) {
    n(NOTE_E5, q); n(NOTE_B4, e); n(NOTE_C5, e); n(NOTE_D5, q); n(NOTE_C5, e); n(NOTE_B4, e);
    n(NOTE_A4, q); n(NOTE_A4, e); n(NOTE_C5, e); n(NOTE_E5, q); n(NOTE_D5, e); n(NOTE_C5, e);
    n(NOTE_B4, q+e); n(NOTE_C5, e); n(NOTE_D5, q); n(NOTE_E5, q);
    n(NOTE_C5, q); n(NOTE_A4, q); n(NOTE_A4, q); n(0, q);
    n(NOTE_D5, q+e); n(NOTE_F5, e); n(NOTE_A5, q); n(NOTE_G5, e); n(NOTE_F5, e);
    n(NOTE_E5, q+e); n(NOTE_C5, e); n(NOTE_E5, q); n(NOTE_D5, e); n(NOTE_C5, e);
    n(NOTE_B4, q); n(NOTE_B4, e); n(NOTE_C5, e); n(NOTE_D5, q); n(NOTE_E5, q);
    n(NOTE_C5, q); n(NOTE_A4, q); n(NOTE_A4, q); n(0, q);
    q = q * 0.80; 
    e = e * 0.80; 
  }
}

void monsterHunterClear() {
  int t = 150;
  n(NOTE_G4, t); n(NOTE_C5, t); n(NOTE_E5, t); n(NOTE_G5, 400); n(NOTE_E5, 150); n(NOTE_G5, 600); 
}

void soTasty() {
  n(NOTE_G5, 100); n(NOTE_A5, 100); n(NOTE_B5, 100); n(NOTE_C6, 400);
}

// ── Lógica Táctil (Solo funciona en Modo Mascota) ─────────────
bool          lastTouchState = false;
unsigned long lastTapTime    = 0;
int           tapCount       = 0;

void handleTouch() {
  if (appMode != 0) return; 

  unsigned long now = millis();
  bool touched = digitalRead(TOUCH_PIN);

  if (!touched && lastTouchState) {
    tapCount++;
    lastTapTime = now;
  }

  if (tapCount > 0 && !touched && (now - lastTapTime > 600)) {
    if (tapCount >= 3) {
      int moodPrev = currentMood;
      currentMood = MOOD_HAPPY; 
      tetrisTheme();
      currentMood = moodPrev;
    } else {
      // --- CAMBIO LINEAL POR TOQUE ---
      currentMood++; // Avanza a la siguiente cara de la lista
      
      // Si nos pasamos del último mood (MOOD_DIZZY que es 9), volvemos al inicio
      if (currentMood > 9) {
        currentMood = 0; 
      }
      
      Serial.print("Tap detectado: Cambiando a cara #");
      Serial.println(currentMood);
    }
    tapCount = 0;
  }
  lastTouchState = touched;
} 

// ── Lógica del Botón Selector de Modos ────────────────────────
void handleModeButton() {
  int reading = digitalRead(MODE_BTN_PIN);

  if (reading != lastBtnState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        appMode++;
        if (appMode > 2) appMode = 0; 
        
        if (appMode == 1) wifiFailed = false;
        // Reiniciamos el Pomodoro por seguridad al cambiar de modo
        pomoState = 0; 
        pomoSeconds = MINUTOS_ENFOQUE * 60; 
        
        Serial.print("Cambio a Modo: "); Serial.println(appMode);
      }
    }
  }
  lastBtnState = reading;
}

// ── INICIAR MICRÓFONO ──
void initMicrophone() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

// ── ESCUCHAR Y REACCIONAR AL ENTORNO ──
void escucharEntorno() {
  int16_t sampleBuffer[256];
  size_t bytesRead;
  
  // Leemos un pequeño fragmento de sonido
  i2s_read(I2S_PORT, &sampleBuffer, sizeof(sampleBuffer), &bytesRead, portMAX_DELAY);
  int samples = bytesRead / 2;

  int maxVal = -32768;
  int minVal = 32767;

  // Buscamos el pico más alto de volumen en ese fragmento
  for (int i = 0; i < samples; i++) {
    if (sampleBuffer[i] > maxVal) maxVal = sampleBuffer[i];
    if (sampleBuffer[i] < minVal) minVal = sampleBuffer[i];
  }

  int amplitud = maxVal - minVal;

// Si hay un ruido fuerte (> 15000) y Mochi no está ya asustado
  if (amplitud > 15000 && !isStartled) { 
    moodBeforeNoise = currentMood; // Recordamos el estado previo
    
    // --- SELECCIÓN ALEATORIA DE REACCIÓN ---
    // Definimos una lista de moods "reactivos"
    int reacciones[] = {MOOD_SURPRISED, MOOD_ANGRY, MOOD_DIZZY, MOOD_SAD, MOOD_SUSPICIOUS};
    
    currentMood = reacciones[esp_random() % 5];

    isStartled = true;
    lastLoudNoiseTime = millis();
    Serial.print("¡Reacción aleatoria detectada! Mood: "); Serial.println(currentMood);
  }

  // Después de 2 segundos (2000 ms), se le pasa el susto y vuelve a la normalidad
  if (isStartled && (millis() - lastLoudNoiseTime > 2000)) {
    currentMood = moodBeforeNoise;
    isStartled = false;
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500); // Pequeña pausa para que el Monitor Serie despierte
  Serial.println("\n--- INICIANDO MOCHI ---");

  // Iniciar Bluetooth
  SerialBT.begin("Mochi_Robot"); // Así lo vas a buscar en tu celular
  Serial.println("Bluetooth Iniciado. ¡Empareja tu celular!");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MODE_BTN_PIN, INPUT_PULLUP); 


  // Iniciar OLED
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 30);
  display.print("Iniciando Mochi...");
  display.display();

  // Iniciar hardware de mochi
  leftEye.init(18, 14, 36, 36);
  rightEye.init(74, 14, 36, 36);
  // Inicar Microfono
  initMicrophone();

}

// ── Loop Principal ────────────────────────────────────────────
void loop() {
  handleModeButton(); // Revisar si se presionó el botón físico

  if (appMode == 0) {
    // ── MODO 0: MASCOTA ──
    handleTouch();
    escucharEntorno(); // ¡NUEVO! Mochi presta atención al sonido
    renderFrame();
    
  } else if (appMode == 1) {
    // ── MODO 1: RELOJ CON WI-FI BAJO DEMANDA ──
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Si no tenemos la hora y no hemos fallado previamente, intentamos conectar
    if (!timeSynced && !wifiFailed) {
      display.setTextSize(1);
      display.setCursor(10, 30);
      display.print("Conectando Wi-Fi...");
      display.display();

      WiFi.mode(WIFI_STA); // Encender antena
      WiFi.begin(ssid, password);
      
      int intentos = 0;
      // Esperar conexión MÁXIMO 5 segundos (10 intentos de 500ms)
      while (WiFi.status() != WL_CONNECTED && intentos < 10) {
        delay(500);
        intentos++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 5000)) { // Darle 5 segundos al servidor NTP
          timeSynced = true; // ¡Éxito!
        }
      }

      // Si después de todo no se logró la hora
      if (!timeSynced) {
        wifiFailed = true; 
        display.clearDisplay();
        display.setCursor(15, 30);
        display.print("Error al conectar");
        display.display();
        delay(2000); // Mostrar el error por 2 segundos antes de continuar
      }

      // IMPORTANTE: Siempre apagar la antena al terminar para evitar colapsos
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }

    // ── DIBUJAR PANTALLA ──
    if (timeSynced) {
      // Si la hora está sincronizada, el ESP32 la mantiene internamente (no necesita más internet)
      struct tm timeinfo;
      if(getLocalTime(&timeinfo)){
        char timeStringBuff[10];
        strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
        display.setTextSize(3);
        int tw = strlen(timeStringBuff) * 18;
        display.setCursor((SCREEN_WIDTH - tw) / 2, 10);
        display.print(timeStringBuff);

        char dateStringBuff[30];
        sprintf(dateStringBuff, "%s %02d %s", diasSemana[timeinfo.tm_wday], timeinfo.tm_mday, mesesAnio[timeinfo.tm_mon]);
        display.setTextSize(1);
        int dw = strlen(dateStringBuff) * 6;
        display.setCursor((SCREEN_WIDTH - dw) / 2, 48);
        display.print(dateStringBuff);
      }
    } else {
      // Si falló el Wi-Fi, muestra una pantalla de "Servicio no disponible"
      display.setTextSize(1);
      display.setCursor(5, 20);
      display.print("Sin conexion Wi-Fi");
      display.setCursor(15, 40);
      display.print("Pulsa el boton ->");
    }
    
    display.display();
    delay(100); 

  } else if (appMode == 2) {
    // ── MODO 2: PRODUCTIVIDAD (POMODORO BLUETOOTH) ──
    
    // 1. Lógica Bluetooth (Leer lo que manda la App del celular)
    if (SerialBT.available()) {
      String comando = SerialBT.readStringUntil('\n');
      comando.trim(); // Limpiar espacios ocultos
      comando.toLowerCase(); // Convertir a minúsculas por si la app manda "Iniciar" con mayúscula
      
// --- MINI-TRADUCTOR DE TEXTO A DÍGITOS ---
      // Por si Google decide escribir el número con letras
      comando.replace(" un ", " 1 ");
      
      Serial.print("Comando de App: ");
      Serial.println(comando);

      // --- EXTRACTOR DE NÚMEROS MATEMÁTICOS ---
      int minutosModificar = 5; // Valor por defecto si no especificas un número
      String numeroEncontrado = "";
      
      // Revisamos la frase letra por letra buscando números
      for (int i = 0; i < comando.length(); i++) {
        if (isDigit(comando[i])) {
          numeroEncontrado += comando[i]; // Juntamos los números (ej. '1' y '5' = "15")
        } else if (numeroEncontrado.length() > 0) {
          break; // Si ya encontró un número y luego hay letras, dejamos de buscar
        }
      }
      
      if (numeroEncontrado.length() > 0) {
        minutosModificar = numeroEncontrado.toInt(); // Convertimos el texto "15" al valor matemático 15
      }
      // --- DICCIONARIO DE SINÓNIMOS INTELIGENTE ---
      // Buscamos raíces de palabras o sinónimos usando "||" (Significa: O)
      bool quiereIniciar = (comando.indexOf("inici") >= 0 || comando.indexOf("arranca") >= 0 || comando.indexOf("comenza") >= 0);
      bool quierePausar  = (comando.indexOf("paus") >= 0 || comando.indexOf("deten") >= 0 || comando.indexOf("para") >= 0);
      bool quiereSumar   = (comando.indexOf("suma") >= 0 || comando.indexOf("agrega") >= 0 || comando.indexOf("mas") >= 0 || comando.indexOf("más") >= 0);
      bool quiereRestar  = (comando.indexOf("resta") >= 0 || comando.indexOf("quita") >= 0 || comando.indexOf("menos") >= 0);
      bool quiereResetear  = (comando.indexOf("reset") >= 0 || comando.indexOf("repit") >= 0 || comando.indexOf("regres") >= 0 || comando.indexOf("reinici") >= 0);

      // --- EJECUCIÓN DE COMANDOS ---

      if (quiereResetear) {
        pomoState = 0;
        pomoSeconds = MINUTOS_ENFOQUE * 60;
        Serial.println("¡Mochi reseteo el reloj!");
      }

      if (quiereIniciar && pomoState == 0) {
        pomoState = 1; 
        lastPomoTick = millis();
        Serial.println("¡Mochi arranca el reloj!");
      } 
      else if (quierePausar && pomoState != 0) {
        pomoState = 0; // Pausa
        Serial.println("¡Mochi pausa el reloj!");
      }
      
      if (quiereSumar) {
        pomoSeconds += (minutosModificar * 60); // Sumar 5 minutos
        Serial.println("¡Mochi suma " + String(minutosModificar) + " minutos!");
      } 
      
      if (quiereRestar) {
        pomoSeconds -= (minutosModificar * 60); // Restar 5 minutos
        if (pomoSeconds < 0) pomoSeconds = 0; 
        Serial.println("¡Mochi resta " + String(minutosModificar) + " minutos!");
      }

    }

// 2. Lógica Táctil (1 toque: Iniciar/Pausar | 3 toques: Reiniciar)
    bool touched = digitalRead(TOUCH_PIN);
    
    // Detectar cuando el dedo toca el sensor (flanco de subida)
    if (touched && !lastPomoTouch) {
      pomoTapCount++;
      lastPomoTapTime = millis();

      // Ejecutar la pausa/inicio SOLO en el primer toque
      if (pomoTapCount == 1) {
        if (pomoState == 0) {
          pomoState = 1; 
          lastPomoTick = millis();
        } else {
          pomoState = 0; // Pausa el reloj
        }
      }
    }
    lastPomoTouch = touched;

    // Evaluar los toques después de que pase medio segundo (500ms) sin tocar
    if (pomoTapCount > 0 && !touched && (millis() - lastPomoTapTime > 500)) {
      if (pomoTapCount >= 3) {
        // ¡REINICIO! Si detectó 3 o más toques rápidos
        pomoState = 0; // Lo dejamos pausado
        pomoSeconds = MINUTOS_ENFOQUE * 60; // Vuelve al tiempo original
        
        Serial.println("¡Mochi reinicia el reloj por 3 toques!");
        
        // Hacemos un pequeño "beep" agudo para que sepas que funcionó el reinicio
        tone(BUZZER_PIN, 1200, 150); 
      }
      pomoTapCount = 0; // Resetear el contador de toques para la próxima
    }

    // 3. Lógica del Temporizador (No bloqueante)
    if (pomoState != 0) {
      if (millis() - lastPomoTick >= 1000) {
        lastPomoTick = millis();
        if (pomoSeconds > 0) {
          pomoSeconds--;
        } else {
          // ¡Se acabó el tiempo!
          if (pomoState == 1) { 
            pomoState = 2;
            pomoSeconds = MINUTOS_DESCANSO * 60; 
            monsterHunterClear(); // Suena al terminar de estudiar
          } else if (pomoState == 2) { 
            pomoState = 0;
            pomoSeconds = MINUTOS_ENFOQUE * 60;
            soTasty(); // Suena al terminar el descanso
          }
        }
      }
    }

    // 4. Dibujar la pantalla del Pomodoro
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    display.setTextSize(1);
    display.setCursor(35, 0);
    if (pomoState == 0) display.print("POMODORO");
    else if (pomoState == 1) display.print("ENFOQUE!");
    else display.print("DESCANSO");

    char pomoStr[10];
    sprintf(pomoStr, "%02d:%02d", pomoSeconds / 60, pomoSeconds % 60);
    
    display.setTextSize(3);
    int tw = strlen(pomoStr) * 18;
    display.setCursor((SCREEN_WIDTH - tw) / 2, 25);
    display.print(pomoStr);
    
    if (pomoState == 0) {
      display.setTextSize(1);
      display.setCursor(15, 55);
      display.print("Envia voz por App");
    }

    display.display();
  }
}