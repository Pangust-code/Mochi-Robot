/* Fragmento para mochi_unified_5.ino
  Implementación del Reto 7: Espejo Emocional
*/

// Definir umbrales basados en tus pruebas con prueba_sonido_microfono.ino
#define SILENCIO_LIMITE   2000
#define RUIDO_MEDIO       10000
#define RUIDO_FUERTE      25000

// Variable para recordar el mood previo antes de reaccionar a un sonido
int moodPrevio = MOOD_NORMAL; 
unsigned long tiempoReaccion = 0;

void runMascotaMode() {
  int amp = leerAmplitud(); // Usando tu función de lectura I2S
  unsigned long ahora = millis();

  // 1. Lógica de cambio de Mood basado en sonido
  int moodObjetivo = currentMood;

  if (amp > RUIDO_FUERTE) {
    moodObjetivo = MOOD_SURPRISED; // Sorpresa ante ruido fuerte
    tiempoReaccion = ahora;
  } 
  else if (amp > RUIDO_MEDIO) {
    moodObjetivo = MOOD_EXCITED;   // Emoción con ruido medio
    tiempoReaccion = ahora;
  }
  else if (amp > SILENCIO_LIMITE) {
    moodObjetivo = MOOD_NORMAL;    // Normal con ruido bajo/voz
    tiempoReaccion = ahora;
  }
  else {
    // Si hay silencio por más de 5 segundos, se duerme
    if (ahora - tiempoReaccion > 5000) {
      moodObjetivo = MOOD_SLEEPY;
    }
  }

  // 2. Aplicar cambio si es diferente al actual
  if (moodObjetivo != currentMood) {
    // Guardamos el mood antes de cambiar (si no estábamos durmiendo)
    if (currentMood != MOOD_SLEEPY) {
      moodPrevio = currentMood;
    }
    
    setMood(moodObjetivo);
    Serial.printf("Sonido: %d -> Mood cambiado a: %d\n", amp, moodObjetivo);
  }

  // 3. Opcional: Regresar al mood previo si ya pasó el susto (ej: 2 segundos)
  if (ahora - tiempoReaccion > 2000 && currentMood != MOOD_SLEEPY) {
    if (currentMood != moodPrevio) {
      setMood(moodPrevio);
    }
  }
}