# Sección 3 — Funciones del robot

> **Prerrequisito:** [03_firmware/](../) — firmware cargado y funcionando.

Esta carpeta contiene el código fuente completo del robot. Esta sección explica cómo está organizado y qué hace cada módulo, para que puedas modificarlo con confianza.

---

## Archivos de esta carpeta

| Archivo | Contenido |
|---------|-----------|
| `mochi_unified_5.ino` | Código fuente principal del robot |
| `data/` | Animaciones en formato `.bin` para el Modo 1 (GIFs) |

---

## Estructura general del código

```
mochi_unified_5.ino
│
├── Configuración (pines, constantes, variables globales)
├── Struct Eye           — física spring-damper de los ojos
├── procesTouch()        — lectura centralizada del sensor táctil
├── handleModeChange()   — transición entre modos al hacer hold
├── initFS()             — monta LittleFS (sistema de archivos)
├── initMicrophone()     — configura I2S a 16 kHz
├── leerAmplitud()       — lee nivel de ruido del micrófono
│
├── setup()
│   ├── Inicializa OLED (I2C)
│   ├── Configura micrófono (I2S)
│   ├── Monta LittleFS
│   └── Si modo 2 activo → conecta WiFi + NTP
│
└── loop()  [~30 veces/segundo]
    ├── procesTouch()       → publica eventos de interacción
    ├── handleModeChange()  → si hubo hold, avanza modo
    ├── leerAmplitud()      → detecta ruidos fuertes
    └── renderiza el modo activo
        ├── Modo 0: Mascota (ojos + moods)
        ├── Modo 1: GIFs (LittleFS → U8g2)
        ├── Modo 2: Reloj (WiFi + NTP)
        ├── Modo 3: Pomodoro
        └── Modo 4: Test micrófono
```

---

## Módulos — tabla resumen

| Módulo | Función | Entradas | Salidas |
|--------|---------|----------|---------|
| `procesTouch()` | Lee el sensor táctil UNA VEZ por ciclo y publica eventos | GPIO 2 | Variables `ev_*` |
| Motor de ojos | Anima dos ojos con física spring-damper | `currentMood`, `millis()` | Píxeles OLED |
| Sistema de moods | 10 formas de dibujar los ojos | ID de mood (0–9) | Tamaño y máscara de párpado |
| `leerAmplitud()` | Lee nivel de audio del micrófono | I2S DMA | Amplitud (int) |
| `handleModeChange()` | Avanza al siguiente modo | `ev_longHold` | `appMode` actualizado |
| Modo 0 — Mascota | Ojos reactivos al tacto y al sonido | Eventos touch, amplitud | OLED + buzzer |
| Modo 1 — GIFs | Reproduce `.bin` a 20 FPS | LittleFS → `data/` | OLED (U8g2) |
| Modo 2 — Reloj | Muestra hora y fecha en español | WiFi, NTP pool.ntp.org | OLED |
| Modo 3 — Pomodoro | Timer 25/5 min con buzzer | Eventos touch | OLED + buzzer |
| Modo 4 — Test mic | Barra de volumen en tiempo real | I2S DMA | OLED |

---

## Módulo: `procesTouch()`

**Por qué existe:** Centralizar la lectura del pin evita que dos partes del código lo lean simultáneamente y pierdan eventos — un bug clásico en Arduino.

**Eventos publicados (booleanos, se reinician cada ciclo):**

| Variable | Cuándo es `true` |
|----------|-----------------|
| `ev_shortTap` | Toque corto soltado (< 800 ms) |
| `ev_longHold` | Toque largo soltado (≥ 800 ms) |
| `ev_isHolding` | Está presionado ≥ 800 ms ahora mismo |
| `ev_tapsReady` | Expiró la ventana de taps (900 ms) |
| `ev_tapCount` | Cuántos taps hubo en esa ventana |

---

## Módulo: Motor de ojos (spring-damper)

