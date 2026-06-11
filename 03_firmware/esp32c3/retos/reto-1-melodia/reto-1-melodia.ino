/*
  reto-1-melodia.ino
  Reto 1 — Tu propia melodía ⭐ Fácil

  Este sketch es un banco de pruebas para tu melodía.
  Modifica el array mel[] y cárgalo al ESP32 para escuchar
  tu composición. Cuando suene bien, aplica los mismos
  cambios a la función tetrisTheme() en mochi_unified_5.ino

  Hardware:
    Buzzer pasivo → GPIO 10
    Sensor TTP223 → GPIO 2

  Uso:
    Toca el sensor → reproduce la melodía una vez
*/

#define BUZZER_PIN 10
#define TOUCH_PIN   2

// ── Frecuencias de notas ──────────────────────────────────────────────
#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494
#define C5  523
#define D5  587
#define E5  659
#define F5  698
#define G5  784
#define A5  880
#define B5  988
#define SIL   0   // silencio

// ─────────────────────────────────────────────────────────────────────
//  TODO: Reemplaza estas notas con tu propia melodía.
//
//  Formato: { nota, duración_ms, nota, duración_ms, ... }
//  Un "0" como nota = silencio.
//
//  Ejemplo — primeras notas de "Cumpleaños Feliz":
//    { C4,200, C4,200, D4,400, C4,400, F4,400, E4,800,
//      C4,200, C4,200, D4,400, C4,400, G4,400, F4,800 }
//
//  Ejemplo — primeras notas de "Mario Bros":
//    { E5,150, E5,150, SIL,150, E5,150, SIL,150,
//      C5,150, E5,150, SIL,150, G5,300, SIL,300, G4,300 }
// ─────────────────────────────────────────────────────────────────────
int mel[] = {
  // ↓ Escribe tu melodía aquí ↓
  C4, 200,
  E4, 200,
  G4, 200,
  C5, 400,
  SIL, 200,
  C5, 400,
  G4, 200,
  E4, 200,
  C4, 600
  // ↑ Escribe tu melodía aquí ↑
};

// ─────────────────────────────────────────────────────────────────────
//  No necesitas modificar nada debajo de esta línea.
// ─────────────────────────────────────────────────────────────────────

void reproducir() {
  int n = sizeof(mel) / sizeof(mel[0]) / 2;
  for (int i = 0; i < n; i++) {
    int freq = mel[i * 2];
    int dur  = mel[i * 2 + 1];
    if (freq == SIL) {
      noTone(BUZZER_PIN);
    } else {
      tone(BUZZER_PIN, freq, (int)(dur * 0.9f));
    }
    delay(dur);
  }
  noTone(BUZZER_PIN);
}

bool prevTouch = false;

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);
  Serial.println("Reto 1 — Melodía personalizada");
  Serial.println("Toca el sensor para reproducir tu melodía.");
}

void loop() {
  bool touched = digitalRead(TOUCH_PIN);
  if (touched && !prevTouch) {
    Serial.println("▶ Reproduciendo...");
    reproducir();
    Serial.println("✓ Listo. Toca de nuevo para repetir.");
  }
  prevTouch = touched;
  delay(10);
}
