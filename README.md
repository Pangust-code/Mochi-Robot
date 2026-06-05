# Mochi — Robot de compañía para talleres educativos

Mochi es un pequeño robot con personalidad construido sobre un **ESP32-C6 Supermini**. Tiene ojos animados en una pantalla OLED, reacciona al tacto y al sonido, muestra la hora en tiempo real y puede reproducir animaciones GIF. Es ideal para aprender electrónica, programación embebida y diseño de interacciones en un ambiente de taller.

> Este repositorio es tu **manual completo**: desde conectar los cables hasta personalizar el robot con tus propias ideas.

---

## Objetivo del taller

Al terminar este taller habrás:

- Ensamblado un robot con pantalla, micrófono, buzzer y sensor táctil.
- Cargado firmware en un microcontrolador ESP32 desde la terminal.
- Entendido cómo funciona cada componente del código.
- Personalizado el comportamiento del robot con tus propias modificaciones.
- Llevado a casa un repositorio que funciona como guía de referencia permanente.

**No necesitas experiencia previa en electrónica.** Solo conocimientos básicos de Arduino (variables, funciones, `setup()` / `loop()`).

---

## ¿Qué vas a construir?

Un robot compacto con cinco modos de operación:

| Modo | Ícono | Descripción |
|------|-------|-------------|
| Mascota | 🐾 | Ojos animados que reaccionan al tacto y al sonido |
| GIFs | 🎞 | Animaciones almacenadas en memoria interna |
| Reloj | 🕐 | Hora y fecha sincronizadas vía WiFi (NTP) |
| Pomodoro | 🍅 | Temporizador 25/5 min para estudiar |
| Micrófono | 🎙 | Visualizador de volumen en tiempo real |

Para cambiar de modo: **mantén presionado el sensor táctil** hasta escuchar un beep (≥ 800 ms).

---

## Arquitectura general

```
┌─────────────────────────────────────────────────────────┐
│                    HARDWARE                             │
│                                                         │
│  [TTP223]──GPIO2──┐                                     │
│  [INMP441]──I2S───┤   ESP32-C6 Supermini                │
│  [OLED SH1106]────┤   ├── LittleFS (GIFs .bin)          │
│  [Buzzer]──GPIO19─┘   └── WiFi/NTP (modo Reloj)         │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│                    FIRMWARE (mochi_unified_5)           │
│                                                         │
│  procesTouch() ──► eventos (tap / hold / ráfaga)        │
│  leerAmplitud() ──► nivel de ruido ambiental            │
│                                                         │
│  loop()                                                 │
│   ├── Modo 0: Motor de ojos (spring-damper + 10 moods)  │
│   ├── Modo 1: Reproductor GIF (LittleFS → U8g2)         │
│   ├── Modo 2: Reloj NTP (WiFi → time.h)                 │
│   ├── Modo 3: Pomodoro (25 min + 5 min)                 │
│   └── Modo 4: Test micrófono (barra de volumen)         │
└─────────────────────────────────────────────────────────┘
```

Diagrama Mermaid detallado → [docs/arquitectura.md](docs/arquitectura.md)

---

## Hardware necesario

| Componente | Especificación | Notas |
|-----------|---------------|-------|
| Microcontrolador | **ESP32-C6 Supermini** | WiFi + Bluetooth integrado |
| Pantalla | **OLED SH1106 128×64**, I2C | Dirección `0x3C` |
| Sensor táctil | **TTP223** (capacitivo) | Módulo listo para usar |
| Micrófono | **INMP441**, I2S | Conectar L/R a GND |
| Buzzer | **Pasivo** (no activo) | El activo solo emite un tono fijo |
| Protoboard | 400 puntos o más | Para el prototipo |
| Cables Dupont | Hembra-hembra y hembra-macho | Para conexiones |
| Cable USB | **USB-C** | Para programar y alimentar |

### Diagrama de conexiones

| Componente | Pin | GPIO ESP32-C6 |
|-----------|-----|--------------|
| OLED | VCC | 3.3V |
| OLED | GND | GND |
| OLED | SDA | **GPIO 6** |
| OLED | SCL | **GPIO 7** |
| TTP223 | VCC | 3.3V |
| TTP223 | GND | GND |
| TTP223 | OUT | **GPIO 2** |
| INMP441 | VDD | 3.3V |
| INMP441 | GND | GND |
| INMP441 | L/R | GND |
| INMP441 | WS | **GPIO 5** |
| INMP441 | SCK | **GPIO 4** |
| INMP441 | SD | **GPIO 3** |
| Buzzer | + | **GPIO 19** |
| Buzzer | − | GND |

