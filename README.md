# Mochi Robot — Guía para Estudiantes

Mochi es un robot de compañía (pet robot) basado en ESP32 con ojos animados en pantalla OLED, sensor táctil, micrófono y buzzer. Este repositorio es la guía paso a paso para que puedas armarlo y programarlo desde cero.

---

## ¿Qué vas a construir?

Un pequeño robot con pantalla OLED que:
- Tiene **ojos animados** con 10 estados de ánimo (feliz, enojado, somnoliento, enamorado…)
- Reacciona al **tacto** y a los **sonidos del ambiente**
- Muestra la **hora en tiempo real** (sincronizada por WiFi)
- Incluye un **temporizador Pomodoro** para estudiar
- Reproduce **animaciones GIF** almacenadas en memoria

---

## Ruta de aprendizaje

Sigue las carpetas en orden:

| Paso | Carpeta | Contenido |
|------|---------|-----------|
| 1 | [01_hardware/](01_hardware/) | Lista de componentes y cómo conectarlos |
| 2 | [02_software/](02_software/) | Configurar Arduino CLI y VS Code |
| 3 | [03_firmware/](03_firmware/) | El código del robot y cómo subirlo |
| 4 | [04_pagina_web/](04_pagina_web/) | *(En desarrollo)* Interfaz web del robot |

---

## Requisitos previos

- Computadora con Windows, Mac o Linux
- Cable USB-C
- Conocimientos básicos de Arduino (variables, funciones, `setup()`/`loop()`)
- Ganas de aprender

---

## Hardware necesario

| Componente | Cantidad |
|-----------|----------|
| ESP32-C6 Supermini | 1 |
| Pantalla OLED SH1106 128×64 (I2C) | 1 |
| Sensor táctil TTP223 | 1 |
| Micrófono INMP441 (I2S) | 1 |
| Buzzer pasivo | 1 |
| Protoboard + cables dupont | 1 set |

> Detalle de conexiones en [01_hardware/](01_hardware/).

---

## Vista rápida de modos

| Modo | Activar | Descripción |
|------|---------|-------------|
| 🐾 Mascota | Por defecto | Ojos animados que reaccionan al tacto y al sonido |
| 🎞 GIFs | Hold largo → modo 1 | Animaciones almacenadas en memoria |
| 🕐 Reloj | Hold largo → modo 2 | Hora y fecha en tiempo real (WiFi) |
| 🍅 Pomodoro | Hold largo → modo 3 | Timer 25/5 min — 1 toque = iniciar/pausar |
| 🎙 Micrófono | Hold largo → modo 4 | Visualizador de volumen en tiempo real |

> Para cambiar de modo: mantén presionado el sensor táctil hasta escuchar un tono.
