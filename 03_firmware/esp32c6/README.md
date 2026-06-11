# Sección 2 — Carga del firmware

> **Prerrequisito:** [02_software/](../02_software/) — Arduino CLI y librerías instaladas.

Esta carpeta contiene todo el código del robot. Aquí compilarás y cargarás el firmware, verificarás cada componente con los sketches de prueba, y accederás a los retos de personalización.

---

## Contenido de esta carpeta

```
03_firmware/
├── mochi_unified_5/              ← Firmware principal — usa este
│   ├── mochi_unified_5.ino
│   └── data/                     ← Animaciones GIF (.bin) para el Modo 1
├── pruebas/                      ← Sketches para verificar el hardware
│   ├── 01_prueba_pantalla/
│   ├── 02_prueba_touch/
│   ├── 03_prueba_buzzer/
│   ├── 04_prueba_sonido_beta/
│   ├── 05_prueba_wifi/
│   ├── 06_prueba_littlefs/
│   ├── 07_prueba_conexion_microfono/
│   ├── 08_prueba_sonido_microfono/
│   ├── 09_transcripcion/         ← Avanzado: Whisper + servidor local
│   ├── 10_codigo_funcional_transcripcion/  ← Avanzado: pipeline IoT completo
│   └── 11_prueba_canvas/         ← Avanzado: canvas experimental
├── retos/                        ← Personalización del robot (Sección 4)
│   ├── reto-1-melodia/
│   ├── reto-2-mood-wink/
│   ├── reto-3-gif-propio/
│   ├── reto-4-aplausos/
│   ├── reto-5-chromedino/        ← Easter Egg: Chrome Dino en OLED
│   └── reto-6-flappybird/        ← Easter Egg: Flappy Bird en OLED
└── referencia/                   ← Firmware v4 (ESP32 clásico, solo consulta)
```

---

## ¿Qué versión del firmware usar?

| Carpeta | Versión | Estado | Microcontrolador |
|---------|---------|--------|-----------------|
| [`mochi_unified_5/`](mochi_unified_5/) | v6 | ✅ **Usa esta** | ESP32-C6 Supermini |
| [`referencia/`](referencia/) | v4 | 📦 Solo consulta | ESP32 clásico (38 pines) |

---

## Paso 1 — Verificar el hardware con los sketches de prueba

Antes de cargar el firmware completo, verifica que cada componente está bien conectado. Los sketches están numerados — síguelos en orden.

> Guía detallada con tablas de registro de valores: [pruebas/README.md](pruebas/README.md)

### Sketches del taller (01–08)

| # | Sketch | ¿Qué verifica? | Fase del taller |
|---|--------|----------------|----------------|
| 01 | [`01_prueba_pantalla/`](pruebas/01_prueba_pantalla/) | Pantalla OLED (I2C, texto, gráficos) | Fase 1 |
| 02 | [`02_prueba_touch/`](pruebas/02_prueba_touch/) | Sensor táctil TTP223 | Fase 2 |
| 03 | [`03_prueba_buzzer/`](pruebas/03_prueba_buzzer/) | Buzzer pasivo (sweep + escala + tonos) | Opcional |
| 04 | [`04_prueba_sonido_beta/`](pruebas/04_prueba_sonido_beta/) | Buzzer básico con `ledcWriteTone` | Opcional |
| 05 | [`05_prueba_wifi/`](pruebas/05_prueba_wifi/) | Conexión WiFi e IP asignada | Fase 1 |
| 06 | [`06_prueba_littlefs/`](pruebas/06_prueba_littlefs/) | Sistema de archivos LittleFS | Opcional |
| 07 | [`07_prueba_conexion_microfono/`](pruebas/07_prueba_conexion_microfono/) | Pines físicos del INMP441 (I2S) | Fase 2 |
| 08 | [`08_prueba_sonido_microfono/`](pruebas/08_prueba_sonido_microfono/) | Captura de audio y niveles de amplitud | Fase 2 |

### Sketches avanzados (09–11) — integración con servidor

| # | Sketch | Para qué sirve |
|---|--------|----------------|
| 09 | [`09_transcripcion/`](pruebas/09_transcripcion/) | Graba audio PCM16 y transcribe con Whisper (servidor Python local) |
| 10 | [`10_codigo_funcional_transcripcion/`](pruebas/10_codigo_funcional_transcripcion/) | Pipeline IoT completo: PCM16 → servidor → `mood_id` + intención en pantalla |
| 11 | [`11_prueba_canvas/`](pruebas/11_prueba_canvas/) | Prueba experimental de canvas gráfico |