> Usa siempre **3.3V**, nunca 5V. El ESP32-C6 no tolera 5V en sus pines GPIO.

Guía completa de hardware → [01_hardware/README.md](01_hardware/README.md)

---

## Prerrequisitos de software

| Herramienta | Versión mínima | Para qué sirve |
|------------|---------------|----------------|
| **Arduino CLI** | 0.35+ | Compilar y subir código desde la terminal |
| **VS Code** | Cualquiera | Editor con tareas preconfiguradas |
| **Driver CH340/CP2102** | — | Para que el PC reconozca el ESP32 por USB |
| **Core ESP32** | esp32:esp32 3.x | Soporte para el ESP32-C6 |

### Librerías Arduino necesarias

| Librería | Autor | Usada en |
|---------|-------|---------|
| `Adafruit GFX Library` | Adafruit | Gráficos en la pantalla OLED |
| `Adafruit SH110X` | Adafruit | Driver específico del SH1106 |
| `U8g2` | olikraus | Renderizado de GIFs en modo 1 |

---

## Ruta de aprendizaje

Abre cada carpeta en orden y lee el `README.md` que está dentro — ese es la guía de esa sección:

| Paso | Carpeta | Guía | Resultado |
|------|---------|------|-----------|
| 1 | [02_software/](02_software/) | [README.md](02_software/README.md) | Arduino CLI, librerías y driver USB instalados |
| 2 | [03_firmware/](03_firmware/) | [README.md](03_firmware/README.md) | Robot compilado, cargado y funcionando |
| 3 | [03_firmware/mochi_unified_5/](03_firmware/mochi_unified_5/) | [README.md](03_firmware/mochi_unified_5/README.md) | Entiendes cada módulo del código |
| 4 | [03_firmware/retos/](03_firmware/retos/) | [README.md](03_firmware/retos/README.md) | Robot personalizado con tu propio toque |

---

---

## Sección 1 — Preparación del entorno

### 1.1 Instalar Arduino CLI

**Windows:**
```powershell
winget install --id ArduinoSA.CLI -e --source winget
```

Cierra y vuelve a abrir la terminal. Verifica la instalación:
```powershell
arduino-cli version
# → arduino-cli  Version: 0.35.x ...
```

> Si el comando no se reconoce, usa la ruta completa: `& "C:\Program Files\Arduino CLI\arduino-cli.exe" version`

**Mac:**
```bash
brew install arduino-cli
arduino-cli version
```

**Linux:**
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/
arduino-cli version
```

---

### 1.2 Instalar el core de ESP32

```powershell
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

Verifica que el ESP32-C6 aparece en la lista:
```powershell
arduino-cli board listall | grep esp32c6
# → esp32:esp32:esp32c6   ESP32-C6 Dev Module
```

---

### 1.3 Instalar las librerías

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
arduino-cli lib install "U8g2"
```

Verifica que están instaladas:
```powershell
arduino-cli lib list
# Debe aparecer: Adafruit GFX Library, Adafruit SH110X, U8g2
```

---

### 1.4 Instalar el driver USB

Si tu ESP32-C6 Supermini no aparece como puerto COM al conectarlo por USB, necesitas el driver del chip USB-serie:

- **CH340:** [Descargar driver CH340](https://www.wch-ic.com/downloads/CH341SER_EXE.html)
- **CP2102:** [Descargar driver CP2102](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

Después de instalarlo, reconecta el ESP32 y ejecuta:
```powershell
arduino-cli board list
# → COM3   esp32:esp32:esp32c6   ESP32-C6 Dev Module
```

---

### 1.5 Clonar el repositorio

```bash
git clone https://github.com/Pangust-code/Mochi-Robot.git
cd Mochi-Robot
```

---

### ✅ Checklist Sección 1

- [ ] `arduino-cli version` muestra una versión instalada
- [ ] `arduino-cli board listall | grep esp32c6` devuelve resultados
- [ ] Las tres librerías aparecen en `arduino-cli lib list`
- [ ] El ESP32 aparece como puerto COM al conectarlo por USB

---

---

## Sección 2 — Carga del firmware

### 2.1 Configurar el WiFi (solo para el modo Reloj)

Abre [03_firmware/mochi_unified_5/mochi_unified_5.ino](03_firmware/mochi_unified_5/mochi_unified_5.ino) y edita las líneas 57–58:

```cpp
const char* ssid     = "TU_RED_WIFI";      // ← nombre de tu red
const char* password = "TU_CONTRASEÑA";   // ← contraseña de tu red
```

> Si no configuras el WiFi, los modos 0, 1, 3 y 4 funcionarán igual. Solo el **Modo 2 (Reloj)** necesita conexión.

---

### 2.2 Identificar el puerto del ESP32

Conecta el ESP32 por USB y ejecuta:
```powershell
arduino-cli board list
```

Busca la línea que muestre tu ESP32. El puerto será algo como `COM3`, `COM4` (Windows) o `/dev/ttyUSB0` (Linux/Mac). Anota ese puerto — lo usarás en los siguientes pasos.

---

### 2.3 Compilar el firmware

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

> La opción `PartitionScheme=huge_app` es **obligatoria**. El firmware es grande y no cabe en la partición por defecto. Sin ella verás el error `Image length ... doesn't fit in partition length`.

