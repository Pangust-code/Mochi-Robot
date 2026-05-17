# Firmware — mochi_unified_5 (v6)

Este es el firmware principal de Mochi. Está escrito en Arduino/C++ para el **ESP32-C6 Supermini**.

> **Estado:** Beta funcional. Todos los modos operan correctamente.

---

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `mochi_unified_5.ino` | Código principal del firmware |
| `data/` | Animaciones en formato binario para el modo GIF |

---

## Estructura del código

El código está organizado en secciones bien delimitadas. Esta es la estructura general:

```
mochi_unified_5.ino
│
├── Configuración de hardware (pines, I2C, I2S)
├── Definición de estados de ánimo (10 moods)
├── Sistema de físicas para los ojos (spring-damper)
├── Manejo de toque táctil (eventos)
├── Manejo de micrófono (I2S)
│
├── setup()   — inicializa pantalla, WiFi (si modo reloj), LittleFS
└── loop()
    ├── leer sensor táctil → publicar eventos
    ├── leer micrófono
    ├── procesar eventos (toque corto, toque largo, ráfaga de toques)
    └── renderizar modo activo
        ├── Modo 0: Mascota (ojos animados)
        ├── Modo 1: GIFs (LittleFS)
        ├── Modo 2: Reloj (NTP)
        ├── Modo 3: Pomodoro
        └── Modo 4: Test micrófono
```

---

## Modos explicados

### Modo 0 — Mascota (por defecto)

Muestra ojos animados en la pantalla OLED con física de primavera (spring-damper) para movimientos suaves.

**Sistema de ánimos — 10 estados:**

| Estado | Qué muestra |
|--------|------------|
| `NORMAL` | Ojos redondos, parpadeando |
| `HAPPY` | Ojos curvados hacia arriba |
| `SURPRISED` | Ojos grandes, abiertos |
| `SLEEPY` | Parpados caídos, lentos |
| `ANGRY` | Cejas inclinadas hacia abajo |
| `SAD` | Ojos caídos en las esquinas |
| `EXCITED` | Ojos muy abiertos, brillantes |
| `LOVE` | Ojos en forma de corazón |
| `SUSPICIOUS` | Un ojo semicerrado |
| `DIZZY` | Ojos en espiral |

**Controles:**
- **1 toque** → ánimo aleatorio
- **3+ toques rápidos** → reproduce el tema de Tetris 🎵
- **Sonido fuerte** → reacción de susto por 2 segundos

**Características de animación:**
- Parpadeo automático cada 2–6 segundos
- Movimientos sacádicos (ojos que se mueven solos de forma natural)
- Las pupilas siguen patrones de movimiento

---

### Modo 1 — GIFs

Reproduce los 14 archivos `.bin` de la carpeta `data/` en secuencia, a **20 FPS**, usando la memoria LittleFS del ESP32.

**Animaciones incluidas:**

| Archivo | Descripción |
|---------|-------------|
| `0.bin` | Animación por defecto |
| `star.bin` | Estrella |
| `bee.bin` | Abeja |
| `bmo.bin` | BMO (personaje de Hora de Aventura) |
| `sushi.bin` | Sushi |
| `angry.bin`, `angry3.bin` | Expresiones de enojo |
| `guess.bin` | Interrogación / pensando |
| `speed.bin` | Velocidad |
| `yakura.bin`, `gian_du.bin`, `cuoi_khinh_bi.bin`, `sung_nuoc.bin`, `sung_nuoc_2.bin` | Expresiones varias |

> Para agregar tus propias animaciones: convierte los frames a binario (128×64 px, 1 bit por píxel) y coloca el archivo `.bin` en la carpeta `data/`.

---

### Modo 2 — Reloj

Conecta a WiFi, sincroniza la hora vía NTP y muestra la hora y fecha en español (zona horaria Ecuador, UTC-5).

**Configuración WiFi:** edita las líneas en el código:
```cpp
const char* ssid     = "TU_RED_WIFI";
const char* password = "TU_CONTRASEÑA";
```

---

### Modo 3 — Pomodoro

Temporizador de productividad basado en la técnica Pomodoro:
- **Foco:** 25 minutos
- **Descanso:** 5 minutos

**Controles:**
- **1 toque** → iniciar / pausar
- **3+ toques** → reiniciar

Al terminar cada sesión, el buzzer emite un tono de aviso.

---

### Modo 4 — Test de micrófono

Muestra en tiempo real una barra de volumen basada en la amplitud del micrófono INMP441. Útil para verificar que el micrófono esté funcionando.

- **Umbral de reacción:** amplitud > 15000
- **Frecuencia de muestreo:** 16 kHz (I2S)

---

## Cómo cambiar de modo

Mantén presionado el sensor táctil por **más de 800 ms** hasta escuchar un tono. Cada hold largo avanza al siguiente modo en ciclo:

```
Mascota → GIFs → Reloj → Pomodoro → Micrófono → Mascota → ...
```

---

## Cómo subir el firmware

### 1. Subir el código

```bash
# Compilar con partición huge_app (obligatorio por el tamaño)
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app .

# Subir al ESP32 (reemplaza COM3 con tu puerto)
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 .
```

O usa las tareas de VS Code desde [02_software/](../../02_software/).

### 2. Subir los archivos de animación (data/)

Los archivos `.bin` deben subirse a la memoria LittleFS del ESP32 por separado:

```bash
# Instalar el plugin de LittleFS si no lo tienes
arduino-cli core update-index

# Subir filesystem (usa la tarea de VS Code o el plugin)
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 --input-dir data/
```

> Si no subes la carpeta `data/`, el Modo 1 (GIFs) no mostrará animaciones.

---

## Solución de problemas comunes

| Problema | Causa probable | Solución |
|----------|---------------|----------|
| Pantalla en blanco | I2C mal conectado | Verifica SDA=6, SCL=7 |
| Error de compilación "sketch too big" | Partición incorrecta | Usa `PartitionScheme=huge_app` |
| Reloj no sincroniza | WiFi incorrecto o sin cobertura | Verifica SSID y contraseña |
| GIFs no se reproducen | `data/` no subida | Sube el filesystem LittleFS |
| Micrófono sin respuesta | INMP441 mal conectado | Verifica L/R conectado a GND |

---

## Siguiente paso

¿Quieres modificar el robot? Empieza por cambiar el SSID de WiFi en el Modo 2 o agregar un nuevo estado de ánimo. El código está comentado internamente para guiarte.
