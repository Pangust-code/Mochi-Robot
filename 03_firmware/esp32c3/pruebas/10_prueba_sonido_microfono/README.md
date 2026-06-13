# Prueba 10 — Nivel de sonido con micrófono INMP441

Verifica que el micrófono INMP441 recibe audio por I2S y que el nivel de amplitud se refleja correctamente en la barra de la OLED.

---

## ¿Qué vas a aprender?

El **INMP441** es un micrófono digital que usa el protocolo **I2S** — un bus de audio de 3 cables: SCK (reloj), WS (selección de canal: izquierdo/derecho) y SD (datos de audio serializados).

El ESP32-C3 lee bloques de 256 muestras de 32 bits. Con esas muestras calcula la **amplitud pico a pico**:

```
amplitud = max(muestras) - min(muestras)
```

Este cálculo elimina el offset DC (el "cero" del micrófono que nunca es exactamente cero) y te da la variación real de la señal. En silencio: ≈ 200–800. Con voz normal: 10 000–50 000. Con sonido fuerte: > 80 000.

### Suavizado exponencial

En vez de mostrar el valor crudo (que parpadea mucho), se aplica:

```
suavizado = (suavizado × 5 + amplitud × 3) / 8
```

Los coeficientes `5` y `3` controlan qué tan "perezosa" es la respuesta:
- `5` de peso al pasado → inercia alta, barra suave pero lenta
- `3` de peso al presente → responde rápido pero con más ruido

La suma debe ser `8` para que el promedio sea correcto (ambos divididos entre 8).

---

## Hardware

| Pin INMP441 | GPIO ESP32-C3 Super Mini | Función |
|------------|--------------------------|---------|
| VCC | 3.3V | Alimentación |
| GND | GND | Tierra |
| SCK | GPIO 4 | I2S — reloj de bit |
| WS | GPIO 1 | I2S — selección izquierda/derecha |
| SD | GPIO 3 | I2S — datos seriales |
| L/R | GND | Selecciona canal izquierdo |

> El pin **WS está en GPIO 1** (no GPIO 5 como en el ESP32-C6). Verifica bien este cable.

También se usa la OLED (SDA → GPIO 8, SCL → GPIO 9) para la barra visual.

---

## Dependencias

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
```

I2S es nativo del ESP32-C3 — no requiere librería adicional.

---

## Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/10_prueba_sonido_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/10_prueba_sonido_microfono
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

---

## Resultado esperado

La OLED muestra una **barra de nivel** que crece con el volumen:

```
[████████░░░░░░░░░░░░] 42%   ← voz normal
[█░░░░░░░░░░░░░░░░░░░]  8%   ← silencio
[████████████████████] 98%   ← palmada fuerte
```

En el Monitor Serie:

```
[MIC] Init OK  SCK=4 WS=1 SD=3
Silencio:       amp=320    suav=310
Voz normal:     amp=28450  suav=19230
Palmada:        amp=95000  suav=61000
```

---

## Diagnóstico

| Lo que ves | Causa probable | Solución |
|-----------|---------------|----------|
| Barra siempre en 0 | WS o SCK desconectado | Verifica GPIO 1 (WS) y GPIO 4 (SCK) |
| Barra siempre al máximo | SD desconectado (ruido en línea flotante) | Verifica GPIO 3 (SD) |
| Barra no responde al hablar pero sí a golpes | INMP441 mal orientado | El micrófono debe apuntar hacia afuera |
| `I2S init failed` en serie | Pin WS o SCK incorrecto | Confirma que WS=1, SCK=4, SD=3 en el sketch |

---

## Mini-reto

El suavizado está definido en el sketch. Busca la línea que contiene `* 5` y `* 3`.

Prueba estos dos extremos y describe qué ves en la barra:

**Caso A — Reacción rápida (sin inercia):**
```cpp
suavizado = (suavizado * 2 + amplitud * 6) / 8;
```

**Caso B — Inercia máxima (muy suave):**
```cpp
suavizado = (suavizado * 7 + amplitud * 1) / 8;
```

¿Cuál es más útil para detectar aplausos? ¿Cuál para medir el nivel general de ruido de una sala?

> Pista: una aplicación de "detectar palmada" necesita respuesta instantánea. Un medidor ambiental necesita estabilidad.

**Retos relacionados:**
- [Reto 4 — Contador de aplausos](../retos/reto-4-aplausos/) — umbral de pico + debounce de audio
- [Reto 7 — Espejo emocional](../retos/reto-7-espejo-emocional/) — modos según nivel de ruido

---

**Siguiente prueba →** [11_transcripcion/](../11_transcripcion/)