Una compilación exitosa termina con:
```
Sketch uses 987654 bytes (47%) of program storage space.
```

---

### 2.4 Subir el firmware

Reemplaza `COM3` con el puerto que encontraste en el paso 2.2:

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

Una subida exitosa termina con:
```
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

---

### 2.5 Subir las animaciones GIF (LittleFS)

Las animaciones del Modo 1 se almacenan en la memoria interna del ESP32 como archivos `.bin`. Deben subirse por separado al sistema de archivos LittleFS usando la extensión **arduino-littlefs-upload** para VS Code.

#### Instalar la extensión (una sola vez)

> Guía en video: [youtube.com/watch?v=vICDKOLizrU](https://www.youtube.com/watch?v=vICDKOLizrU)

1. Descarga **[arduino-littlefs-upload-1.5.4.vsix](https://github.com/earlephilhower/arduino-littlefs-upload/releases/download/1.5.4/arduino-littlefs-upload-1.5.4.vsix)**
2. En VS Code: `Ctrl+Shift+X` → `···` → **Instalar desde VSIX...** → selecciona el archivo
3. Reinicia VS Code.

#### Subir los archivos

1. Cierra el Monitor Serie.
2. `Ctrl+Shift+P` → **Upload LittleFS to Pico/ESP8266/ESP32**

#### Opción alternativa — esptool
```powershell
mklittlefs -c 03_firmware/mochi_unified_5/data -p 256 -b 4096 -s 0x150000 littlefs.bin
esptool.py --chip esp32c6 --port COM3 write_flash 0x290000 littlefs.bin
```

> Si no subes la carpeta `data/`, el **Modo 1 (GIFs)** no mostrará animaciones y mostrará un mensaje de error, pero los demás modos funcionarán con normalidad.

---

### 2.6 Verificar el funcionamiento

Abre el monitor serial para ver los mensajes de inicio:

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

Deberías ver algo como:
```
=== MOCHI v6 ===
LittleFS OK — 42/128 KB
I2S: inicializado
-> Modo 0 (Mascota)
```

#### Con VS Code
Usa `Ctrl+Shift+B` para compilar o ejecuta las tareas desde el menú **Terminal → Run Task**:
- `Arduino: Compilar ESP32-C6 (huge_app)`
- `Arduino: Subir al ESP32-C6`
- `Arduino: Monitor Serial (115200)`

---

### ✅ Checklist Sección 2

- [ ] La compilación termina sin errores
- [ ] La subida termina con "Hard resetting via RTS pin"
- [ ] El monitor serial muestra mensajes de inicio
- [ ] La pantalla OLED enciende y muestra ojos animados
- [ ] Al tocar el sensor se escucha un beep y los ojos cambian
- [ ] Al hacer hold (≥ 800 ms) se escucha otro beep y cambia el modo
- [ ] El buzzer responde (se escuchan los tonos de feedback)

---

---

## Sección 3 — Funciones del robot

### Tabla resumen de módulos

| Módulo | Función principal | Archivo |
|--------|------------------|---------|
| `procesTouch()` | Lee el sensor táctil y publica eventos | `mochi_unified_5.ino` |
| `Motor de ojos` | Anima los ojos con física spring-damper | `mochi_unified_5.ino` |
| `Sistema de moods` | Gestiona los 10 estados de ánimo | `mochi_unified_5.ino` |
| `leerAmplitud()` | Lee el nivel de ruido del micrófono | `mochi_unified_5.ino` |
| `Modo 0 — Mascota` | Ojos reactivos al tacto y al sonido | `mochi_unified_5.ino` |
| `Modo 1 — GIFs` | Reproduce animaciones desde LittleFS | `mochi_unified_5.ino` |
| `Modo 2 — Reloj` | Conecta a WiFi y sincroniza hora NTP | `mochi_unified_5.ino` |
| `Modo 3 — Pomodoro` | Temporizador 25/5 min | `mochi_unified_5.ino` |
| `Modo 4 — Test mic` | Barra de volumen en tiempo real | `mochi_unified_5.ino` |

---

### Módulo: procesTouch()

**Qué hace:** Es el único lugar del código que lee el pin del sensor táctil. Detecta tres tipos de interacciones y las publica como "eventos" que los demás módulos consumen.

**Por qué existe:** Centralizar la lectura del sensor evita que varios módulos lean el pin al mismo tiempo y pierdan eventos (bug clásico en proyectos Arduino).

**Entradas:** Pin GPIO 2 (señal HIGH = tocado).

**Salidas (eventos publicados):**

| Variable | Tipo | Cuándo es `true` |
|----------|------|-----------------|
| `ev_shortTap` | bool | Toque corto (< 800 ms) |
| `ev_longHold` | bool | Toque largo (≥ 800 ms), al soltar |
| `ev_isHolding` | bool | Mientras se mantiene presionado ≥ 800 ms |
| `ev_tapsReady` | bool | Cuando expira la ventana de taps (900 ms) |
| `ev_tapCount` | int | Cuántos taps hubo en esa ventana |

**Gestos reconocidos:**

| Gesto | Acción en Modo Mascota | Acción global |
|-------|----------------------|---------------|
| 1 tap corto | Cambiar estado de ánimo | — |
| 3+ taps rápidos | Reproducir tema de Tetris | — |
| Hold (≥ 800 ms) | — | Avanzar al siguiente modo |

---

### Módulo: Motor de ojos

**Qué hace:** Dibuja y anima dos ojos en la pantalla OLED usando un sistema de física llamado **spring-damper** (resorte amortiguado). Los ojos se mueven suavemente en lugar de saltar de posición.

**Por qué existe:** Las animaciones directas (saltar de A a B) se ven mecánicas. El spring-damper hace que el movimiento se vea orgánico, como si los ojos tuvieran peso.

**Entradas:** Estado de ánimo actual (`currentMood`), tiempo (`millis()`).

**Salidas:** Pixels dibujados en la pantalla OLED.

**Características:**
- Parpadeo automático cada 2–6 segundos.
- Movimientos sacádicos (los ojos se mueven solos de forma natural).
- Animación de respiración (los ojos se expanden y contraen levemente).

---

### Módulo: Sistema de moods (estados de ánimo)

**Qué hace:** Define 10 formas diferentes de dibujar los ojos.

**Entradas:** ID del estado de ánimo (0–9).

**Salidas:** Parámetros de tamaño y máscara de párpado para el motor de ojos.

| ID | Nombre | Aspecto visual |
|----|--------|---------------|
| 0 | `NORMAL` | Ojos redondos, parpadeando |
| 1 | `HAPPY` | Ojos curvados hacia arriba (feliz) |
| 2 | `SURPRISED` | Ojos muy abiertos |
| 3 | `SLEEPY` | Parpados caídos a la mitad |
| 4 | `ANGRY` | Cejas anguladas hacia abajo |
| 5 | `SAD` | Ojos caídos en las esquinas |
| 6 | `EXCITED` | Ojos muy grandes y abiertos |
| 7 | `LOVE` | Ojos con fondo de corazón |
| 8 | `SUSPICIOUS` | Un ojo semicerrado, el otro normal |
| 9 | `DIZZY` | Ojos en espiral girando |

---

### Modo 0 — Mascota (por defecto)

**Qué hace:** Muestra ojos animados que reaccionan al entorno.

**Controles:**
- **1 tap** → cambia al siguiente estado de ánimo (aleatorio).
- **3+ taps rápidos** → toca el tema de Tetris 🎵.
- **Sonido fuerte** → reacción de susto: ojos `SURPRISED` por 2 segundos.

**Ejemplo de personalización:**
Para que el robot siempre comience en modo HAPPY, cambia en `setup()`:
```cpp
currentMood = MOOD_HAPPY;   // antes era MOOD_NORMAL
```

---

### Modo 1 — GIFs

**Qué hace:** Reproduce en secuencia los 14 archivos `.bin` de la carpeta `data/`, a 20 FPS, usando el sistema de archivos LittleFS del ESP32.

**Cómo agregar tus propias animaciones:**
1. Elige un GIF de la carpeta `recursos/gifs/` o usa el tuyo.
2. Conviértelo a formato `.bin` con `herramientas/convertidor_gifs/gif.exe`.
3. Copia el `.bin` a `03_firmware/mochi_unified_5/data/`.
4. Sube el filesystem (paso 2.5).

---

### Modo 2 — Reloj

**Qué hace:** Conecta al WiFi configurado, sincroniza la hora con el servidor NTP (`pool.ntp.org`) y muestra la hora y fecha en español, ajustada a la zona horaria UTC-5 (Ecuador).

**Requisito:** Editar `ssid` y `password` en el código antes de subir (ver paso 2.1).

**Si no hay WiFi disponible:** El robot muestra `WiFi: error` y queda estático en ese modo.

---

### Modo 3 — Pomodoro

**Qué hace:** Implementa la técnica Pomodoro: 25 minutos de trabajo, 5 minutos de descanso. Al terminar cada sesión, el buzzer emite un tono de aviso.

**Controles:**
- **1 tap** → iniciar / pausar el temporizador.
- **3+ taps** → reiniciar desde 25:00.

---

### Modo 4 — Test de micrófono

**Qué hace:** Muestra en tiempo real una barra de volumen basada en la amplitud captada por el INMP441. Sirve para verificar que el micrófono esté bien conectado.

**Cómo interpretarlo:**
- Barra vacía en silencio → micrófono funcionando correctamente.
- Barra siempre llena → revisa la conexión del pin L/R (debe ir a GND).
- Barra que no se mueve → verifica los pines SCK (GPIO 4), WS (GPIO 5) y SD (GPIO 3).

---

---

## Sección 4 — Retos opcionales

Estos retos están diseñados para quienes terminaron antes o quieren explorar más.

---

### 🟢 Reto 1 — Nueva melodía al tocar (Fácil)

**Objetivo:** Reemplazar el tema de Tetris por una melodía que tú elijas.

**Nivel:** ⭐ Fácil

**Prerrequisitos:** Secciones 1 y 2 completadas.

**Pistas:**
1. Busca la función `tetrisTheme()` en `mochi_unified_5.ino`.
2. El array `mel[]` contiene pares `{frecuencia, duración}`. Por ejemplo:
   - Do central = 262 Hz | Re = 294 | Mi = 330 | Fa = 349 | Sol = 392 | La = 440 | Si = 494 | Do alto = 523
3. Agrega tu melodía con el mismo formato de pares.
4. Busca una melodía en línea (hay tablas de canciones famosas en frecuencias Hz).

**Resultado esperado:** Al dar 3 taps rápidos, el robot toca tu melodía.

---

### 🟡 Reto 2 — Nuevo estado de ánimo: Guiño (Medio)

**Objetivo:** Agregar un undécimo estado de ánimo donde el robot guiña un ojo.

**Nivel:** ⭐⭐ Medio

**Prerrequisitos:** Haber leído la Sección 3 (módulo de moods).

**Pistas:**
1. Define una nueva constante: `#define MOOD_WINK 10` y actualiza `#define MOOD_COUNT 11`.
2. En la función `drawEyelidMask()`, agrega un caso para `MOOD_WINK`:
   - El ojo derecho se dibuja normal.
   - El ojo izquierdo se cubre con un rectángulo (como en `MOOD_SLEEPY` pero solo para uno).
