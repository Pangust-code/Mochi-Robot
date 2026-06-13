# Prueba 09 — Nivel de sonido del micrófono

Verifica que el micrófono INMP441 capta audio y produce lecturas de amplitud variables en tiempo real.

---

## ¿Qué vas a aprender?

El INMP441 convierte variaciones de presión del aire (sonido) en números de 16 bits. El sketch captura paquetes de 256 muestras a 16,000 Hz y calcula la **amplitud** de cada paquete: el valor máximo menos el valor mínimo dentro del grupo.

```
amplitud = max(muestras) - min(muestras)
```

Esto filtra automáticamente el desplazamiento de corriente continua (DC offset) y mide la energía bruta de la señal. No procesamos frecuencias — no hay FFT. Solo energía. Es suficiente para distinguir silencio, voz y aplausos.

Para evitar que la barra visual salte erráticamente, se aplica un **filtro de suavizado exponencial**:

```cpp
suavizado = (suavizado * 5 + amplitud * 3) / 8
```

Cuanto mayor sea el primer coeficiente (5), más peso tiene el historial — la respuesta es más lenta pero estable. Cuanto mayor sea el segundo (3), más reacciona al valor nuevo.

---

## Hardware

| Pin INMP441 | GPIO ESP32-C6 | Función I2S |
|------------|--------------|------------|
| VDD | 3.3V | Alimentación |
| GND | GND | Tierra |
| L/R | GND | Canal izquierdo |
| SCK | GPIO 4 | Reloj de bits (BCLK) |
| WS | GPIO 5 | Word Select (LRCK) |
| SD | GPIO 3 | Datos seriales |

> El pin L/R **debe** estar en GND. Si queda flotando, el canal de datos no está definido y los valores serán incorrectos o cero.

---

## Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/esp32c6/pruebas/09_prueba_sonido_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/esp32c6/pruebas/09_prueba_sonido_microfono
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

---

## Resultado en tiempo real

El Monitor Serie muestra un dashboard que se actualiza 10 veces por segundo:

```
========================================
  PRUEBA DE SONIDO — INMP441
========================================
  SCK=GPIO4  WS=GPIO5  SD=GPIO3
  L/R → GND

  Guía de valores:
    Silencio      :     0 –  2000
    Ruido / voz   :  2000 – 10000
    Voz fuerte    : 10000 – 25000
    Muy fuerte    : > 25000

  Amp    | Pico  | Nivel        | Barra
  -------|-------|--------------|------------------------------------------
     850  |   850 | silencio     | |###                                     |
    5200  |  5200 | ruido / voz  | |#############                           |
   14300  | 14300 | voz fuerte   | |############################            |
   31000  | 31000 | MUY FUERTE!  | |#####################################!!!|
```

---

## Tabla de registro (llénala)

| Condición | Amplitud observada | Ejemplo típico |
|-----------|-------------------|----------------|
| Silencio absoluto | ____________ | `200 – 800` |
| Voz normal (30 cm) | ____________ | `3 000 – 8 000` |
| Aplauso seco cerca | ____________ | `15 000 – 30 000` |

> Tu umbral de "sonido fuerte" para el firmware debe estar entre voz normal y aplauso. Anota el valor que eliges: ______

---

## Diagnóstico

| Lo que ves | Causa probable | Solución |
|-----------|---------------|----------|
| Valores siempre en cero | Pin L/R flotante o SD desconectado | Conecta L/R → GND; verifica GPIO 3 |
| Valores siempre al máximo | Cortocircuito en SD o interferencia | Aleja el micrófono de la fuente de 5V |
| Ruido constante sin sonido | Interferencia electromagnética | Aleja el cable SD de los cables de alimentación |
| `ERROR al instalar driver I2S` | Conflicto de pines | Verifica que ningún otro periférico usa GPIO 3, 4 o 5 |

---

## Mini-reto

El coeficiente de suavizado controla qué tan rápido reacciona la barra. Busca esta línea en el sketch:

```cpp
suavizado = (suavizado * 5 + amp * 3) / 8;
```

Experimenta con estas variaciones y observa la barra:

| Variante | Código | Comportamiento esperado |
|----------|--------|------------------------|
| Muy reactivo | `(suavizado * 2 + amp * 6) / 8` | Salta rápido, difícil de leer |
| Muy suave | `(suavizado * 7 + amp * 1) / 8` | Lento, casi no reacciona |
| Equilibrado (original) | `(suavizado * 5 + amp * 3) / 8` | Respuesta natural |

¿Cuál variante sería más útil para un **detector de aplausos** (necesita reaccionar rápido) versus un **indicador de ruido ambiental** (necesita ser estable)?

---

## Retos disponibles

Con los umbrales que registraste, puedes ir a:

- **[Reto 4 — Contador de aplausos](../retos/reto-4-aplausos/)** ⭐⭐⭐ Difícil
  Crea un Modo 5 en Mochi que cuente aplausos usando exactamente los valores que mediste. Cada aplauso sube el contador en pantalla.

- **[Reto 7 — Espejo emocional](../retos/reto-7-espejo-emocional/)** ⭐⭐ Medio
  Haz que los ojos de Mochi cambien en tiempo real según el nivel de ruido ambiental. Silencio = dormido, voz = feliz, grito = sorprendido.

---

**Siguiente prueba →** [10_transcripcion/](../10_transcripcion/)
