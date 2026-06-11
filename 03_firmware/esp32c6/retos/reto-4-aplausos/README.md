# Reto 4 — Contador de aplausos ⭐⭐⭐ Difícil

Crea un Modo 5 que cuente aplausos usando el micrófono y muestre el contador en pantalla.

## Archivos

| Archivo | Contenido |
|---------|-----------|
| `reto-4-aplausos.ino` | Sketch standalone para probar la detección sin OLED |

---

## Fase 1 — Probar la detección de aplausos

Antes de integrar el modo en el firmware completo, verifica que la detección funciona con este sketch.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 seccion-4-retos/reto-4-aplausos
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 seccion-4-retos/reto-4-aplausos
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

Aplaude frente al micrófono. Debes ver en el monitor serial:
```
Aplauso detectado! Contador: 1  (amp=28431)
Aplauso detectado! Contador: 2  (amp=31200)
```

### Calibración

Si el contador sube sin que aplaudas → baja `CLAP_THRESHOLD`
Si no detecta aplausos → sube `CLAP_THRESHOLD`

El sketch imprime `amplitud actual: XXXX` cada segundo — usa ese valor como referencia.

---

## Fase 2 — Integrar como Modo 5 en el firmware

Una vez que la detección funciona, integra la lógica en `03_firmware/mochi_unified_5/mochi_unified_5.ino`.

### Cambio 1 — Agregar el modo

**Busca:** `#define NUM_MODES 5`
**Cambia a:** `#define NUM_MODES 6`

### Cambio 2 — Variables de estado

**Busca el bloque `// Mic test`** y agrega debajo:
```cpp
// Contador de aplausos (Modo 5)
static int   clapCount       = 0;
static unsigned long lastClapAt = 0;
#define CLAP_THRESHOLD  20000
#define CLAP_DEBOUNCE_MS  400
```

### Cambio 3 — Lógica del modo en loop()

**Busca `case 4:` en el `switch(appMode)`** y agrega después:
```cpp
case 5: {
  // Leer amplitud
  int amp = leerAmplitud();
  unsigned long now = millis();

  // Detectar aplauso
  if (amp > CLAP_THRESHOLD && (now - lastClapAt) > CLAP_DEBOUNCE_MS) {
    clapCount++;
    lastClapAt = now;
    tone(BUZZER_PIN, 1200, 50);
  }

  // 1 tap reinicia el contador
  if (ev_shortTap) clapCount = 0;

  // Mostrar en pantalla
  activarAdafruit();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(30, 5);
  display.print("APLAUSOS");
  display.setTextSize(4);
  display.setCursor(40, 22);
  display.print(clapCount);
  display.setTextSize(1);
  display.setCursor(10, 56);
  display.print("tap = reiniciar");
  display.display();
  break;
}
```

---

## Resultado esperado

- Entra al **Modo 5** con hold (avanza hasta el nuevo modo).
- La pantalla muestra un número grande.
- Cada aplauso sube el contador y produce un beep.
- Un tap reinicia el contador a 0.