3. En `updateEyes()`, en el `switch(currentMood)`, agrega el tamaño de ojos para `MOOD_WINK`.
4. Para probar: cambia `currentMood = MOOD_WINK;` temporalmente en `setup()`.

**Resultado esperado:** El robot puede mostrar un guiño que aparece en la rotación aleatoria de moods.

---

### 🟠 Reto 3 — Animación GIF propia (Fácil-Medio)

**Objetivo:** Agregar una animación GIF personalizada al robot.

**Nivel:** ⭐ Fácil (si el GIF ya está listo) / ⭐⭐ Medio (si debes crearlo desde cero)

**Prerrequisitos:** Sección 2 completada.

**Pistas:**
1. Elige o crea un GIF animado de 128×64 píxeles en blanco y negro.
2. Usa `herramientas/convertidor_gifs/gif.exe` para convertirlo a formato `.bin`.
3. Copia el archivo a `03_firmware/mochi_unified_5/data/`.
4. Vuelve a subir el filesystem (paso 2.5).

**Resultado esperado:** Tu animación aparece en la rotación del Modo 1.

---

### 🔴 Reto 4 — Contador de aplausos (Difícil)

**Objetivo:** Crear un **Modo 5** que cuente cuántas veces se aplaude usando el micrófono y muestre el contador en pantalla.

