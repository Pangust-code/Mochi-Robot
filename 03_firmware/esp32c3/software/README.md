# Firmware principal — ESP32-C3 Super Mini

> **Prerrequisito:** [pruebas/](../pruebas/) — hardware verificado componente por componente.

Esta carpeta contiene el firmware unificado de Mochi para el **ESP32-C3 Super Mini**. El sketch `software.ino` integra todos los modos de operación: mascota, GIFs, reloj, micrófono y más.

---

## Contenido de esta carpeta

```
software/
├── software.ino      ← Sketch principal — este es el que subes
└── data/             ← Animaciones GIF (.bin) para el Modo 1
```

---

## Conexiones de hardware

| Componente | Pin del componente | GPIO C3 |
|---|---|---|
| **OLED SH1106** | SDA | GPIO 8 |
| **OLED SH1106** | SCL | GPIO 9 |
| **Buzzer pasivo** | Signal | GPIO 10 |
| **Touch TTP223** | OUT | GPIO 2 |
| **INMP441** | SCK | GPIO 4 |
| **INMP441** | WS | GPIO 1 |
| **INMP441** | SD | GPIO 3 |
| **INMP441** | L/R | GND |
| **Todos** | VCC | 3.3V |
| **Todos** | GND | GND |

---

## Paso 1 — Verificar el hardware

Antes de cargar el firmware completo, confirma que todos los componentes responden correctamente:

> Guía detallada con tablas de registro de valores: [../pruebas/README.md](../pruebas/README.md)

| # | Sketch | ¿Qué verifica? |
|---|--------|----------------|
| 01 | [`pruebas/01_prueba_pantalla/`](../pruebas/01_prueba_pantalla/) | Pantalla OLED (I2C, texto, gráficos) |
| 02 | [`pruebas/02_prueba_touch/`](../pruebas/02_prueba_touch/) | Sensor táctil TTP223 |
| 03 | [`pruebas/03_prueba_buzzer/`](../pruebas/03_prueba_buzzer/) | Buzzer pasivo (sweep + escala + tonos) |
| 05 | [`pruebas/05_prueba_conexion_microfono/`](../pruebas/05_prueba_conexion_microfono/) | Pines físicos del INMP441 (I2S) |
| 06 | [`pruebas/06_diagnostico_hardware/`](../pruebas/06_diagnostico_hardware/) | Diagnóstico integral de todos los componentes |
| 08 | [`pruebas/08_prueba_wifi/`](../pruebas/08_prueba_wifi/) | Conexión WiFi e IP asignada |
| 10 | [`pruebas/10_prueba_sonido_microfono/`](../pruebas/10_prueba_sonido_microfono/) | Captura de audio y niveles de amplitud |

**Subir un sketch de prueba** (reemplaza `01_prueba_pantalla` por el que corresponda):

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/01_prueba_pantalla
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

> Los sketches de prueba son pequeños — **no necesitan** `PartitionScheme=huge_app`.

---

## Paso 2 — Configurar el WiFi (solo si vas a usar el Modo Reloj)

Abre [`software.ino`](software.ino) y edita el arreglo `networks[]` al inicio del archivo:

```cpp
WiFiCredential networks[] = {
  // WPA2-Personal (red doméstica)
  {"TU_RED_WIFI",       "TU_CONTRASEÑA",   false, "", ""},
  // WPA2-Enterprise (red universitaria — opcional)
  {"RED_UNIVERSITARIA", "",                true,
   "usuario@universidad.edu", "contraseña_eap"},
};
```

> Sin WiFi configurado, los modos 0, 1, 3 y 4 funcionan con normalidad. Solo el **Modo 2 (Reloj)** requiere conexión.

---

## Paso 3 — Compilar el firmware principal

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app 03_firmware/esp32c3/software
```

> `PartitionScheme=huge_app` es **obligatorio** — el firmware completo no cabe en la partición por defecto.

Compilación exitosa:
```
Sketch uses XXXXXX bytes (XX%) of program storage space.
```

---

## Paso 4 — Subir el firmware principal

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app 03_firmware/esp32c3/software
```

Subida exitosa:
```
Hash of data verified.
Hard resetting via RTS pin...
```

---

## Paso 5 — Subir las animaciones GIF (LittleFS)

Las animaciones del Modo 1 se almacenan como archivos `.bin` en la flash del ESP32 y deben subirse por separado. Los archivos están en [`data/`](data/).

### Instalar la extensión arduino-littlefs-upload (una sola vez)

1. Descarga **[arduino-littlefs-upload-1.5.4.vsix](https://github.com/earlephilhower/arduino-littlefs-upload/releases/download/1.5.4/arduino-littlefs-upload-1.5.4.vsix)**
2. En VS Code: panel **Extensiones** (`Ctrl+Shift+X`) → `···` → **Instalar desde VSIX...**
3. Selecciona el archivo descargado → reinicia VS Code.

### Subir los archivos

1. Cierra el Monitor Serie si está abierto.
2. `Ctrl+Shift+P` → **Upload LittleFS to Pico/ESP8266/ESP32**

**Opción alternativa — esptool:**
```powershell
mklittlefs -c 03_firmware/esp32c3/software/data -p 256 -b 4096 -s 0x150000 littlefs.bin
esptool.py --chip esp32c3 --port COM3 write_flash 0x290000 littlefs.bin
```

> Si omites este paso, el Modo 1 (GIFs) mostrará error pero los demás modos funcionarán.

---

## Paso 6 — Verificar con el Monitor Serie

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

Salida esperada al iniciar correctamente:
```
=== MOCHI ===
LittleFS OK — 42/128 KB
I2S: inicializado
-> Modo 0 (Mascota)
```

Si ves `LittleFS: error` → repite el Paso 5.

---

## Modos de operación

| Modo | Nombre | Cómo acceder | Descripción |
|------|--------|-------------|-------------|
| 0 | Mascota | Inicio | Ojos animados. 1 tap = cambio de mood. 3 taps = melodía. |
| 1 | GIFs | Hold desde Modo 0 | Reproduce animaciones `.bin` de LittleFS en bucle. |
| 2 | Reloj | Hold desde Modo 1 | Hora sincronizada por NTP (requiere WiFi). |
| 3 | Micrófono | Hold desde Modo 2 | Barra de volumen reactiva al sonido ambiente. |
| 4 | Clima | Hold desde Modo 3 | Temperatura y condición actual (requiere WiFi + API key). |

> **Navegar entre modos:** mantén el sensor táctil ≥ 800 ms para avanzar al modo siguiente.

---

## Checklist

- [ ] Sketch `06_diagnostico_hardware` reporta todos los componentes OK
- [ ] OLED muestra texto correctamente (SDA → GPIO 8, SCL → GPIO 9)
- [ ] Touch TTP223 detecta toque (GPIO 2)
- [ ] Buzzer emite tono (GPIO 10)
- [ ] INMP441 captura audio sin errores I2S (SCK→4, WS→1, SD→3)
- [ ] Compilación con `PartitionScheme=huge_app` termina sin errores
- [ ] Subida termina con "Hard resetting via RTS pin"
- [ ] Monitor Serie muestra `=== MOCHI ===` y `LittleFS OK`
- [ ] OLED muestra ojos animados al encender
- [ ] 1 tap cambia el estado de ánimo y se escucha un beep
- [ ] Hold largo (≥ 800 ms) cambia de modo

---

## ¿Algo no funciona?

Consulta la [guía de errores comunes](../../../docs/errores-comunes.md).

---

**← Anterior:** [pruebas/](../pruebas/) — diagnóstico de hardware
**Siguiente →** [retos/](../retos/) — personalización del robot
