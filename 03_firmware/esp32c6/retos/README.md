# Sección 4 — Retos opcionales

> **Prerrequisito:** [mochi_unified_5/](../mochi_unified_5/) — haber leído las funciones del robot.

Esta carpeta contiene los retos de personalización del taller. No son ejercicios con respuesta única — son puntos de partida abiertos. Cada uno pide que entiendas una parte del sistema y la modifiques a tu criterio.

Los retos **5 y 6 son Easter Eggs** listos para jugar. Los demás requieren modificar el firmware.

---

## Contenido de esta carpeta

```
retos/
├── reto-1-melodia/
│   ├── README.md              ← instrucciones del reto
│   └── reto-1-melodia.ino     ← sketch standalone con TODO para tu melodía
├── reto-2-mood-wink/
│   └── README.md              ← snippets exactos para agregar el nuevo mood
├── reto-3-gif-propio/
│   └── README.md              ← guía para convertir y subir tu GIF
├── reto-4-aplausos/
│   ├── README.md              ← instrucciones en dos fases
│   └── reto-4-aplausos.ino   ← sketch standalone para calibrar la detección
├── reto-5-chromedino/         ← Easter Egg: Chrome Dino en OLED
│   ├── chromeDino.ino
│   ├── t-rex-duino.h          ← variables de física y dificultad aquí
│   └── TRexCore.h
├── reto-6-flappybird/         ← Easter Egg: Flappy Bird en OLED
│   └── flappyBird.ino         ← variables de física al inicio del archivo
├── reto-7-espejo-emocional/   ← Reto nuevo: moods según el sonido ambiental
├── reto-8-alarma-reloj/      ← Reto nuevo: alarma visual y sonora para el reloj
└── reto-9-bienvenida/        ← Reto nuevo: animación de arranque
```

---

## Índice de retos