Los ojos no "saltan" a su posición — se mueven con física de resorte amortiguado, lo que da un movimiento orgánico.

**Parámetros en `struct Eye`:**

| Parámetro | Valor | Efecto |
|-----------|-------|--------|
| `k = 0.12` | Constante del resorte | Velocidad de aproximación al objetivo |
| `d = 0.60` | Amortiguamiento | Suavidad del freno al llegar |
| Parpadeo | cada 2–6 s | Aleatorio, natural |
| Saccade | cada 0.5–3 s | Movimiento espontáneo de pupilas |

---

## Módulo: Sistema de moods (10 estados de ánimo)

| ID | Constante | Cómo se ven los ojos |
|----|-----------|---------------------|
| 0 | `MOOD_NORMAL` | Redondos, parpadeando |
| 1 | `MOOD_HAPPY` | Curvados hacia arriba |
| 2 | `MOOD_SURPRISED` | Muy abiertos |
| 3 | `MOOD_SLEEPY` | Parpados a la mitad |
| 4 | `MOOD_ANGRY` | Cejas en V |
| 5 | `MOOD_SAD` | Comisuras caídas |
| 6 | `MOOD_EXCITED` | Muy grandes y abiertos |
| 7 | `MOOD_LOVE` | Con corazón superpuesto |
| 8 | `MOOD_SUSPICIOUS` | Un ojo semicerrado |
| 9 | `MOOD_DIZZY` | En espiral girando |

**Cómo cambiar el mood inicial:**
```cpp
// En setup(), busca esta línea y cambia el valor:
currentMood = MOOD_NORMAL;   // ← escribe cualquier MOOD_X
```

---

## Modos — controles y personalización

### Modo 0 — Mascota (por defecto)

| Gesto | Resultado |
|-------|-----------|
| 1 tap | Mood aleatorio + beep 800 Hz |
| 3+ taps rápidos | Tema de Tetris 🎵 |
| Sonido fuerte (amp > 15 000) | `MOOD_SURPRISED` 2 segundos |

**Para cambiar la melodía de 3 taps**, busca `tetrisTheme()` y modifica el array `mel[]`. Tienes un sketch de práctica en [`retos/reto-1-melodia/`](../retos/reto-1-melodia/).

### Modo 1 — GIFs

Reproduce los archivos de `data/` en secuencia a 20 FPS. Para agregar tu propio GIF convierte el archivo con `herramientas/convertidor_gifs/gif.exe` y cópialo a `data/`. Guía completa en [`retos/reto-3-gif-propio/`](../retos/reto-3-gif-propio/).

### Modo 2 — Reloj

Conecta a WiFi y sincroniza con `pool.ntp.org`. Zona horaria UTC-5 (Ecuador). Requiere editar `ssid` y `password` en el código (ver [03_firmware/README.md](../README.md), Paso 2).

### Modo 3 — Pomodoro

| Gesto | Resultado |
|-------|-----------|
| 1 tap | Iniciar / pausar |
| 3+ taps | Reiniciar desde 25:00 |

**Para cambiar la duración:**
```cpp
int pomoSeconds = 25 * 60;   // ← cambia 25 por los minutos que quieras
```

### Modo 4 — Test de micrófono

Muestra una barra de volumen en tiempo real. Un tap reinicia el valor de pico máximo.

- Barra vacía en silencio → ✅ micrófono correcto
- Barra siempre llena → ❌ pin L/R no está a GND
- Barra sin movimiento → ❌ verifica WS (GPIO 5), SCK (GPIO 4), SD (GPIO 3)

---

## Animaciones disponibles en `data/`

| Archivo | Descripción |
|---------|-------------|
| `0.bin` | Animación por defecto |
| `star.bin`, `bee.bin`, `bmo.bin`, `sushi.bin` | Íconos y objetos |
| `angry.bin`, `angry3.bin`, `gian_du.bin` | Expresiones de enojo |
| `speed.bin`, `yakura.bin`, `cuoi_khinh_bi.bin` | Expresiones varias |
| `sung_nuoc.bin`, `sung_nuoc_2.bin` | Más expresiones |
| `cafe.bin`, `mario-sick.bin`, `pacman.bin` | Personajes y objetos |

