# Firmware — ESP32-C3 Super Mini

> **Prerrequisito:** [02_software/](../02_software/) — Arduino CLI y librerías instaladas.

Esta carpeta contiene el firmware y los sketches de prueba para el **ESP32-C3 Super Mini**. Sigue el orden de las secciones para tener Mochi funcionando en menos de una hora.

---

## Conexiones de hardware

| Componente | Pin del componente | GPIO del C3 |
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

## Contenido de esta carpeta

```
esp32c3/
├── software/                           ← Firmware principal
│   ├── software.ino                    ← Sketch unificado — este es el que subes
│   └── data/                           ← Animaciones GIF (.bin) para el Modo 1
├── pruebas/                            ← Sketches de diagnóstico por componente
│   ├── 01_prueba_pantalla/
│   ├── 02_prueba_touch/
│   ├── 03_prueba_buzzer/
│   ├── 04_prueba_sonido_beta/
│   ├── 05_prueba_conexion_microfono/
│   ├── 06_diagnostico_hardware/
│   ├── 07_prueba_littlefs/
│   ├── 08_prueba_wifi/
│   ├── 09_prueba_clima/
│   ├── 10_prueba_sonido_microfono/
│   ├── 11_transcripcion/
│   ├── 12_codigo_funcional_transcripcion/
│   └── 13_prueba_canvas/
├── retos/                              ← Retos de personalización
│   ├── reto-1-melodia/
│   ├── reto-2-mood-wink/
│   ├── reto-3-gif-propio/
│   ├── reto-4-aplausos/
│   ├── reto-5-chromedino/
│   ├── reto-6-flappybird/
│   ├── reto-7-espejo-emocional/
│   ├── reto-8-alarma-reloj/
│   └── reto-9-bienvenida/
├── diagnostico_hardware_esp32c3.ino    ← Diagnóstico standalone (sin subcarpeta)
├── flappy_bird_esp32c3.ino             ← Easter Egg standalone
└── inmp441_oled_esp32c3.ino            ← Prueba INMP441 + OLED standalone
```

---

## Ruta de aprendizaje

| Paso | Carpeta | Descripción |
|------|---------|-------------|
| 1 | [pruebas/](pruebas/) | Verifica cada componente antes de integrar |
| 2 | [software/](software/) | Carga el firmware unificado |
| 3 | [retos/](retos/) | Personaliza el robot |

---

## Inicio rápido

### 1 — Verificar el hardware

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/06_diagnostico_hardware
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/06_diagnostico_hardware
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

> Si algún componente falla, consulta [pruebas/README.md](pruebas/README.md) antes de continuar.

### 2 — Compilar y subir el firmware principal

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app 03_firmware/esp32c3/software
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3:PartitionScheme=huge_app 03_firmware/esp32c3/software
```

> `PartitionScheme=huge_app` es **obligatorio** — el firmware completo no cabe en la partición por defecto.

Guía detallada con WiFi, LittleFS y checklist completo → [software/README.md](software/README.md)

---

## ¿Algo no funciona?

Consulta la [guía de errores comunes](../../docs/errores-comunes.md).

---

**← Anterior:** [02_software/](../02_software/) — preparación del entorno
**Siguiente →** [esp32c3/pruebas/](pruebas/) — diagnóstico de hardware