| # | Reto | Nivel | Qué aprenderás |
|---|------|-------|---------------|
| 5 | [Chrome Dino](#reto-5--chrome-dino-en-oled-) | Easter Egg | Constantes de física y game loop en OLED |
| 6 | [Flappy Bird](#reto-6--flappy-bird-en-oled-) | Easter Egg | Variables de gravedad, velocidad y gap |
| 1 | [Nueva melodía](reto-1-melodia/) | ⭐ Fácil | Arrays, frecuencias Hz, duración de notas |
| 9 | [Pantalla de inicio animada](reto-9-bienvenida/) | ⭐ Fácil | `setup()`, funciones de dibujo, `delay()` no bloqueante |
| 2 | [Nuevo mood: guiño](reto-2-mood-wink/) | ⭐⭐ Medio | Constantes, máscara de párpado, `switch/case` |
| 3 | [GIF personalizado](reto-3-gif-propio/) | ⭐⭐ Medio | LittleFS, conversión de formato, filesystem flash |
| 7 | [Espejo emocional](reto-7-espejo-emocional/) | ⭐⭐ Medio | Umbrales de audio, máquina de estados, transiciones |
| 8 | [Alarma para el reloj](reto-8-alarma-reloj/) | ⭐⭐ Medio | NTP, comparación de tiempo, flags de estado |
| 10 | [Tu propia API](reto-10-nueva-api/) | ⭐⭐ Medio | HTTP GET, JSON parsing, documentación de APIs |
| 4 | [Contador de aplausos](reto-4-aplausos/) | ⭐⭐⭐ Difícil | Detección de picos, debounce de audio, nuevo modo |

---

---

## Reto 5 — Chrome Dino en OLED 🎮

**Objetivo:** Subir y jugar el clon del dino de Chrome directamente en la pantalla OLED de Mochi. Luego modificar la física para hacer el juego más o menos difícil.

**Control:** Sensor táctil (toca para saltar, mantén para agacharte).

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-5-chromedino
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-5-chromedino
```

> Este sketch usa LittleFS. Asegúrate de que `data/dino.bin` esté subido al filesystem antes de jugar.
> Usa el mismo método de LittleFS Data Upload de la Fase 4.

### Variables que puedes modificar

Abre `reto-5-chromedino/t-rex-duino.h` y busca el bloque "Misc. Settings" (alrededor de la línea 193):

| Variable | Valor por defecto | Efecto |
|----------|------------------|--------|
| `GROUND_CACTI_SCROLL_SPEED` | `3` | Velocidad de los obstáculos (más alto = más difícil) |
| `PTERODACTY_SPEED` | `5` | Velocidad del pterodáctilo |
| `TARGET_FPS_START` | `23` | FPS iniciales del juego (más alto = más rápido desde el inicio) |
| `TARGET_FPS_MAX` | `48` | Velocidad máxima que puede alcanzar el juego |
| `CACTI_RESPAWN_RATE` | `50` | Frecuencia de aparición de cactos (menos valor = más frecuentes) |
| `LIVES_START` | `3` | Número de vidas al comenzar |

**Reto propuesto:** duplica `GROUND_CACTI_SCROLL_SPEED` y supera 5 obstáculos sin morir.

---

## Reto 6 — Flappy Bird en OLED 🎮

**Objetivo:** Subir y jugar Flappy Bird en la OLED. Luego modificar la física del pájaro y las tuberías para crear tu propia versión del juego.

**Control:** Sensor táctil (toca para saltar).

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-6-flappybird
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-6-flappybird
```

### Variables que puedes modificar

Abre `reto-6-flappybird/flappyBird.ino` y busca el bloque de constantes al inicio del archivo:

```cpp
const int GRAVITY       = 5;    // ← gravedad sobre el pájaro
const int JUMP_STRENGTH = -25;  // ← fuerza del salto (negativo = sube)
const int PIPE_SPEED    = 10;   // ← velocidad de las tuberías
const int PIPE_GAP      = 30 * SCALE_FACTOR;  // ← espacio entre tuberías
const int PIPE_SPACING  = 65 * SCALE_FACTOR;  // ← distancia entre pares de tuberías
```

| Variable | Efecto al aumentar el valor |
|----------|-----------------------------|
| `GRAVITY` | El pájaro cae más rápido — más difícil |
| `JUMP_STRENGTH` | Salto más alto (usar valor más negativo, ej: `-35`) |
| `PIPE_SPEED` | Las tuberías vienen más rápido |
| `PIPE_GAP` | Espacio más pequeño entre tuberías — más difícil |
| `PIPE_SPACING` | Más espacio entre pares de tuberías — más fácil |

**Reto propuesto:** crea una versión "modo fácil" (gravedad baja, espacio amplio) y una "modo imposible" (gravedad alta, tuberías rápidas). ¿Cuál es la combinación más jugable?

---

## Reto 1 — Nueva melodía - Fácil

**Objetivo:** Reemplaza el tema de Tetris por una melodía de tu elección.

**Qué modificar:** Función `tetrisTheme()` en `mochi_unified_5.ino`.

**Cómo practicar sin tocar el firmware:** Usa el sketch [`reto-1-melodia/reto-1-melodia.ino`](reto-1-melodia/reto-1-melodia.ino) — tiene el array de notas vacío con instrucciones y una tabla de frecuencias. Toca el sensor para escuchar tu melodía.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-1-melodia
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-1-melodia
```

Guía completa → [reto-1-melodia/README.md](reto-1-melodia/README.md)

---

## Reto 2 — Nuevo mood: guiño - Medio

**Objetivo:** Agrega un undécimo estado de ánimo donde el robot guiña un ojo.

**Qué modificar:** Tres lugares en `mochi_unified_5.ino`: la constante `MOOD_COUNT`, la función `drawEyelidMask()` y el `switch` dentro de `updateEyes()`.

El README del reto tiene los tres snippets exactos que debes agregar, con la línea donde van.

Guía completa → [reto-2-mood-wink/README.md](reto-2-mood-wink/README.md)

---

## Reto 3 — GIF personalizado Fácil a Medio

**Objetivo:** Agrega una animación propia al Modo 1 del robot.

**Pasos clave:**
1. Consigue o crea un GIF de 128×64 px en blanco y negro.
2. Conviértelo con `herramientas/convertidor_gifs/gif.exe`.
3. Copia el `.bin` a `mochi_unified_5/data/`.
4. Sube el filesystem (LittleFS).

Guía completa → [reto-3-gif-propio/README.md](reto-3-gif-propio/README.md)

---

## Reto 4 — Contador de aplausos - Difícil

**Objetivo:** Crea un Modo 5 que cuente aplausos y muestre el contador en pantalla.

**Dos fases:**

**Fase 1 — Calibrar la detección:** usa el sketch standalone [`reto-4-aplausos.ino`](reto-4-aplausos/reto-4-aplausos.ino). No usa OLED, solo el monitor serial. Ajusta `CLAP_THRESHOLD` hasta que detecte correctamente.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-4-aplausos
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/retos/reto-4-aplausos
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

**Fase 2 — Integrar como Modo 5:** cuando la detección funcione, el README te guía con los 3 cambios exactos en `mochi_unified_5.ino`.

Guía completa → [reto-4-aplausos/README.md](reto-4-aplausos/README.md)

---

## Reto 10 — Tu propia API ⭐⭐ Medio

**Objetivo:** Conecta Mochi a cualquier API REST pública y muestra sus datos en la OLED.

**Qué necesitas:** WiFi funcionando (Prueba 07) y, opcionalmente, haber completado la Prueba 08 — Clima (que ya hace exactamente este patrón).

**El patrón que aprenderás:**
```
ESP32 → HTTP GET → API externa → JSON → parsear → mostrar en OLED
```

Algunas APIs gratuitas sin registro: Advice Slip (consejos), Open-Meteo (clima sin clave), Frankfurter (divisas), Bored API (actividades).

Guía completa → [reto-10-nueva-api/README.md](reto-10-nueva-api/README.md)

---

**← Anterior:** [mochi_unified_5/](../mochi_unified_5/) — funciones del robot
**Inicio →** [README principal](../../README.md)
