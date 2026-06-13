#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h> // <--- Librería cambiada para SH1106
#include <driver/i2s.h>

// ==========================================
// DEFINICIÓN DE PINES (ESP32-C6 Supermini)
// ==========================================
// Pines Pantalla OLED (I2C)
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7

// Pines Micrófono INMP441 (I2S)
#define I2S_SCK_PIN 4
#define I2S_WS_PIN  1
#define I2S_SD_PIN  3
#define I2S_PORT    I2S_NUM_0

// Pines Extras
#define TOUCH_PIN   2
#define BUZZER_PIN  19

// ==========================================
// CONFIGURACIÓN OLED SH1106
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 

// Inicialización específica para SH1106
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Tamaño del buffer de lectura de audio
#define BUFFER_SIZE 256
int32_t sampleBuffer[BUFFER_SIZE];

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // 1. Configurar Pines Extras
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // 2. Inicializar I2C para OLED con los pines específicos del C6
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // Iniciar la pantalla SH1106 (Dirección I2C comúnmente 0x3C)
  if(!display.begin(0x3C, true)) { 
    Serial.println(F("Fallo al inicializar SH1106 OLED"));
    for(;;); // Bucle infinito si falla
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE); // <--- Constante de color actualizada
  display.setCursor(0, 20);
  display.println("Iniciando INMP441...");
  display.display();
  delay(1000);

  // 3. Configurar I2S para el INMP441
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 8000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // El INMP441 usa 32 bits
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // L/R a GND (canal izquierdo)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };

  // Instalar driver I2S
  esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Fallo al instalar driver I2S: %d\n", err);
    while (true);
  }
  
  // Asignar pines I2S
  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Fallo al configurar pines I2S: %d\n", err);
    while (true);
  }

  Serial.println("I2S Micrófono inicializado.");
}

void loop() {
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, &sampleBuffer, sizeof(sampleBuffer), &bytesIn, portMAX_DELAY);
  
  if (result == ESP_OK) {
    int samplesRead = bytesIn / 4;
    int32_t maxAmplitude = 0;
    int32_t rawValue = 0; // Variable para ver qué envía realmente el micrófono
    
    // Variable estática para recordar el nivel de DC
    static int32_t dc_offset = 0; 
    
    for (int i = 0; i < samplesRead; i++) {
      // Usaremos el desplazamiento de 14 bits que te funcionó en el primer intento
      int32_t sample = sampleBuffer[i] >> 14; 
      
      // Guardar una muestra cruda para imprimirla y diagnosticar
      rawValue = sample; 
      
      // Filtro EMA
      dc_offset = ((dc_offset * 63) + sample) >> 6;
      
      // Señal limpia
      int32_t val = abs(sample - dc_offset);
      
      if (val > maxAmplitude) {
      maxAmplitude = val;
    }
  } // <-- Fin del bucle for

  // ==========================================
  // COMPUERTA DE RUIDO (NOISE GATE)
  // ==========================================
  int threshold = 1200; // Ajusta este valor según el ruido de tu habitación
  
  if (maxAmplitude < threshold) {
    maxAmplitude = 0; // Silencio absoluto
  } else {
    // Restamos el umbral para que la barra gráfica empiece suavemente desde cero
    maxAmplitude = maxAmplitude - threshold; 
  }

    // Leer el sensor táctil
    bool isTouched = digitalRead(TOUCH_PIN);
    digitalWrite(BUZZER_PIN, isTouched ? HIGH : LOW);

    // ==========================================
    // IMPRESIÓN EN SERIE PARA DIAGNÓSTICO
    // ==========================================
    Serial.print("Touch:");
    Serial.print(isTouched ? 100 : 0); 
    Serial.print(",RAW:");
    Serial.print(rawValue); // <--- ESTO ES LA CLAVE
    Serial.print(",Filtrado:");
    Serial.println(maxAmplitude);

    // ==========================================
    // OLED SH1106
    // ==========================================
    display.clearDisplay();
    
    display.setCursor(0, 0);
    display.print("Mic: INMP441");

    display.setCursor(80, 0);
    if (isTouched) {
      display.print("[TOQUE]");
    }

    display.setCursor(0, 16);
    display.print("Nivel: ");
    display.print(maxAmplitude);

    // Barra visual 
    int barWidth = map(maxAmplitude, 4000, 14000, 0, SCREEN_WIDTH);
    if (barWidth > SCREEN_WIDTH) barWidth = SCREEN_WIDTH; 
    
    display.drawRect(0, 30, SCREEN_WIDTH, 12, SH110X_WHITE);      
    display.fillRect(0, 30, barWidth, 12, SH110X_WHITE);          

    display.display();
  }
  
  delay(20); 
}