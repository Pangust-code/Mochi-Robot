/* Lógica para el Reto 8: Alarma del Reloj
   Integra estas variables globales y la función de comprobación
*/

// Variables globales para la alarma
int alarmaHora = 7;
int alarmaMin = 30;
bool alarmaActiva = false;
bool alarmaDisparadaHoy = false;
bool sonandoAlarma = false;

void verificarAlarma(struct tm *ti) {
  // Comprobar si coincide la hora
  if (alarmaActiva && !alarmaDisparadaHoy) {
    if (ti->tm_hour == alarmaHora && ti->tm_min == alarmaMin) {
      sonandoAlarma = true;
      alarmaDisparadaHoy = true; // Evita que suene cada segundo durante el minuto
    }
  }

  // Resetear el flag al cambiar de minuto
  if (ti->tm_min != alarmaMin) {
    alarmaDisparadaHoy = false;
  }
}

void runAlarmaScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(20, 10);
  display.print("ALARMA!");
  display.setTextSize(1);
  display.setCursor(25, 40);
  display.print("Tap para parar");
  display.display();
  
  // Sonido de alarma
  tone(BUZZER_PIN, 1000, 200);
  delay(300);
  tone(BUZZER_PIN, 800, 200);
  delay(300);
}