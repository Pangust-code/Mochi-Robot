# Prueba 01 — Pantalla OLED SH1106

Verifica que la pantalla OLED está bien conectada y que el bus I2C responde en la dirección `0x3C`.

---

## ¿Qué vas a aprender?

El protocolo **I2C** permite que varios dispositivos compartan solo dos cables: SDA (datos) y SCL (reloj). Cada dispositivo tiene una dirección única de 7 bits. La pantalla SH1106 usa `0x3C` por defecto. Si el microcontrolador envía un mensaje a esa dirección y nadie responde, algo está mal en el cableado o en el voltaje.

El sketch ejecuta 7 pruebas en secuencia para estresar tanto el canal de comunicación como el panel físico de la pantalla.

---

## Hardware

| Pin OLED | GPIO ESP32-C3 Super Mini | Función |
|----------|--------------------------|---------|
| VCC | 3.3V | Alimentación |
| GND | GND | Tierra |
| SDA | GPIO 8 | Bus de datos I2C |
| SCL | GPIO 9 | Bus de reloj I2C |

> Usa siempre 3.3V. El ESP32-C3 Super Mini no tolera 5V en sus pines GPIO.

---

## Dependencias

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
```

---

## Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/01_prueba_pantalla
```

---

## Secuencia de pruebas

El sketch ejecuta automáticamente estas 7 pruebas al encender:

| # | Prueba | Qué verifica |
|---|--------|-------------|
| 1 | Scanner I2C | Que la pantalla responde en `0x3C` |
| 2 | Texto básico | Coordenadas X/Y correctas |
| 3 | Escalas de texto | Tipografía en tamaños 1, 2 y 3 |
| 4 | Formas geométricas | Rectángulos, círculos, líneas |
| 5 | Todos los píxeles ON | Detecta píxeles físicamente dañados |
| 6 | Inversión de contraste | Comando nativo `invertDisplay()` |
| 7 | Barra de carga | Tasa de refresco y función `map()` |

**Salida esperada en Monitor Serie (115200 baudios):**

```
========================================
  PRUEBA DE PANTALLA — SH1106 OLED
========================================
  SDA=GPIO8  SCL=GPIO9  Dir=0x3C
========================================

[TEST] Scanner I2C
      Dispositivo encontrado en 0x3C
[INIT] Inicializando display...  OK
[TEST 1] Texto básico            OK
[TEST 2] Escalas de texto        OK
[TEST 3] Formas geométricas      OK
[TEST 4] Píxeles ON              OK
[TEST 5] Inversión               OK
[TEST 6] Barra de carga          OK
```

---

## Diagnóstico

| Lo que ves | Causa probable | Solución |
|-----------|---------------|----------|
| Pantalla en blanco | SDA o SCL desconectado | Verifica GPIO 8 y GPIO 9 |
| `No I2C device found` | Voltaje incorrecto o pines invertidos | Confirma 3.3V; prueba SDA y SCL al revés |
| Pantalla con píxeles negros aleatorios | Panel dañado físicamente | Visible en la prueba 5 (todos los píxeles ON) |
| Texto cortado o desalineado | Librería incorrecta | Verifica que sea `Adafruit SH110X`, no `SSD1306` |

---

## Mini-reto

Abre `prueba_pantalla.ino` y busca la línea:

```cpp
display.println("PANTALLA OK");
```

Cámbiala por tu nombre. Compila y sube. Ahora prueba con `setTextSize(2)` — ¿cuántos caracteres caben antes de que el texto se corte por el borde derecho?

La pantalla tiene 128 píxeles de ancho. Con `setTextSize(1)` cada carácter mide 6 píxeles de ancho, con `setTextSize(2)` mide 12. ¿Cuántos caracteres caben en cada caso?

---

**Siguiente prueba →** [02_prueba_touch/](../02_prueba_touch/)
