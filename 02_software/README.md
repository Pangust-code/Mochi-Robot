# 02 — Software y entorno de programación

En esta sección configurarás todo lo necesario para compilar y subir el firmware al ESP32 desde VS Code, sin necesidad de instalar el IDE de Arduino.

---

## Herramientas que vas a instalar

| Herramienta | Para qué sirve |
|-------------|----------------|
| **Arduino CLI** | Compilar y subir código desde la terminal |
| **VS Code** | Editor de código con tareas preconfiguradas |
| **Extensión Arduino** (opcional) | Resaltado de sintaxis `.ino` |

---

## Tutorial completo

Sigue el tutorial paso a paso:

👉 [ArduinoCLI_tutorial.md](ArduinoCLI_tutorial.md)

El tutorial cubre:
1. Instalación de Arduino CLI con `winget`
2. Instalación del core de ESP32
3. Instalación de librerías necesarias
4. Compilación con la partición `huge_app` (requerida por el tamaño del firmware)
5. Subida al ESP32 y monitor serial
6. Solución de errores comunes

---

## Tareas de VS Code (ya configuradas)

El repositorio incluye [`.vscode/tasks.json`](../.vscode/tasks.json) con las siguientes tareas listas para usar:

| Tarea | Atajo |
|-------|-------|
| Compilar ESP32 | `Ctrl+Shift+B` → seleccionar tarea |
| Subir al ESP32 | Tarea "Upload to ESP32" |
| Monitor serial (115200) | Tarea "Monitor Serial" |
| Compilar + subir | Tarea combinada |

> Asegúrate de cambiar el puerto COM en `tasks.json` al puerto que aparezca en tu computadora al conectar el ESP32.

---

## Librerías necesarias

Instálalas con Arduino CLI (el tutorial explica cómo):

```
Adafruit GFX Library
Adafruit SH110X
U8g2
```

---

## Siguiente paso

Con el entorno listo, continúa con [03_firmware/](../03_firmware/) para entender el código y subirlo al robot.
