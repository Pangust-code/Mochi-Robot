# Sección 4 — Retos opcionales

> **Prerrequisito:** [mochi_unified_5/](../mochi_unified_5/) — haber leído las funciones del robot.

Esta carpeta contiene los retos de personalización del taller. Cada uno tiene su propio directorio con instrucciones y, donde corresponde, código listo para modificar.

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
└── reto-6-flappybird/         ← Easter Egg: Flappy Bird en OLED
    └── flappyBird.ino         ← variables de física al inicio del archivo
```

---

## Índice de retos

| # | Reto | Nivel | Tipo de cambio |
|---|------|-------|---------------|
| 5 | [Chrome Dino](#reto-5--chrome-dino-en-oled-) | Easter Egg | Modificar constantes de física en `t-rex-duino.h` |
| 6 | [Flappy Bird](#reto-6--flappy-bird-en-oled-) | Easter Egg | Modificar constantes de física en `flappyBird.ino` |
| 1 | [Nueva melodía](reto-1-melodia/) | Fácil | Modificar array de notas en `tetrisTheme()` |
| 2 | [Nuevo mood: guiño](reto-2-mood-wink/) | Medio | Agregar constante + máscara de párpado + tamaño |
| 3 | [GIF personalizado](reto-3-gif-propio/) | Medio | Convertir GIF y subirlo a LittleFS |
| 4 | [Contador de aplausos](reto-4-aplausos/) | Difícil | Agregar Modo 5 con detección de audio |

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

## ¿Quieres más retos?

Ideas para seguir explorando:

- **Modo "Espejo":** los ojos reflejan el nivel de ruido en tiempo real (SLEEPY en silencio → EXCITED con ruido).
- **Reacción por hora:** si son las 12:00 en el Modo Reloj, reproduce una melodía especial.
- **Pomodoro con animación:** al terminar el Pomodoro, muestra el Modo 1 durante 10 segundos antes de volver.

---

**← Anterior:** [mochi_unified_5/](../mochi_unified_5/) — funciones del robot
**Inicio →** [README principal](../../README.md)