Hay 99 GIFs adicionales en [`recursos/gifs/`](../../recursos/gifs/) para convertir y agregar.

---

## ¿Algo no funciona en el código?

Consulta la [guía de errores comunes](../../docs/errores-comunes.md).

---

## Fase 5 — Configuración e integración final

Esta sección es el último paso antes de cerrar la carcasa. Editarás únicamente la **sección de configuración** del firmware — no necesitas entender el resto del código para completarla.

### Qué editar en `mochi_unified_5.ino`

Abre el archivo y localiza el bloque de configuración cerca del inicio (líneas 50–80 aproximadamente). Solo modificarás valores dentro de ese bloque.

#### 1. Credenciales WiFi

Necesarias para el Modo 2 (Reloj). Si no vas a usar el reloj, puedes dejarlo en blanco.

```cpp
const char* ssid     = "NOMBRE_DE_TU_RED";   // ← tu red WiFi 2.4 GHz
const char* password = "TU_CONTRASEÑA";
```

#### 2. Umbral de sonido (valor de tu Fase 2)

Este es el valor que mediste en la Prueba 08. Un sonido por encima de este umbral hace que Mochi se sobresalte.

```cpp
const int SOUND_THRESHOLD = 15000;   // ← reemplaza con tu valor de la Fase 2
```

> Si no modificas este valor, el umbral por defecto (15 000) puede no corresponder a tu entorno.

#### 3. Nombres de archivos de animación

Si convertiste GIFs propios en la Fase 4, agrega sus nombres al array de la lista de archivos. Busca la sección donde se listan los `.bin`:

```cpp
// Ejemplo: si convertiste "feliz.bin" y "enojado.bin"
// Asegúrate de que esos archivos estén en data/ y hayan sido subidos por LittleFS
```

El firmware carga automáticamente todos los `.bin` que encuentre en `data/` — solo asegúrate de que los archivos estén subidos correctamente.

---

### Pasos para subir el firmware configurado

**1. Compilar con la partición correcta:**

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

**2. Subir el firmware:**

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

**3. Subir las animaciones (LittleFS):**

Cierra el Monitor Serie → `Ctrl+Shift+P` → **Upload LittleFS to Pico/ESP8266/ESP32**.

> Si aún no tienes la extensión instalada, sigue la guía en [herramientas/README.md](../../herramientas/README.md) — Paso 5.

**4. Abrir el Monitor Serie y verificar:**

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

Salida esperada al iniciar correctamente:

```
=== MOCHI v6 ===
LittleFS OK — XX/128 KB
I2S: inicializado
-> Modo 0 (Mascota)
```

---

### Checklist — Fase 5

- [ ] WiFi configurado (o dejado vacío a propósito)
- [ ] Umbral de sonido ingresado con el valor de tu Fase 2
- [ ] Compilación sin errores (`PartitionScheme=huge_app`)
- [ ] Subida exitosa ("Hard resetting via RTS pin")
- [ ] Monitor Serie muestra `=== MOCHI v6 ===`
- [ ] Pantalla OLED muestra ojos animados (Modo 0)
- [ ] Tocar el sensor cambia el mood
- [ ] Hold largo cambia de modo
- [ ] Modo 1 muestra tus animaciones personalizadas
- [ ] Carcasa cerrada ✅

---

### Arquitectura del sistema completo

Para entender cómo fluye la información desde el hardware hasta la respuesta visual, consulta:

**[docs/arquitectura.md](../../docs/arquitectura.md)** — diagramas Mermaid del hardware, firmware y pipeline IoT

---

**← Anterior:** [03_firmware/](../) — carga del firmware
**Siguiente →** [03_firmware/retos/](../retos/) — personalizar el robot