**Nivel:** ⭐⭐⭐ Difícil

**Prerrequisitos:** Secciones 1, 2 y 3 completadas. Entender cómo funciona `leerAmplitud()`.

**Pistas:**
1. Agrega `#define NUM_MODES 6` y un nuevo estado `static int clapCount = 0;` en el bloque del Modo 5.
2. Lee la amplitud con `leerAmplitud()`. Un aplauso es un pico rápido seguido de silencio.
3. Define un umbral (ej: `if (amp > 20000)`) y un tiempo de rebote (ej: no contar dos aplausos en menos de 500 ms).
4. Muestra `clapCount` en la pantalla con `display.print()`.
5. Un tap corto puede reiniciar el contador.

**Resultado esperado:** El robot registra aplausos y muestra un número que sube en pantalla.

---

---

## Resolución de problemas

Guía ampliada → [docs/errores-comunes.md](docs/errores-comunes.md)

### El ESP32 no aparece como puerto COM

| Causa | Solución |
|-------|---------|
| Driver USB no instalado | Instala el driver CH340 o CP2102 (ver Sección 1.4) |
| Cable USB-C solo para carga | Usa un cable que soporte datos |
| Puerto ocupado por otro programa | Cierra el monitor serial o cualquier programa que use el puerto |
| ESP32 en modo bootloader | Mantén pulsado el botón BOOT mientras conectas el cable |