**Subir un sketch de prueba** (reemplaza `01_prueba_pantalla` por el que corresponda):

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

> Los sketches de prueba son pequeños — **no necesitan** `PartitionScheme=huge_app`.

---

## Paso 2 — Configurar el WiFi (solo si vas a usar el Modo Reloj)

Abre [`mochi_unified_5/mochi_unified_5.ino`](mochi_unified_5/mochi_unified_5.ino) y edita las credenciales en la sección de configuración al inicio del archivo:

```cpp
const char* ssid     = "TU_RED_WIFI";      // ← nombre de tu red 2.4 GHz
const char* password = "TU_CONTRASEÑA";   // ← contraseña
```

> Sin WiFi configurado, los modos 0, 1, 3 y 4 funcionan con normalidad. Solo el **Modo 2 (Reloj)** requiere conexión.

---

## Paso 3 — Compilar el firmware principal

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

> `PartitionScheme=huge_app` es **obligatorio** — el firmware completo no cabe en la partición por defecto.

Compilación exitosa:
```
Sketch uses XXXXXX bytes (XX%) of program storage space.
```

---

## Paso 4 — Subir el firmware principal

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

Subida exitosa:
```
Hash of data verified.
Hard resetting via RTS pin...
```

---

## Paso 5 — Subir las animaciones GIF (LittleFS)

Las animaciones del Modo 1 se almacenan como archivos `.bin` en la flash del ESP32 y deben subirse por separado. Los archivos están en [`mochi_unified_5/data/`](mochi_unified_5/data/).

### Instalar la extensión arduino-littlefs-upload (una sola vez)

> Guía en video: [youtube.com/watch?v=vICDKOLizrU](https://www.youtube.com/watch?v=vICDKOLizrU)

1. Descarga **[arduino-littlefs-upload-1.5.4.vsix](https://github.com/earlephilhower/arduino-littlefs-upload/releases/download/1.5.4/arduino-littlefs-upload-1.5.4.vsix)**
2. En VS Code: panel **Extensiones** (`Ctrl+Shift+X`) → `···` → **Instalar desde VSIX...**
3. Selecciona el archivo descargado → reinicia VS Code.

### Subir los archivos

1. Cierra el Monitor Serie si está abierto.
2. `Ctrl+Shift+P` → **Upload LittleFS to Pico/ESP8266/ESP32**

**Opción alternativa — esptool:**
```powershell
mklittlefs -c 03_firmware/mochi_unified_5/data -p 256 -b 4096 -s 0x150000 littlefs.bin
esptool.py --chip esp32c6 --port COM3 write_flash 0x290000 littlefs.bin
```

> Si omites este paso, el Modo 1 (GIFs) mostrará error pero los demás modos funcionarán.

---

## Paso 6 — Verificar con el Monitor Serie

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

Salida esperada al iniciar correctamente:
```
=== MOCHI v6 ===
LittleFS OK — 42/128 KB
I2S: inicializado
-> Modo 0 (Mascota)
```

Si ves `LittleFS: error` → repite el Paso 5.

**Con VS Code:** usa las tareas preconfiguradas desde **Terminal → Run Task**.

---

## Checklist — Sección 2

- [ ] Sketch `01_prueba_pantalla` muestra texto y gráficos en la OLED
- [ ] Sketch `05_prueba_wifi` muestra una IP en el Monitor Serie
- [ ] Sketch `02_prueba_touch` detecta correctamente el sensor táctil
- [ ] Sketch `07_prueba_conexion_microfono` no reporta error de pines I2S
- [ ] Sketch `08_prueba_sonido_microfono` muestra valores de amplitud variables con el sonido
- [ ] La compilación del firmware principal termina sin errores (`PartitionScheme=huge_app`)
- [ ] La subida termina con "Hard resetting via RTS pin"
- [ ] El Monitor Serie muestra `=== MOCHI v6 ===` y `LittleFS OK`
- [ ] La pantalla OLED muestra ojos animados al encender
- [ ] 1 tap cambia el estado de ánimo y se escucha un beep
- [ ] Hold largo (≥ 800 ms) cambia de modo

---

## ¿Algo no funciona?

Consulta la [guía de errores comunes](../docs/errores-comunes.md).

---

**← Anterior:** [02_software/](../02_software/) — preparación del entorno
**Siguiente →** [pruebas/](pruebas/) — diagnóstico de hardware (Fases 1 y 2)
