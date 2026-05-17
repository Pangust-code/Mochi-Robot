# 03 — Firmware

El firmware es el programa que corre dentro del ESP32 y hace que Mochi funcione. Aquí encontrarás las versiones disponibles y cuál debes usar.

---

## ¿Qué versión usar?

| Carpeta | Versión | Estado | Microcontrolador |
|---------|---------|--------|-----------------|
| [mochi_unified_5/](mochi_unified_5/) | v6 | ✅ **Actual** — usa esta | ESP32-C6 Supermini |
| [referencia/](referencia/) | v4 | 📦 Referencia histórica | ESP32 clásico (38 pines) |

> **Usa `mochi_unified_5`**. Es la versión más completa y la que corresponde al hardware de la guía.

---

## Diferencias entre versiones

| Característica | v6 (actual) | v4 (referencia) |
|---------------|-------------|-----------------|
| Modos | 5 modos | 3 modos |
| Cambio de modo | Hold táctil | Botón físico |
| Manejo de toque | Basado en eventos (sin bugs) | Basado en delay |
| Bluetooth | No | Sí (comandos Pomodoro) |
| Animaciones GIF | Sí (14 archivos) | No |
| Micrófono | INMP441 I2S | INMP441 I2S |

---

## Siguiente paso

Lee el README dentro de [mochi_unified_5/](mochi_unified_5/) para entender el código antes de subirlo.
