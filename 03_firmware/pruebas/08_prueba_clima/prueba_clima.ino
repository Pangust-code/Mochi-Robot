// ═══════════════════════════════════════════════════════════════════════
//  CLIMA — ESP32-C6 Supermini — Estación Meteorológica
//
//  Un toque corto → ciclar entre vistas:
//    0 = Condición actual  (icono + temp + descripción + humedad/viento)
//    1 = Pronóstico del día (min/max + icono grande)
//    2 = Pronóstico 3 días  (3 columnas con icono + día + temp)
//
//  Toque largo (>800ms) → forzar actualización de la API
//
//  Pines ESP32-C6 Supermini:
//    OLED  SDA=6  SCL=7  (I2C, 0x3C, SH1106 128x64)
//    Buzzer=19   Touch TTP223=2
//
//  Librerías necesarias:
//    Adafruit GFX, Adafruit SH110X, ArduinoJson (v6 o v7), HTTPClient
//  API: openweathermap.org (clave gratuita)
// ═══════════════════════════════════════════════════════════════════════

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>

// ── Configuración del usuario ──────────────────────────────────────────
const char* WIFI_SSID   = "TU_RED_WIFI";
const char* WIFI_PASS   = "TU_CONTRASEÑA";
const char* OWM_API_KEY = "5aafee3b82187abfbc856645942658ef";   // openweathermap.org → gratuito
const char* OWM_CITY    = "Cuenca";
const char* OWM_COUNTRY = "EC";           // código ISO del país
const char* OWM_UNITS   = "metric";       // metric=°C, imperial=°F
const long  GMT_OFFSET  = -18000;         // UTC-5 Ecuador (segundos)

// ── Pines ──────────────────────────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define SDA_PIN         6
#define SCL_PIN         7
#define TOUCH_PIN       2
#define BUZZER_PIN     19

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Touch ──────────────────────────────────────────────────────────────
#define HOLD_MS       800
#define TAP_WINDOW_MS 400

static bool          tc_last   = false;
static unsigned long tc_downAt = 0;
static unsigned long tc_tapAt  = 0;
static int           tc_taps   = 0;

bool ev_tap  = false;
bool ev_hold = false;

void procesTouch() {
  unsigned long now = millis();
  bool cur = digitalRead(TOUCH_PIN);
  ev_tap = ev_hold = false;

  if (cur && !tc_last)  tc_downAt = now;

  if (!cur && tc_last) {
    unsigned long held = now - tc_downAt;
    if (held >= HOLD_MS) {
      ev_hold = true;
      tc_taps = 0;
    } else {
      ev_tap = true;
      tc_taps++;
      tc_tapAt = now;
    }
  }

  if (tc_taps > 0 && !cur && (now - tc_tapAt > TAP_WINDOW_MS))
    tc_taps = 0;

  tc_last = cur;
}

// ── Datos del clima ────────────────────────────────────────────────────
struct DayForecast {
  float tempMin, tempMax;
  int   weatherId;
  char  dayName[5];
};

struct WeatherData {
  float temp, tempMin, tempMax;
  float humidity, windSpeed;
  int   weatherId;
  bool  isNight;
  char  desc[40];
  char  city[24];
  char  timeStr[8];
  DayForecast forecast[3];
  bool         valid     = false;
  unsigned long fetchedAt = 0;
};

WeatherData wx;
int viewMode = 0;
#define UPDATE_INTERVAL_MS 600000UL  // 10 minutos

// ── Categoría del clima ────────────────────────────────────────────────
// 0=sol  1=luna(noche clara)  2=sol+nube  3=nube  4=lluvia  5=nieve  6=tormenta
int weatherCategory(int id, bool isNight) {
  if (id == 800)               return isNight ? 1 : 0;
  if (id == 801 || id == 802)  return isNight ? 1 : 2;
  if (id == 803 || id == 804)  return 3;
  if (id >= 200 && id < 300)   return 6;
  if (id >= 300 && id < 600)   return 4;
  if (id >= 600 && id < 700)   return 5;
  return 3;
}