### El firmware no compila

| Error | Causa | Solución |
|-------|-------|---------|
| `Image length ... doesn't fit` | Partición incorrecta | Usa `PartitionScheme=huge_app` en el FQBN |
| `fatal error: Adafruit_SH110X.h not found` | Librería no instalada | `arduino-cli lib install "Adafruit SH110X"` |
| `fatal error: U8g2lib.h not found` | Librería no instalada | `arduino-cli lib install "U8g2"` |
| `Error: 13 FQBN not recognized` | FQBN incorrecto o core no instalado | Verifica `esp32:esp32:esp32c6` y reinstala el core |

### El firmware no sube

| Error | Causa | Solución |
|-------|-------|---------|
| `Could not open COM3, the port is busy` | Monitor serial abierto | Cierra el monitor serial y vuelve a subir |
| `A fatal error occurred: Failed to connect` | ESP32 no en modo upload | Presiona BOOT en el ESP32 mientras ejecutas el upload |
| `esptool.py not found` | esptool no instalado | Viene incluido con el core `esp32:esp32`; reinstala el core |

### La pantalla OLED no enciende

- Verifica SDA en **GPIO 6** y SCL en **GPIO 7**.
- Verifica que VCC esté en **3.3V** (no 5V).
- Ejecuta `prueba_pantalla` para diagnosticar la pantalla de forma aislada.
- Comprueba la dirección I2C con un scanner I2C.

### Los GIFs no se reproducen

- La carpeta `data/` no fue subida al filesystem. Repite el paso 2.5.
- En el monitor serial verás `LittleFS: error. Sube los .bin primero.`

### El reloj no sincroniza

- Verifica las credenciales WiFi en `mochi_unified_5.ino` (paso 2.1).
- El robot necesita estar en la misma red WiFi durante el encendido.
- En el monitor serial verás si la conexión falla.

### El micrófono no responde

- Verifica que el pin **L/R** del INMP441 esté conectado a **GND**.
- Verifica WS → GPIO 5, SCK → GPIO 4, SD → GPIO 3.
- Ejecuta `prueba_sonido_microfono` para probar el micrófono de forma aislada.

