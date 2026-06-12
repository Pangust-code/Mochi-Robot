/*
  reto-4-aplausos.ino
  Reto 4 — Contador de aplausos ⭐⭐⭐ Difícil

  Sketch standalone para probar y calibrar la detección de aplausos
  ANTES de integrarla como Modo 5 en mochi_unified_5.ino.

  Este sketch no usa pantalla OLED — muestra el contador
  en el monitor serial y con beeps de confirmación.

  Hardware:
    Micrófono INMP441 → I2S (SCK=GPIO4, WS=GPIO5, SD=GPIO3, L/R→GND)
    Buzzer pasivo     → GPIO 19
    Sensor TTP223     → GPIO 2  (para reiniciar el contador)

  Uso:
    1. Sube este sketch
    2. Abre el monitor serial (115200 baud)
    3. Aplaude frente al micrófono
    4. Observa que el contador sube
    5. 1 tap en el sensor → reinicia a 0
    6. Ajusta CLAP_THRESHOLD si hay falsas detecciones
*/

#include <driver/i2s.h>

// ── Pines ────────────────────────────────────────────────────────────
#define BUZZER_PIN   19
#define TOUCH_PIN     2
#define I2S_SCK       4
#define I2S_WS        1
#define I2S_SD        3
#define I2S_PORT  I2S_NUM_0

// ── Configuración — ajusta si hay problemas ───────────────────────────
// Si cuenta aplausos donde no hay → baja CLAP_THRESHOLD
// Si no detecta aplausos → sube CLAP_THRESHOLD
#define CLAP_THRESHOLD   20000   // amplitud mínima para considerar un aplauso
#define CLAP_DEBOUNCE_MS    400  // ms mínimos entre dos aplausos (evita doble conteo)

// ─────────────────────────────────────────────────────────────────────
//  No necesitas modificar nada debajo de esta línea para el reto base.
//  Una vez que la detección funcione, integra la lógica en mochi_unified_5.ino
// ─────────────────────────────────────────────────────────────────────

int clapCount = 0;
unsigned long lastClapTime = 0;
bool prevTouch = false;

void initMicrophone() {
  i2s_config_t cfg = {
    .mode              = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
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
    .bck_io_num   = I2S_SCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };
  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
}

int leerAmplitud() {
  int16_t buf[256];
  size_t  br = 0;
  i2s_read(I2S_PORT, buf, sizeof(buf), &br, 10);
  if (br == 0) return 0;
  int n = br / 2, mx = 0;
  for (int i = 0; i < n; i++) {
    int v = abs((int)buf[i]);
    if (v > mx) mx = v;
  }
  return mx;
}

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);
  initMicrophone();

  Serial.println();
  Serial.println("=== CONTADOR DE APLAUSOS ===");
  Serial.printf("Umbral de deteccion: %d\n", CLAP_THRESHOLD);
  Serial.printf("Debounce: %d ms\n", CLAP_DEBOUNCE_MS);
  Serial.println("Aplaude frente al microfono.");
  Serial.println("1 tap en el sensor = reiniciar contador.");
  Serial.println("============================");
}

void loop() {
  unsigned long now = millis();

  // ── Leer amplitud ──────────────────────────────────────────────────
  int amp = leerAmplitud();

  // ── Detectar aplauso ───────────────────────────────────────────────
  if (amp > CLAP_THRESHOLD && (now - lastClapTime) > CLAP_DEBOUNCE_MS) {
    clapCount++;
    lastClapTime = now;
    tone(BUZZER_PIN, 1200, 50);   // beep corto de confirmación
    Serial.printf("👏 Aplauso detectado! Contador: %d  (amp=%d)\n", clapCount, amp);
  }

  // ── Tap = reiniciar contador ───────────────────────────────────────
  bool touched = digitalRead(TOUCH_PIN);
  if (touched && !prevTouch) {
    clapCount = 0;
    tone(BUZZER_PIN, 400, 200);
    Serial.println("↺ Contador reiniciado");
  }
  prevTouch = touched;

  // ── Debug: muestra amplitud cada segundo ──────────────────────────
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 1000) {
    Serial.printf("   amplitud actual: %d (umbral: %d)  |  aplausos: %d\n",
                  amp, CLAP_THRESHOLD, clapCount);
    lastPrint = now;
  }

  delay(5);
}