// ── Icono grande (32×32) centrado en (cx, cy) ─────────────────────────
void drawIconLarge(int cx, int cy, int cat) {
  switch (cat) {

    case 0: // Sol
      display.fillCircle(cx, cy, 8, SH110X_WHITE);
      for (int a = 0; a < 360; a += 45) {
        float r = a * PI / 180.0f;
        display.drawLine(
          cx + (int)(11*cosf(r)), cy + (int)(11*sinf(r)),
          cx + (int)(15*cosf(r)), cy + (int)(15*sinf(r)),
          SH110X_WHITE);
      }
      break;

    case 1: // Luna creciente
      display.fillCircle(cx - 1, cy, 11, SH110X_WHITE);
      display.fillCircle(cx + 5, cy - 4, 9, SH110X_BLACK);
      break;

    case 2: // Sol + nube (parcialmente nublado)
      // Sol pequeño arriba-derecha
      display.fillCircle(cx + 7, cy - 8, 6, SH110X_WHITE);
      for (int a = 0; a < 360; a += 60) {
        float r = a * PI / 180.0f;
        display.drawLine(
          (cx+7)+(int)(8*cosf(r)), (cy-8)+(int)(8*sinf(r)),
          (cx+7)+(int)(11*cosf(r)),(cy-8)+(int)(11*sinf(r)),
          SH110X_WHITE);
      }
      // Nube abajo-izquierda
      display.fillRoundRect(cx - 14, cy + 1, 28, 12, 5, SH110X_WHITE);
      display.fillCircle(cx - 4, cy + 2, 8, SH110X_WHITE);
      display.fillCircle(cx + 5, cy - 1, 7, SH110X_WHITE);
      break;

    case 3: // Nube
      display.fillRoundRect(cx - 12, cy - 1, 26, 12, 5, SH110X_WHITE);
      display.fillCircle(cx - 3, cy - 2, 9, SH110X_WHITE);
      display.fillCircle(cx + 6, cy - 5, 7, SH110X_WHITE);
      break;

    case 4: // Lluvia
      display.fillRoundRect(cx - 12, cy - 8, 26, 11, 4, SH110X_WHITE);
      display.fillCircle(cx - 4, cy - 9, 7, SH110X_WHITE);
      display.fillCircle(cx + 6, cy - 11, 6, SH110X_WHITE);
      for (int i = 0; i < 4; i++) {
        int dx = cx - 11 + i * 8;
        display.drawLine(dx,    cy + 5, dx - 2, cy + 12, SH110X_WHITE);
      }
      break;

    case 5: // Nieve (copo)
      display.drawLine(cx, cy - 13, cx, cy + 13, SH110X_WHITE);
      display.drawLine(cx - 13, cy, cx + 13, cy, SH110X_WHITE);
      display.drawLine(cx - 9, cy - 9, cx + 9, cy + 9, SH110X_WHITE);
      display.drawLine(cx + 9, cy - 9, cx - 9, cy + 9, SH110X_WHITE);
      display.fillCircle(cx, cy, 3, SH110X_WHITE);
      display.fillCircle(cx, cy - 12, 2, SH110X_WHITE);
      display.fillCircle(cx, cy + 12, 2, SH110X_WHITE);
      display.fillCircle(cx - 12, cy, 2, SH110X_WHITE);
      display.fillCircle(cx + 12, cy, 2, SH110X_WHITE);
      break;

    case 6: // Tormenta
      display.fillRoundRect(cx - 12, cy - 11, 26, 11, 4, SH110X_WHITE);
      display.fillCircle(cx - 4, cy - 12, 7, SH110X_WHITE);
      display.fillCircle(cx + 6, cy - 14, 6, SH110X_WHITE);
      display.drawLine(cx - 1, cy + 2, cx - 6, cy + 11, SH110X_WHITE);
      display.drawLine(cx - 6, cy + 11, cx + 2, cy + 9, SH110X_WHITE);
      display.drawLine(cx + 2, cy + 9, cx - 3, cy + 17, SH110X_WHITE);
      break;
  }
}

