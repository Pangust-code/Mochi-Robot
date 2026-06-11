int alarmaHora = 7;
int alarmaMin = 30;
bool alarmaActiva = false;      // ¿La alarma está encendida?
bool alarmaSonando = false;     // ¿Está sonando en este momento?
bool alarmaDisparadaHoy = false; // Flag para evitar que suene todo el minuto

void verificarAlarma(struct tm *ti) {
  // Comprobar si la hora actual coincide con la alarma
  if (alarmaActiva && !alarmaDisparadaHoy) {
    if (ti->tm_hour == alarmaHora && ti->tm_min == alarmaMin) {
      alarmaSonando = true;
      alarmaDisparadaHoy = true; 
    }
  }

  // Resetear el flag al cambiar de minuto para que funcione al día siguiente
  if (ti->tm_min != alarmaMin) {
    alarmaDisparadaHoy = false;
  }
}

void dispararAlarmaVisual() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.print("ALARMA!");
  
  display.setTextSize(1);
  display.setCursor(25, 40);
  display.print("Tap: Parar");
  display.display();
  
  // Patrón de sonido (beep-beep)
  tone(BUZZER_PIN, 1000, 200);
  delay(300);
  tone(BUZZER_PIN, 800, 200);
}