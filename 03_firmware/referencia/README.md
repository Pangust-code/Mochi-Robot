# Firmware de referencia — v4

Esta versión es una referencia histórica del proyecto. Fue desarrollada para el **ESP32 clásico de 38 pines** y tiene 3 modos de operación.

> **No es la versión activa.** Úsala solo como referencia o si tienes un ESP32 de 38 pines en lugar del ESP32-C6 Supermini.

---

## Diferencias con la versión actual

| Característica | Esta versión (v4) | Versión actual (v6) |
|---------------|-------------------|---------------------|
| Microcontrolador | ESP32 38 pines | ESP32-C6 Supermini |
| Pines I2C | SDA=21, SCL=22 | SDA=6, SCL=7 |
| Pin táctil | GPIO 15 | GPIO 2 |
| Cambio de modo | Botón físico (GPIO 4) | Hold táctil largo |
| Modos disponibles | 3 | 5 |
| Bluetooth | Sí (comandos Pomodoro) | No |
| Animaciones GIF | No | Sí (14 archivos) |
| Manejo de ánimos | Secuencial (0→1→2…→9) | Aleatorio por toque |

---

## Modos disponibles (v4)

### Modo 0 — Mascota
Ojos animados con 10 estados de ánimo. El toque cambia el ánimo de forma **secuencial** (0→1→2→…→9→0), a diferencia de la v6 que lo hace de forma aleatoria.

### Modo 1 — Reloj
Sincronización NTP via WiFi. Muestra hora y fecha.

### Modo 2 — Pomodoro con Bluetooth
Timer 25/5 minutos. Acepta comandos de voz via Bluetooth Serial desde una app en el celular.

**Comandos Bluetooth reconocidos:**
- `"iniciar"` / `"empezar"` → start
- `"pausa"` / `"pausar"` → pause
- `"reset"` / `"reiniciar"` → reset
- `"suma 10 minutos"` → agrega 10 min al timer

---

## Archivo

| Archivo | Descripción |
|---------|-------------|
| `esp32WROOMsinIA.ino` | Código fuente de la versión de referencia |
