# Prueba 03 — Buzzer pasivo

Verifica que el buzzer puede reproducir tonos de frecuencia variable, confirmando que es **pasivo** y no activo.

---

## ¿Qué vas a aprender?

Existen dos tipos de buzzer y son fácilmente confundibles porque lucen idénticos:

| Tipo | Cómo funciona | ¿Sirve para melodías? |
|------|--------------|----------------------|
| **Pasivo** | Solo un parlante — el ESP32 genera la señal | Sí |
| **Activo** | Tiene oscilador incorporado — emite siempre el mismo tono | No |

La función `tone(pin, frecuencia)` genera una onda cuadrada en el pin: a 262 Hz suena Do4, a 440 Hz suena La4. El buzzer pasivo convierte esa señal eléctrica en vibración mecánica de aire. El activo ignora la frecuencia y zumba siempre igual.

La prueba de sweep al inicio es exactamente para detectar esto: si el tono sube y baja, es pasivo. Si suena constante, cámbialo.

---

## Hardware

| Componente | Pin | GPIO ESP32-C6 | Función |
|-----------|-----|--------------|---------|
| Buzzer pasivo | SIG | GPIO 19 | Salida PWM |
| Buzzer pasivo | GND | GND | Tierra |
| OLED SH1106 | SDA | GPIO 6 | I2C datos |
| OLED SH1106 | SCL | GPIO 7 | I2C reloj |
| TTP223 | OUT | GPIO 2 | Entrada digital |

---

## Dependencias

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
```

---

## Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/esp32c6/pruebas/03_prueba_buzzer
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/esp32c6/pruebas/03_prueba_buzzer
```

---

## Secuencia de pruebas

Al encender, el sketch ejecuta tres tests automáticos:

**Test 1 — Sweep de frecuencias (200 Hz → 3000 Hz → 200 Hz)**

Escucha atentamente. Si el tono sube y baja de forma gradual, el buzzer es pasivo. Si escuchas un zumbido constante que no cambia de tono, es activo — debes reemplazarlo.

**Test 2 — Escala musical (Do Re Mi Fa Sol La Si Do)**

```
DO  262 Hz
RE  294 Hz
MI  330 Hz
FA  349 Hz
SOL 392 Hz
LA  440 Hz
SI  494 Hz
DO  523 Hz
```

**Test 3 — Tonos del firmware Mochi**

Los sonidos exactos que escucharás en el firmware final:
- Tap de ánimo: 800 Hz, 60 ms
- Cambio de modo: 1000 Hz, 80 ms
- Pomodoro listo: 880 Hz + 1046 Hz

**Salida esperada en Monitor Serie (115200 baudios):**

```
========================================
  PRUEBA BUZZER — pasivo GPIO 19
========================================
[TEST 1] Sweep 200Hz → 3000Hz → 200Hz   Sweep completo.
[TEST 2] Escala musical  DO RE MI FA SOL LA SI DO
[TEST 3] Tonos del firmware  OK

Tests automáticos completos.
Modo interactivo: TAP x1 → tono tap | TAP x3+ → Tetris | HOLD → cambio de modo
```

**Modo interactivo** (después de los tests):
- 1 tap → tono corto de feedback
- 3 taps → tema de Tetris completo (cada vez un poco más rápido)
- Hold → tono de cambio de modo

---

## Diagnóstico

| Lo que ves | Causa probable | Solución |
|-----------|---------------|----------|
| Zumbido constante en el sweep | Buzzer activo | Reemplaza por buzzer pasivo |
| Sin sonido en ningún test | Pin desconectado o buzzer roto | Verifica GPIO 19 y el cable GND |
| Sonido muy bajo | Buzzer de baja impedancia | Normal — es más audible cerca |

---

## Mini-reto

La escala musical está definida como un array de pares `{frecuencia, duración_ms}`. Búscala en el sketch (busca `262` o `"DO"`).

Agrega estas tres notas al **final** del array:

```cpp
{587, 300},  // RE5
{659, 300},  // MI5
{698, 300},  // FA5
```

¿Reconoces la melodía que forman Do-Re-Mi-Fa del principio más estas tres? Es el inicio de "Do Re Mi" de The Sound of Music. ¿Puedes completar la frase musical completa?

**Tabla de frecuencias útiles:**

| Nota | Hz | Nota | Hz |
|------|-----|------|-----|
| Do4 | 262 | Do5 | 523 |
| Re4 | 294 | Re5 | 587 |
| Mi4 | 330 | Mi5 | 659 |
| Fa4 | 349 | Fa5 | 698 |
| Sol4 | 392 | Sol5 | 784 |
| La4 | 440 | La5 | 880 |
| Si4 | 494 | Si5 | 988 |
| Silencio | 0 | — | — |

**Reto disponible:** [Reto 1 — Nueva melodía](../retos/reto-1-melodia/) — un sketch standalone con el array de notas vacío para que compongas sin tocar el firmware principal.

---

**Siguiente prueba →** [05_prueba_conexion_microfono/](../05_prueba_conexion_microfono/)
