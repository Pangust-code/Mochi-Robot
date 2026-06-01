# Sección 2 — Carga del firmware

> **Prerrequisito:** [02_software/](../02_software/) — Arduino CLI y librerías instaladas.

Esta carpeta contiene todo el código del robot. Aquí compilarás y cargarás el firmware, verificarás cada componente con los sketches de prueba, y accederás a los retos de personalización.

---

## Contenido de esta carpeta

```
03_firmware/
├── mochi_unified_5/     ← Firmware principal — usa este
│   ├── mochi_unified_5.ino
│   └── data/            ← Animaciones GIF (.bin) para el Modo 1
├── pruebas/             ← Sketches para verificar el hardware (seguir en orden)
│   ├── 01_prueba_pantalla/
│   ├── 02_prueba_touch/
│   ├── 03_prueba_buzzer/
│   ├── 04_prueba_sonido_beta/
│   ├── 05_prueba_wifi/
│   ├── 06_prueba_littlefs/
│   ├── 07_prueba_conexion_microfono/
│   └── 08_prueba_sonido_microfono/
├── retos/               ← Personalización del robot (Sección 4)
└── referencia/          ← Versión anterior del firmware (solo consulta)
```

---

## ¿Qué versión del firmware usar?

| Carpeta | Versión | Estado | Microcontrolador |
|---------|---------|--------|-----------------|
| [`mochi_unified_5/`](mochi_unified_5/) | v6 | ✅ **Usa esta** | ESP32-C6 Supermini |
| [`referencia/`](referencia/) | v4 | 📦 Solo consulta | ESP32 clásico (38 pines) |

---

## Paso 1 — Verificar el hardware con los sketches de prueba

Antes de cargar el firmware completo, verifica que cada componente está bien conectado. Los sketches están numerados — síguelos en orden:

| # | Sketch | ¿Qué verifica? |
|---|--------|----------------|
| 01 | [`01_prueba_pantalla/`](pruebas/01_prueba_pantalla/) | Pantalla OLED (I2C, texto, gráficos) |
| 02 | [`02_prueba_touch/`](pruebas/02_prueba_touch/) | Sensor táctil TTP223 |
| 03 | [`03_prueba_buzzer/`](pruebas/03_prueba_buzzer/) | Buzzer pasivo (sweep + escala + tonos del firmware) |
| 04 | [`04_prueba_sonido_beta/`](pruebas/04_prueba_sonido_beta/) | Buzzer básico con `ledcWriteTone` |
| 05 | [`05_prueba_wifi/`](pruebas/05_prueba_wifi/) | Conexión WiFi e IP asignada |
| 06 | [`06_prueba_littlefs/`](pruebas/06_prueba_littlefs/) | Sistema de archivos LittleFS |
| 07 | [`07_prueba_conexion_microfono/`](pruebas/07_prueba_conexion_microfono/) | Pines físicos del INMP441 |
| 08 | [`08_prueba_sonido_microfono/`](pruebas/08_prueba_sonido_microfono/) | Captura de audio en tiempo real |

**Subir un sketch de prueba** (reemplaza `01_prueba_pantalla` por el número correspondiente):
```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
```

> Los sketches de prueba son pequeños — **no necesitan** `PartitionScheme=huge_app`.

---

## Paso 2 — Configurar el WiFi (solo si vas a usar el Modo Reloj)

Abre [`mochi_unified_5/mochi_unified_5.ino`](mochi_unified_5/mochi_unified_5.ino) y edita las líneas 57–58:

```cpp
const char* ssid     = "TU_RED_WIFI";      // ← nombre de tu red
const char* password = "TU_CONTRASEÑA";   // ← contraseña
```

> Sin WiFi configurado, los modos 0, 1, 3 y 4 funcionan con normalidad.

---

## Paso 3 — Compilar el firmware principal

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

> `PartitionScheme=huge_app` es **obligatorio** para el firmware principal — el binario es grande y no cabe en la partición por defecto.

✅ Compilación exitosa:
```
Sketch uses XXXXXX bytes (XX%) of program storage space.
```

---

## Paso 4 — Subir el firmware principal

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

✅ Subida exitosa:
```
Hash of data verified.
Hard resetting via RTS pin...
```

---

## Paso 5 — Subir las animaciones GIF (LittleFS)

Las animaciones del Modo 1 se almacenan en la flash del ESP32 como archivos `.bin` y deben subirse por separado. Los archivos están en [`mochi_unified_5/data/`](mochi_unified_5/data/).

**Opción A — Plugin Arduino IDE:**
1. Instala el plugin **ESP32 LittleFS Data Upload**.
2. Abre el sketch en Arduino IDE.
3. **Herramientas → ESP32 LittleFS Data Upload**.

**Opción B — esptool:**
```powershell
mklittlefs -c 03_firmware/mochi_unified_5/data -p 256 -b 4096 -s 0x150000 littlefs.bin
esptool.py --chip esp32c6 --port COM3 write_flash 0x290000 littlefs.bin
```

> Si omites este paso, el Modo 1 (GIFs) mostrará error pero los demás modos funcionarán.

---

## Paso 6 — Verificar con el monitor serial

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

✅ Salida esperada al iniciar:
```
=== MOCHI v6 ===
LittleFS OK — 42/128 KB
I2S: inicializado
-> Modo 0 (Mascota)
```

**Con VS Code:** `Ctrl+Shift+B` para compilar, o **Terminal → Run Task** para ver todas las tareas.

---

## ✅ Checklist — Sección 2

- [ ] Todos los sketches de prueba 01–08 se ejecutan correctamente
- [ ] La compilación del firmware principal termina sin errores
- [ ] La subida termina con "Hard resetting via RTS pin"
- [ ] El monitor serial muestra `=== MOCHI v6 ===`
- [ ] La pantalla OLED muestra ojos animados
- [ ] Al tocar el sensor se escucha un beep y los ojos cambian
- [ ] Al hacer hold (≥ 800 ms) se escucha otro beep y el modo cambia

---

## Sketches avanzados (integración con servidor)

| # | Sketch | Para qué sirve |
|---|--------|----------------|
| 09 | [`09_transcripcion/`](pruebas/09_transcripcion/) | Graba audio y transcribe con Whisper (servidor Python local) |
| 10 | [`10_codigo_funcional_transcripcion/`](pruebas/10_codigo_funcional_transcripcion/) | Integración completa: PCM16 → servidor → mood_id + intent en pantalla |

---

## ¿Algo no funciona?

Consulta la [guía de errores comunes](../docs/errores-comunes.md).

---

**← Anterior:** [02_software/](../02_software/) — preparación del entorno
**Siguiente →** [mochi_unified_5/](mochi_unified_5/) — entender las funciones del robot
