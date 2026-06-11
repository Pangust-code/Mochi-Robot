/*
  prueba_sonido_microfono.ino
  Hardware: ESP32-C3 Super Mini + INMP441

  Verifica que el micrófono detecte sonido y que los valores tengan sentido.
  Muestra amplitud en tiempo real con una barra ASCII y clasificación de nivel.

  CÓMO USAR:
  1. Conecta el INMP441:  SCK=GPIO4  WS=GPIO1  SD=GPIO3  L/R→GND
  2. Abre el Monitor Serial a 115200 baud
  3. Observa los valores en reposo (silencio ambiental)
  4. Habla, aplaude o sopla cerca del micrófono
  5. Verifica que los valores suban al hacer ruido

  VALORES ESPERADOS:
    Silencio ambiental :    100 –  2 000
    Voz / conversación :  2 000 – 10 000
    Voz fuerte / aplauso: 10 000 – 25 000
    Saturación (muy cerca): > 30 000

  Si el valor se queda siempre en 0 o siempre en 65535:
    → Verifica la conexión del pin SD (GPIO 3)
    → Verifica que L/R esté conectado a GND
*/

#include <driver/i2s.h>

// ── Pines (deben coincidir con el firmware principal) ──────────────────────
#define PIN_SCK  4
#define PIN_WS   1
#define PIN_SD   3
#define I2S_PORT I2S_NUM_0

// ── Umbrales de clasificación ──────────────────────────────────────────────
#define UMBRAL_BAJO    2000
#define UMBRAL_MEDIO  10000
#define UMBRAL_FUERTE 25000

// ── Configuración de lectura ───────────────────────────────────────────────
#define NUM_MUESTRAS   256
#define ANCHO_BARRA     40
#define ESCALA_MAX   40000   // amplitud máxima esperada para la barra

void iniciarMicrofono() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = 16000,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 4,
    .dma_buf_len          = NUM_MUESTRAS,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_pin_config_t pines = {
    .bck_io_num   = PIN_SCK,
    .ws_io_num    = PIN_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_SD
  };
  esp_err_t r = i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  if (r != ESP_OK) {
    Serial.printf("  ERROR al instalar driver I2S: %d\n", r);
    Serial.println("  Verifica los pines SCK, WS, SD.");
    while (true) delay(1000);
  }
  i2s_set_pin(I2S_PORT, &pines);
}

// Devuelve la amplitud pico a pico del buffer leído
int leerAmplitud() {
  int16_t buf[NUM_MUESTRAS];
  size_t bytesLeidos = 0;
  i2s_read(I2S_PORT, buf, sizeof(buf), &bytesLeidos, pdMS_TO_TICKS(20));
  if (bytesLeidos == 0) return -1;

  int n = bytesLeidos / 2;
  int16_t maxVal = -32768, minVal = 32767;
  for (int i = 0; i < n; i++) {
    if (buf[i] > maxVal) maxVal = buf[i];
    if (buf[i] < minVal) minVal = buf[i];
  }
  return (int)(maxVal - minVal);
}

void imprimirBarra(int valor, int escala, int ancho) {
  int lleno = constrain(map(valor, 0, escala, 0, ancho), 0, ancho);
  Serial.print("|");
  for (int i = 0; i < ancho; i++) {
    if (i < lleno) Serial.print(i > ancho * 7 / 10 ? "!" : "#");
    else           Serial.print(" ");
  }
  Serial.print("|");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA DE SONIDO — INMP441");
  Serial.println("========================================");
  Serial.println("  SCK=GPIO4  WS=GPIO1  SD=GPIO3");
  Serial.println("  L/R → GND");
  Serial.println("========================================");
  Serial.println("  Iniciando I2S...");

  iniciarMicrofono();

  Serial.println("  Listo.");
  Serial.println();
  Serial.println("  Guia de valores:");
  Serial.printf("    Silencio      : %5d – %5d\n", 0,             UMBRAL_BAJO);
  Serial.printf("    Ruido / voz   : %5d – %5d\n", UMBRAL_BAJO,   UMBRAL_MEDIO);
  Serial.printf("    Voz fuerte    : %5d – %5d\n", UMBRAL_MEDIO,  UMBRAL_FUERTE);
  Serial.printf("    Muy fuerte    : > %d\n",       UMBRAL_FUERTE);
  Serial.println();
  Serial.println("  Amp    | Pico  | Nivel        | Barra");
  Serial.println("  -------|-------|--------------|" + String("-", ANCHO_BARRA + 2));
}

int picoMax   = 0;
int suavizado = 0;

void loop() {
  int amp = leerAmplitud();

  if (amp < 0) {
    Serial.println("  ERROR: no se leyeron datos del microfono");
    delay(500);
    return;
  }

  // Suavizado exponencial para la barra
  suavizado = (suavizado * 5 + amp * 3) / 8;
  if (amp > picoMax) picoMax = amp;

  const char* nivel;
  if      (amp < UMBRAL_BAJO)   nivel = "silencio    ";
  else if (amp < UMBRAL_MEDIO)  nivel = "ruido / voz ";
  else if (amp < UMBRAL_FUERTE) nivel = "voz fuerte  ";
  else                           nivel = "MUY FUERTE! ";

  Serial.printf("  %5d  | %5d | %s | ", amp, picoMax, nivel);
  imprimirBarra(suavizado, ESCALA_MAX, ANCHO_BARRA);
  Serial.println();

  delay(100);
}