// ── Icono pequeño (16×16) centrado en (cx, cy) ────────────────────────
void drawIconSmall(int cx, int cy, int cat) {
  switch (cat) {

    case 0: // Sol
      display.fillCircle(cx, cy, 4, SH110X_WHITE);
      for (int a = 0; a < 360; a += 45) {
        float r = a * PI / 180.0f;
        display.drawLine(
          cx+(int)(6*cosf(r)), cy+(int)(6*sinf(r)),
          cx+(int)(8*cosf(r)), cy+(int)(8*sinf(r)),
          SH110X_WHITE);
      }
      break;

    case 1: // Luna
      display.fillCircle(cx, cy, 5, SH110X_WHITE);
      display.fillCircle(cx + 3, cy - 2, 4, SH110X_BLACK);
      break;

    case 2: // Sol + nube
      display.fillCircle(cx + 3, cy - 4, 3, SH110X_WHITE);
      display.fillRoundRect(cx - 6, cy, 12, 6, 2, SH110X_WHITE);
      display.fillCircle(cx - 2, cy + 1, 4, SH110X_WHITE);
      display.fillCircle(cx + 3, cy - 1, 3, SH110X_WHITE);
      break;

    case 3: // Nube
      display.fillRoundRect(cx - 6, cy - 2, 12, 7, 2, SH110X_WHITE);
      display.fillCircle(cx - 2, cy - 2, 4, SH110X_WHITE);
      display.fillCircle(cx + 3, cy - 4, 3, SH110X_WHITE);
      break;

    case 4: // Lluvia
      display.fillRoundRect(cx - 5, cy - 5, 10, 5, 2, SH110X_WHITE);
      display.fillCircle(cx - 2, cy - 5, 3, SH110X_WHITE);
      display.drawLine(cx - 3, cy + 2, cx - 4, cy + 6, SH110X_WHITE);
      display.drawLine(cx,     cy + 2, cx - 1, cy + 6, SH110X_WHITE);
      display.drawLine(cx + 3, cy + 2, cx + 2, cy + 6, SH110X_WHITE);
      break;

    case 5: // Nieve
      display.drawLine(cx, cy - 6, cx, cy + 6, SH110X_WHITE);
      display.drawLine(cx - 6, cy, cx + 6, cy, SH110X_WHITE);
      display.drawLine(cx - 4, cy - 4, cx + 4, cy + 4, SH110X_WHITE);
      display.drawLine(cx + 4, cy - 4, cx - 4, cy + 4, SH110X_WHITE);
      display.fillCircle(cx, cy, 2, SH110X_WHITE);
      break;

    case 6: // Tormenta
      display.fillRoundRect(cx - 5, cy - 6, 10, 5, 2, SH110X_WHITE);
      display.fillCircle(cx - 2, cy - 6, 3, SH110X_WHITE);
      display.drawLine(cx,   cy + 1, cx - 2, cy + 5, SH110X_WHITE);
      display.drawLine(cx-2, cy + 5, cx + 1, cy + 4, SH110X_WHITE);
      display.drawLine(cx+1, cy + 4, cx - 1, cy + 8, SH110X_WHITE);
      break;
  }
}

// Prototipo necesario (definida más abajo)
void showMsg(const char* line1, const char* line2 = nullptr);

// ── Conectar WiFi ──────────────────────────────────────────────────────
bool connectWiFi() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(4, 26);
  display.print("Conectando WiFi...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++)
    delay(500);

  return WiFi.status() == WL_CONNECTED;
}

