# 🔈 Suite de Pruebas de Hardware: ESP32-C6 + Buzzer Pasivo + OLED + TTP223

Este repositorio contiene la herramienta de diagnóstico auditivo y visual diseñada para verificar la correcta integración de un **Buzzer Pasivo**, una pantalla **OLED SH1106** y un **sensor capacitivo TTP223**. El objetivo principal es asegurar la capacidad del sistema para generar retroalimentación sonora dinámica (tonos variables) antes de la integración del firmware `mochi_unified_5` [cite: 83].

---

## 🛠️ Arquitectura de Hardware y Pinout

Asegúrate de conectar los componentes físicos de acuerdo con la siguiente tabla para que el script funcione correctamente [cite: 86]:

| Componente | Pin Periférico | Pin ESP32-C6 | Función Lógica | Descripción |
| :--- | :--- | :--- | :--- | :--- |
| **Buzzer** | SIG / PWM | **GPIO 19** | Salida PWM | Generación de frecuencias (sonido) [cite: 86]. |
| **Buzzer** | GND | GND | Tierra | Referencia común de potencial. |
| **OLED SH1106** | SDA | **GPIO 6** | Bus Datos I2C | Transmisión de datos de pantalla [cite: 86]. |
| **OLED SH1106** | SCL | **GPIO 7** | Bus Reloj I2C | Sincronización de pantalla [cite: 86]. |
| **TTP223 Touch** | SIG | **GPIO 2** | Entrada Digital | Lectura de interacciones [cite: 86]. |

---

## 📂 Desglose de los Módulos de Diagnóstico (`prueba_buzzer.ino`)

Este script se divide en dos fases: una secuencia automática de diagnóstico de hardware y un modo interactivo de retroalimentación en tiempo real.

### 1. ⚙️ Fase de Diagnóstico Automático (Al Encender)

Al iniciar, el sistema ejecuta tres rutinas automáticas críticas para certificar el hardware auditivo [cite: 84]:

* **Test 1 — Sweep (Barrido de Frecuencias):** Sube gradualmente desde 200 Hz hasta 3000 Hz, y luego desciende de regreso a 200 Hz, mostrando la frecuencia en la pantalla [cite: 84, 100, 102, 103, 104].
    * ⚠️ **Validación Crítica:** Si escuchas un tono fijo y constante, significa que estás usando un buzzer ACTIVO. **Debes cambiarlo por un buzzer PASIVO**, ya que los activos no pueden reproducir melodías o tonos variables [cite: 84, 101, 134].
* **Test 2 — Escala Musical:** Reproduce la escala estándar (Do, Re, Mi, Fa, Sol, La, Si, Do) con sus respectivas frecuencias (262 Hz a 523 Hz) para confirmar la precisión tonal [cite: 84, 107, 108, 109].
* **Test 3 — Tonos del Firmware:** Emite una muestra de los sonidos exactos que utilizará el firmware `mochi_unified_5` para acostumbrar al usuario [cite: 83, 84, 113]:
    * Tono de Tap de Ánimo: 800 Hz por 60 ms [cite: 84, 114].
    * Tono de Cambio de Modo: 1000 Hz por 80 ms [cite: 85, 115].
    * Tono Pomodoro Listo: 880 Hz seguido de 1046 Hz [cite: 85, 116, 117].

### 2. 👆 Fase de Modo Interactivo (Máquina de Estados)

Una vez superados los tests, el ESP32-C6 entra en un bucle que vincula directamente las interacciones táctiles del TTP223 con respuestas sonoras, utilizando los mismos umbrales de tiempo del firmware final (`HOLD_MS = 800`, `MULTITAP_MS = 400`) [cite: 86]:

* **TAP Simple (x1):** Reproduce el tono de "ánimo" (800 Hz) [cite: 85, 127].
* **TAP Doble (x2):** Reproduce dos tonos de "ánimo" consecutivos [cite: 128, 129].
* **Multi-TAP (x3+):** Reproduce el tema de Tetris completo, aumentando ligeramente la velocidad en cada repetición [cite: 85, 100, 129].
* **HOLD (Mantenido $\ge$ 800ms):** Reproduce inmediatamente el tono de "cambio de modo" (1000 Hz) sin esperar a que expire la ventana multitap [cite: 85, 123, 124].

### 3. 📺 Interfaz Gráfica Dinámica

Durante todo el proceso, la pantalla OLED SH1106 actúa como un dashboard de depuración. La función `mostrar(titulo, detalle)` actualiza constantemente un título superior, un texto centrado principal (con la acción o la frecuencia actual) y una barra inferior con un recordatorio de los controles interactivos [cite: 89, 90, 91].

---

## 💻 Monitoreo por Puerto Serie

Al abrir el monitor serie a **115200 baudios**, recibirás un log estructurado útil para el diagnóstico [cite: 131]:

```text
========================================
  PRUEBA BUZZER — pasivo GPIO 19
========================================
  Si el tono no cambia en el sweep:
  -> tu buzzer es ACTIVO, debes cambiarlo.
========================================

[TEST 1] Sweep de frecuencias (200 Hz → 3000 Hz → 200 Hz)
         Si el tono no cambia de pitch: es buzzer ACTIVO, no sirve.
         Sweep completo.
[TEST 2] Escala musical (Do Re Mi Fa Sol La Si Do)
         DO  262 Hz
         RE  294 Hz
         ...
[TEST 3] Tonos del firmware mochi_unified_5
         Tono tap de animo (800 Hz, 60 ms)...

Tests automaticos completos.
Modo interactivo:
  TAP x1  → tono tap
  TAP x3+ → Tetris theme
  HOLD    → tono cambio de modo
```

---

## 🚀 Instrucciones de Despliegue Rápido

1. Asegúrate de tener instaladas las librerías `Adafruit_GFX` y `Adafruit_SH110X` [cite: 86].
2. Realiza las conexiones según la tabla de **Arquitectura de Hardware y Pinout** [cite: 86].
3. Compila y sube el archivo `.ino` a tu ESP32-C6.
4. Escucha atentamente el **Test 1**; si suena como una alarma constante, reemplaza tu buzzer.
5. Interactúa con el sensor táctil y observa la pantalla OLED para validar la integración de los tres componentes.