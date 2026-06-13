# Prueba 02 — Sensor táctil TTP223

Verifica que el sensor capacitivo detecta correctamente el contacto y distingue tap simple, doble, triple y hold.

---

## ¿Qué vas a aprender?

El TTP223 es un sensor **capacitivo**: cuando tu dedo (un conductor) se acerca a la placa metálica del sensor, cambia la carga eléctrica almacenada en esa superficie. El chip interno detecta ese cambio y emite un `1` en su pin de salida. Es la misma tecnología que usa la pantalla de tu teléfono.

La diferencia con un botón mecánico: no hay partes móviles, y funciona incluso a través de una capa delgada de material (como plástico o vidrio fino).

El sketch implementa una máquina de estados con temporizadores no bloqueantes para distinguir cuatro gestos:

| Gesto | Descripción | `HOLD_MS` / `MULTITAP_MS` |
|-------|-------------|--------------------------|
| TAP x1 | Toque corto soltado antes de 800 ms | — |
| TAP x2 | Dos toques dentro de 400 ms | `MULTITAP_MS = 400` |
| TAP x3+ | Tres o más toques rápidos | `MULTITAP_MS = 400` |
| HOLD | Toque sostenido ≥ 800 ms | `HOLD_MS = 800` |

Estos son exactamente los mismos umbrales que usa el firmware final de Mochi.

---

## Hardware

| Pin TTP223 | GPIO ESP32-C6 | Función |
|-----------|--------------|---------|
| VCC | 3.3V | Alimentación |
| GND | GND | Tierra |
| SIG / OUT | GPIO 2 | Salida digital |

También se usa la OLED para mostrar el panel de estado en tiempo real.

---

## Dependencias

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
```

---

## Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/esp32c6/pruebas/02_prueba_touch
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/esp32c6/pruebas/02_prueba_touch
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

---

## Resultado esperado

La OLED muestra en tiempo real:
- Último evento detectado (TAP x1, TAP x2, TAP x3, HOLD)
- Estado crudo del pin (0 o 1) con temporizador interno en ms
- Contador acumulativo de eventos

En el Monitor Serie:

```
[TOUCH] TAP x1  detectado
[TOUCH] TAP x2  detectado
[TOUCH] TAP x3  detectado
[TOUCH] HOLD    detectado  (1024 ms)
```

---

## Diagnóstico

| Lo que ves | Causa probable | Solución |
|-----------|---------------|----------|
| Siempre `1` aunque no toques | Pin OUT mal conectado o en cortocircuito | Revisa la conexión OUT → GPIO 2 |
| Siempre `0` aunque toques | VCC incorrecto | Verifica que el TTP223 recibe 3.3V |
| No detecta doble tap | Ventana muy corta | Sube `MULTITAP_MS` a `600` |
| HOLD dispara inmediatamente | `HOLD_MS` muy bajo | Verifica que esté en `800` |

---

## Mini-reto

El timing es la clave de la interacción. Abre `prueba_touch.ino` y encuentra las líneas:

```cpp
#define HOLD_MS      800
#define MULTITAP_MS  400
```

Experimenta con estos valores:

1. Cambia `MULTITAP_MS` a `200`. ¿Es más difícil registrar un doble tap?
2. Cambia `HOLD_MS` a `400`. ¿El HOLD se activa demasiado fácil?
3. ¿Qué combinación sientes más natural para usar en un robot?

No hay respuesta correcta — estas son decisiones de diseño de experiencia de usuario (UX). El firmware final usa `800` y `400` porque son los valores que encontramos más intuitivos en pruebas con usuarios.

---

**Siguiente prueba →** [03_prueba_buzzer/](../03_prueba_buzzer/)
