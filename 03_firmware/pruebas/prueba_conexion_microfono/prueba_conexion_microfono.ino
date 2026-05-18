/*
  prueba_conexion_microfono.ino
  Hardware: ESP32-C6 Supermini

  Verifica que cada cable del micrófono INMP441 esté físicamente conectado
  antes de encender el robot completo.

  CÓMO USAR:
  1. NO conectes el micrófono todavía (o desconéctalo si ya está puesto)
  2. Abre el Monitor Serial a 115200 baud
  3. Con un cable dupont suelto, toca momentáneamente cada GPIO a GND:
       GPIO 3 → SD   (línea de datos)
       GPIO 4 → SCK  (reloj de bits)
       GPIO 5 → WS   (selección de canal)
  4. Cada pin debe cambiar de HIGH a LOW al tocarlo con GND
  5. Si un pin NO cambia, ese cable o conexión tiene un problema

  LÓGICA:
  Los pines se configuran como INPUT_PULLUP → normalmente están en HIGH.
  Al conectarlos a GND con un cable, se fuerzan a LOW.
  Si el pin no baja a LOW, la señal eléctrica no está llegando al ESP32.
*/

#define PIN_SD   3   // INMP441 — datos de audio
#define PIN_SCK  4   // INMP441 — reloj de bits (BCK)
#define PIN_WS   5   // INMP441 — selección de canal (LRCK)

bool prevSD  = HIGH;
bool prevSCK = HIGH;
bool prevWS  = HIGH;

void imprimirEstado(bool sd, bool sck, bool ws) {
  Serial.println("----------------------------------------");
  Serial.printf("  SD  (datos)  GPIO %d : %s\n", PIN_SD,
    sd  ? "HIGH  — no conectado a GND"
        : "LOW  ✓ cable responde OK");
  Serial.printf("  SCK (reloj)  GPIO %d : %s\n", PIN_SCK,
    sck ? "HIGH  — no conectado a GND"
        : "LOW  ✓ cable responde OK");
  Serial.printf("  WS  (frame)  GPIO %d : %s\n", PIN_WS,
    ws  ? "HIGH  — no conectado a GND"
        : "LOW  ✓ cable responde OK");

  if (!sd && !sck && !ws) {
    Serial.println();
    Serial.println("  >>> TODOS LOS PINES RESPONDIERON ✓");
    Serial.println("      Conexiones físicas verificadas.");
    Serial.println("      Ya puedes conectar el micrófono.");
  }
  Serial.println("----------------------------------------");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_SD,  INPUT_PULLUP);
  pinMode(PIN_SCK, INPUT_PULLUP);
  pinMode(PIN_WS,  INPUT_PULLUP);

  Serial.println();
  Serial.println("========================================");
  Serial.println("  PRUEBA DE CONEXIONES — INMP441");
  Serial.println("========================================");
  Serial.println("  Toca cada pin a GND con un dupont.");
  Serial.println("  Debe aparecer: LOW ✓");
  Serial.println("========================================");
  Serial.println();
  imprimirEstado(HIGH, HIGH, HIGH);
}

void loop() {
  bool sd  = digitalRead(PIN_SD);
  bool sck = digitalRead(PIN_SCK);
  bool ws  = digitalRead(PIN_WS);

  // Solo imprime cuando hay un cambio de estado
  if (sd != prevSD || sck != prevSCK || ws != prevWS) {
    imprimirEstado(sd, sck, ws);
    prevSD  = sd;
    prevSCK = sck;
    prevWS  = ws;
  }

  delay(50);
}