// ── Fetch: clima actual ────────────────────────────────────────────────
bool fetchCurrent() {
  WiFiClient client;
  HTTPClient http;
  char url[280];
  snprintf(url, sizeof(url),
    "http://api.openweathermap.org/data/2.5/weather"
    "?q=%s,%s&appid=%s&units=%s&lang=es",
    OWM_CITY, OWM_COUNTRY, OWM_API_KEY, OWM_UNITS);

  Serial.println(url);
  http.begin(client, url);
  http.setTimeout(8000);
  int code = http.GET();
  Serial.printf("fetchCurrent HTTP %d\n", code);
  if (code != 200) {
    char msg[24];
    snprintf(msg, sizeof(msg), "HTTP %d", code);
    // 401=clave invalida  404=ciudad no encontrada  429=limite gratis
    const char* hint = (code == 401) ? "Clave invalida/nueva"
                     : (code == 404) ? "Ciudad no encontrada"
                     : (code == 429) ? "Limite de peticiones"
                     : "Sin respuesta";
    showMsg(msg, hint);
    delay(3000);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(2048);
#endif
  if (deserializeJson(doc, body) != DeserializationError::Ok) return false;

  wx.temp      = doc["main"]["temp"]     | 0.0f;
  wx.tempMin   = doc["main"]["temp_min"] | 0.0f;
  wx.tempMax   = doc["main"]["temp_max"] | 0.0f;
  wx.humidity  = doc["main"]["humidity"] | 0.0f;
  wx.windSpeed = doc["wind"]["speed"]    | 0.0f;
  wx.weatherId = doc["weather"][0]["id"] | 800;

  const char* rawDesc = doc["weather"][0]["description"] | "---";
  strlcpy(wx.desc, rawDesc, sizeof(wx.desc));
  if (wx.desc[0] >= 'a') wx.desc[0] -= 32;  // capitalizar

  strlcpy(wx.city, doc["name"] | OWM_CITY, sizeof(wx.city));

  const char* icon = doc["weather"][0]["icon"] | "01d";
  wx.isNight = (strlen(icon) >= 3 && icon[2] == 'n');

  // Hora local desde timestamp de la API
  long dt = doc["dt"] | 0L;
  long tz = doc["timezone"] | GMT_OFFSET;
  long local = dt + tz;
  snprintf(wx.timeStr, sizeof(wx.timeStr), "%02d:%02d",
           (int)((local / 3600) % 24),
           (int)((local / 60) % 60));

  return true;
}

// ── Fetch: pronóstico 3 días ───────────────────────────────────────────
bool fetchForecast() {
  WiFiClient client;
  HTTPClient http;
  char url[280];
  snprintf(url, sizeof(url),
    "http://api.openweathermap.org/data/2.5/forecast"
    "?q=%s,%s&appid=%s&units=%s&lang=es&cnt=32",
    OWM_CITY, OWM_COUNTRY, OWM_API_KEY, OWM_UNITS);

  http.begin(client, url);
  http.setTimeout(8000);
  int code = http.GET();
  Serial.printf("fetchForecast HTTP %d\n", code);
  if (code != 200) { http.end(); return false; }

  String body = http.getString();
  http.end();

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(10240);
#endif
  if (deserializeJson(doc, body) != DeserializationError::Ok) return false;

  const char* dnames[] = {"Dom","Lun","Mar","Mie","Jue","Vie","Sab"};
  long tzOff = doc["city"]["timezone"] | GMT_OFFSET;

  // Día actual (en UTC local)
  long nowTs = doc["list"][0]["dt"] | 0L;
  int todayDay = (int)(((nowTs + tzOff) / 86400L) % 7);

  int filled = 0;
  int lastDay = todayDay;

  JsonArray list = doc["list"].as<JsonArray>();
  for (JsonObject item : list) {
    if (filled >= 3) break;
    long dt    = item["dt"] | 0L;
    long local = dt + tzOff;
    int  wday  = (int)((local / 86400L + 4) % 7);  // 0=Dom
    int  hour  = (int)((local / 3600) % 24);

    if (wday == lastDay) continue;       // mismo día, seguir buscando
    if (hour < 10 || hour > 15) continue; // preferir datos de mediodía

    lastDay = wday;
    DayForecast& df = wx.forecast[filled];
    df.tempMin   = item["main"]["temp_min"] | 0.0f;
    df.tempMax   = item["main"]["temp_max"] | 0.0f;
    df.weatherId = item["weather"][0]["id"] | 800;
    strlcpy(df.dayName, dnames[wday], sizeof(df.dayName));
    filled++;
  }

  // Rellenar si faltan días (datos insuficientes de la API)
  while (filled < 3) {
    DayForecast& df = wx.forecast[filled];
    df.tempMin = df.tempMax = 0;
    df.weatherId = 800;
    strlcpy(df.dayName, "---", sizeof(df.dayName));
    filled++;
  }

  return true;
}

// ── Mensajes en pantalla ───────────────────────────────────────────────
void showMsg(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print(line1);
  if (line2) { display.setCursor(0, 36); display.print(line2); }
  display.display();
}

// ── Actualizar todos los datos ─────────────────────────────────────────
void updateWeather() {
  if (!connectWiFi()) {
    showMsg("Sin WiFi", "Verifica SSID/clave");
    delay(2500);
    WiFi.mode(WIFI_OFF);
    return;
  }

  configTime(GMT_OFFSET, 0, "pool.ntp.org");

  showMsg("Obteniendo clima...");
  bool ok1 = fetchCurrent();

  showMsg("Obteniendo pronostico...");
  bool ok2 = fetchForecast();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  if (ok1) {
    wx.valid     = true;
    wx.fetchedAt = millis();
    Serial.printf("Clima OK: %s %.1f°C %s\n",
                  wx.city, wx.temp, wx.desc);
  } else {
    showMsg("Error API", "Verifica clave OWM");
    delay(2500);
  }
  if (!ok2) Serial.println("Pronostico: error (datos actuales OK)");
}

// ═══════════════════════════════════════════════════════════════════════
//  VISTAS
// ═══════════════════════════════════════════════════════════════════════

// ── Vista 0: Condición actual ──────────────────────────────────────────
//  [ICON 32]   CIUDAD
//              XX.X°C
//              Descripción
//  ──────────────────────────
//  HH:MM · XX% · X.Xm/s

void drawCurrentView() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  int cat = weatherCategory(wx.weatherId, wx.isNight);

  // Icono grande
  drawIconLarge(20, 26, cat);

  // Ciudad
  display.setTextSize(1);
  char city[13]; strlcpy(city, wx.city, sizeof(city));
  display.setCursor(44, 1);
  display.print(city);

  // Temperatura
  display.setTextSize(2);
  display.setCursor(44, 13);
  display.print((int)roundf(wx.temp));
  display.setTextSize(1);
  display.print("\xB0""C");

  // Descripción
  display.setTextSize(1);
  display.setCursor(44, 32);
  display.print(wx.desc);

  // Separador
  display.drawFastHLine(0, 46, SCREEN_WIDTH, SH110X_WHITE);

  // Barra inferior
  display.setCursor(0, 53);
  display.print(wx.timeStr);

  display.setCursor(38, 53);
  display.print("\xB7 ");
  display.print((int)wx.humidity);
  display.print("%");

  display.setCursor(76, 53);
  display.print("\xB7 ");
  char wind[8];
  snprintf(wind, sizeof(wind), "%.1fm/s", wx.windSpeed);
  display.print(wind);

  display.display();
}