### El buzzer no suena o suena mal

- Asegúrate de que es un buzzer **pasivo**. Los activos no permiten cambiar el tono.
- Verifica que está en **GPIO 19** con el positivo (+) en el GPIO y el negativo (−) en GND.
- Ejecuta `prueba_buzzer` para diagnosticar el buzzer.

---

## Estructura del repositorio

```
Mochi-Robot/
│
├── README.md                        ← Punto de entrada del taller
│
├── 01_hardware/                     ← Hardware: componentes y conexiones
│   └── README.md
│
├── 02_software/  ◄─ SECCIÓN 1 ─────── Preparación del entorno
│   ├── README.md                    ← Guía: instalar Arduino CLI, librerías, driver USB
│   └── ArduinoCLI_tutorial.md       ← Referencia extendida de comandos
│
├── 03_firmware/  ◄─ SECCIÓN 2 ─────── Carga del firmware
│   ├── README.md                    ← Guía: compilar, subir y verificar el robot
│   ├── mochi_unified_5/ ◄─ SECCIÓN 3 ─ Funciones del robot
│   │   ├── README.md                ← Guía: módulos, modos y personalización
│   │   ├── mochi_unified_5.ino      ← Código fuente principal
│   │   └── data/                   ← Animaciones GIF (.bin) para LittleFS
│   ├── pruebas/                     ← Sketches ordenados para verificar hardware
│   │   ├── 01_prueba_pantalla/
│   │   ├── 02_prueba_touch/
│   │   ├── 03_prueba_buzzer/
│   │   ├── 04_prueba_sonido_beta/
│   │   ├── 05_prueba_wifi/
│   │   ├── 06_prueba_littlefs/
│   │   ├── 07_prueba_conexion_microfono/
│   │   ├── 08_prueba_sonido_microfono/
│   │   ├── 09_transcripcion/        ← Avanzado
│   │   └── 10_codigo_funcional_transcripcion/  ← Avanzado
│   ├── retos/    ◄─ SECCIÓN 4 ─────── Retos opcionales
│   │   ├── README.md                ← Índice de retos
│   │   ├── reto-1-melodia/          ← .ino listo con TODO
│   │   ├── reto-2-mood-wink/        ← Snippets exactos para agregar
│   │   ├── reto-3-gif-propio/       ← Guía de conversión de GIF
│   │   └── reto-4-aplausos/         ← .ino standalone para calibrar
│   └── referencia/                  ← Firmware v4 (ESP32 clásico, solo consulta)
│
├── docs/                            ← Diagramas de arquitectura y errores comunes
├── herramientas/convertidor_gifs/   ← Convierte GIFs al formato .bin
└── recursos/gifs/                   ← 99 animaciones GIF disponibles
```

---

## Recursos adicionales

### Guías del taller (por carpeta)

| Carpeta | README | Contenido |
|---------|--------|-----------|
| [01_hardware/](01_hardware/) | [README.md](01_hardware/README.md) | Lista de componentes y conexiones |
| [02_software/](02_software/) | [README.md](02_software/README.md) | **Sección 1** — entorno, Arduino CLI, librerías |
| [03_firmware/](03_firmware/) | [README.md](03_firmware/README.md) | **Sección 2** — compilar, cargar y verificar |
| [03_firmware/mochi_unified_5/](03_firmware/mochi_unified_5/) | [README.md](03_firmware/mochi_unified_5/README.md) | **Sección 3** — módulos, modos, personalización |
| [03_firmware/retos/](03_firmware/retos/) | [README.md](03_firmware/retos/README.md) | **Sección 4** — 4 retos con código starter |
| [docs/arquitectura.md](docs/arquitectura.md) | — | Diagramas Mermaid de arquitectura |
| [docs/errores-comunes.md](docs/errores-comunes.md) | — | Guía ampliada de resolución de errores |

### Referencia adicional

| Recurso | Descripción |
|---------|-------------|
| [02_software/ArduinoCLI_tutorial.md](02_software/ArduinoCLI_tutorial.md) | Tutorial extendido de Arduino CLI |
| [03_firmware/mochi_unified_5/README.md](03_firmware/mochi_unified_5/README.md) | Documentación técnica del código del robot |

---

*Repositorio mantenido por el equipo del taller. ¿Encontraste un error o tienes una mejora? Abre un issue o un pull request.*