// ── Vista 1: Pronóstico del día ────────────────────────────────────────
//  HOY · CIUDAD
//      [ICON 32]
//  Min XX°   Max XX°
//  Descripción

void drawTodayView() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  int cat = weatherCategory(wx.weatherId, false);

  display.setTextSize(1);
  display.setCursor(0, 1);
  display.print("HOY  ");
  display.print(wx.city);

  drawIconLarge(64, 28, cat);

  display.setTextSize(1);
  char mm[24];
  snprintf(mm, sizeof(mm), "Min %.0f\xB0   Max %.0f\xB0",
           wx.tempMin, wx.tempMax);
  display.setCursor(0, 47);
  display.print(mm);

  display.setCursor(0, 57);
  display.print(wx.desc);

  display.display();
}

// ── Vista 2: Pronóstico 3 días ─────────────────────────────────────────
//  [──3-DAY FORECAST──]
//  │ ICO │ ICO │ ICO │
//  │ Lun │ Mar │ Mie │
//  │ 15° │ 17° │ 12° │

void draw3DayView() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // Encabezado invertido
  display.fillRect(0, 0, SCREEN_WIDTH, 11, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(14, 2);
  display.print("3-DAY FORECAST");
  display.setTextColor(SH110X_WHITE);

  const int colW = SCREEN_WIDTH / 3;   // 42 px
  for (int i = 0; i < 3; i++) {
    int cx  = colW * i + colW / 2;
    int cat = weatherCategory(wx.forecast[i].weatherId, false);

    if (i > 0)
      display.drawFastVLine(colW * i, 11, SCREEN_HEIGHT - 11, SH110X_WHITE);

    // Icono pequeño
    drawIconSmall(cx, 28, cat);

    // Nombre del día
    int nw = strlen(wx.forecast[i].dayName) * 6;
    display.setCursor(cx - nw / 2, 41);
    display.print(wx.forecast[i].dayName);

    // Temperatura media
    char ts[6];
    snprintf(ts, sizeof(ts), "%.0f\xB0",
             (wx.forecast[i].tempMin + wx.forecast[i].tempMax) / 2.0f);
    int tw = strlen(ts) * 6;
    display.setCursor(cx - tw / 2, 53);
    display.print(ts);
  }

  display.display();
}

// ── Vista de error ─────────────────────────────────────────────────────
void drawNoData() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(16, 10);
  display.print("Sin datos");
  display.setCursor(0, 26);
  display.print("Toque largo para");
  display.setCursor(0, 38);
  display.print("actualizar el clima.");
  display.display();
}

// ═══════════════════════════════════════════════════════════════════════
//  SETUP
// ═══════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== CLIMA v1 ESP32-C6 ===");

  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  if (!display.begin(0x3C, true)) {
    Serial.println("OLED no encontrado — revisa SDA/SCL");
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  updateWeather();
}

// ═══════════════════════════════════════════════════════════════════════
//  LOOP
// ═══════════════════════════════════════════════════════════════════════

void loop() {
  procesTouch();

  // Tap corto → ciclar vista
  if (ev_tap) {
    viewMode = (viewMode + 1) % 3;
    tone(BUZZER_PIN, 1400, 50);
  }

  // Toque largo → actualizar ahora
  if (ev_hold) {
    tone(BUZZER_PIN, 800, 200);
    updateWeather();
  }

  // Auto-actualizar cada UPDATE_INTERVAL_MS
  if (wx.valid && millis() - wx.fetchedAt > UPDATE_INTERVAL_MS)
    updateWeather();

  // Dibujar
  if (!wx.valid) {
    drawNoData();
    delay(500);
    return;
  }

  switch (viewMode) {
    case 0: drawCurrentView(); break;
    case 1: drawTodayView();   break;
    case 2: draw3DayView();    break;
  }

  delay(80);
}
