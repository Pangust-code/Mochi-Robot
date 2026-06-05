# Pruebas de hardware — Diagnóstico por módulos

> **Prerrequisito:** [01_hardware/](../../01_hardware/) — componentes conectados según `circuit_image.png`.

En ingeniería embebida no ensamblamos todo para luego rezar que encienda. Probamos por módulos. Esta carpeta contiene sketches individuales, uno por componente, para que puedas confirmar que cada pieza funciona antes de integrar el firmware completo.

**Regla de oro:** si un sketch de prueba no pasa, no avances al siguiente. Corrige primero.

---

## Mapa de pruebas

| # | Sketch | Componente | Protocolo | Resultado esperado |
|---|--------|-----------|-----------|-------------------|
| 01 | `01_prueba_pantalla` | Pantalla OLED SH1106 | I2C | Texto y gráficos en pantalla |
| 02 | `02_prueba_touch` | Sensor táctil TTP223 | GPIO | `TOCADO` / `SOLTADO` en Monitor Serie |
| 03 | `03_prueba_buzzer` | Buzzer pasivo | PWM | Sweep de tonos ascendentes |
| 05 | `05_prueba_wifi` | WiFi integrado | 802.11 | IP asignada en Monitor Serie |
| 07 | `07_prueba_conexion_microfono` | Micrófono INMP441 | I2S | Pines detectados, sin error |
| 08 | `08_prueba_sonido_microfono` | Micrófono INMP441 | I2S | Barra de volumen reactiva |

> Las pruebas 04 y 06 son opcionales en el taller. El orden 01 → 02 → 03 → 05 → 07 → 08 es el recomendado.

---

## Fase 1 — Diagnóstico de hardware

### Objetivo

Confirmar que la OLED y el WiFi funcionan correctamente antes de continuar. Son los dos componentes más críticos del sistema.

### Requisitos

- ESP32-C6 conectado al PC por USB-C
- Componentes cableados según `01_hardware/circuit_image.png`
- Arduino CLI instalado ([guía de instalación](../../02_software/README.md))
- Puerto COM identificado:

```powershell
arduino-cli board list
# Anota el puerto. Ejemplo: COM3, COM7, /dev/ttyUSB0
```

---

### Prueba 01 — Pantalla OLED

**¿Qué verifica?** Que el bus I2C funciona y la dirección `0x3C` responde correctamente.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
```

**Resultado esperado:**
- La pantalla muestra texto y figuras geométricas.
- Ningún mensaje de error en la terminal.

**Verificación:**

| Lo que ves | Diagnóstico |
|-----------|-------------|
| Texto y gráficos en pantalla |   Todo bien |
| Pantalla en blanco |   Revisa SDA (GPIO 6) y SCL (GPIO 7) |
| Error `No I2C device found` en Monitor Serie |   Verifica voltaje: debe ser 3.3V, no 5V |

> **Nota:** Usa siempre 3.3V. El ESP32-C6 no tolera 5V en sus pines GPIO.

---

### Prueba 05 — WiFi

**¿Qué verifica?** Que el ESP32 puede conectarse a una red WiFi y obtiene una IP.

Antes de compilar, abre el archivo `05_prueba_wifi/prueba_wifi.ino` y edita las credenciales:

```cpp
const char* ssid     = "NOMBRE_DE_TU_RED";
const char* password = "CONTRASEÑA";
```

Luego compila y sube:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/05_prueba_wifi
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/05_prueba_wifi
```

Abre el Monitor Serie:

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

**Resultado esperado:**
```
Conectando a NOMBRE_DE_TU_RED...
WiFi conectado
IP: 192.168.1.XX
```

**Verificación:**

| Lo que ves | Diagnóstico |
|-----------|-------------|
| IP asignada |   WiFi funciona |
| `Connecting...` que no termina |   Verifica SSID y contraseña |
| `Connection failed` |   Red de 5 GHz — el ESP32-C6 solo soporta 2.4 GHz |
| Pantalla del Monitor Serie vacía |   Verifica que el baudrate sea 115200 |

---

### Checklist — Fase 1

Antes de continuar, confirma:

- [ ] La pantalla OLED muestra texto y gráficos sin errores
- [ ] El Monitor Serie muestra una IP asignada para WiFi

> Si algún punto falla, consulta [docs/errores-comunes.md](../../docs/errores-comunes.md) antes de continuar.

---

## Fase 2 — Calibración de sensores

### Objetivo

Medir los valores reales que generan el sensor táctil y el micrófono en tu entorno específico. Estos valores serán tus **umbrales personalizados** para el firmware final.

> Toma nota de los valores que obtengas — los necesitarás en la Fase 5.

---

### Prueba 02 — Sensor táctil

**¿Qué verifica?** Que el TTP223 detecta correctamente el contacto y el estado reposo.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/02_prueba_touch
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/02_prueba_touch
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

**Qué hacer:**
1. Observa el Monitor Serie sin tocar el sensor. Anota el valor en reposo.
2. Toca el sensor con el dedo. Anota el valor al tocar.
3. Suelta. Anota el valor al soltar.

**Tabla de registro:**

| Estado | Valor observado | Ejemplo típico |
|--------|----------------|----------------|
| Reposo (sin tocar) | ____________ | `0` |
| Al tocar | ____________ | `1` |
| Al soltar | ____________ | `0` |

**Verificación:**

| Lo que ves | Diagnóstico |
|-----------|-------------|
| Los valores cambian entre 0 y 1 |   Sensor funcionando |
| Siempre `1` aunque no lo toques |   Revisa la conexión OUT → GPIO 2 |
| Siempre `0` aunque lo toques |   Verifica VCC = 3.3V |

---

### Prueba 07 — Conexión del micrófono (pines I2S)

**¿Qué verifica?** Que los pines físicos del INMP441 están conectados y el bus I2S responde.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/07_prueba_conexion_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/07_prueba_conexion_microfono
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

**Resultado esperado:**
```
I2S: inicializado correctamente
Leyendo muestras... OK
```

---

### Prueba 08 — Nivel de sonido del micrófono

**¿Qué verifica?** Que el micrófono capta audio y produce lecturas de amplitud variables.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/08_prueba_sonido_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/08_prueba_sonido_microfono
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6 -c baudrate=115200
```

**Qué hacer:**
1. Observa los valores en silencio absoluto. Anota el valor base.
2. Habla en voz normal. Anota el valor.
3. Aplaude cerca del micrófono. Anota el valor pico.

**Tabla de registro de umbrales:**

| Condición | Amplitud observada | Ejemplo típico |
|-----------|-------------------|----------------|
| Silencio | ____________ | `200 – 800` |
| Voz normal | ____________ | `3 000 – 8 000` |
| Aplauso o golpe fuerte | ____________ | `15 000 – 30 000` |

> Tu umbral de "sonido fuerte" para el firmware debe estar entre el valor de "voz normal" y el de "aplauso". Un valor entre `12 000` y `18 000` es un buen punto de partida.

**Verificación:**

| Lo que ves | Diagnóstico |
|-----------|-------------|
| Valores suben y bajan con el sonido |   Micrófono funcionando |
| Valor siempre en cero |   Verifica WS→GPIO5, SCK→GPIO4, SD→GPIO3 |
| Valor siempre al máximo |   El pin L/R del INMP441 debe estar en GND |
| Valor con ruido constante sin sonido |   Aleja el micrófono de los cables de alimentación |

---

### Checklist — Fase 2

Antes de continuar, anota:

- [ ] Valor de amplitud en silencio: ______
- [ ] Valor de amplitud con voz normal: ______
- [ ] Valor de amplitud con aplauso: ______
- [ ] Umbral elegido para "sonido fuerte": ______

> Guarda estos valores. En la Fase 5 los ingresarás en la sección de configuración de `mochi_unified_5.ino`.

---

## Solución de problemas frecuentes

| Síntoma | Causa más probable | Acción |
|---------|-------------------|--------|
| Monitor Serie en blanco | Baudrate incorrecto | Cambia a 115200 en el monitor |
| `Connecting...` no termina (WiFi) | Red de 5 GHz | Conéctate al SSID de 2.4 GHz |
| Pantalla en blanco tras subir sketch | Pines I2C invertidos | SDA = GPIO 6, SCL = GPIO 7 (no al revés) |
| `Failed to connect to ESP32` | ESP32 no entró en modo upload | Presiona BOOT mientras ejecutas el upload |
| `port is busy` | Monitor Serie abierto en otro terminal | Cierra otros monitores con Ctrl+C |

Guía ampliada → [docs/errores-comunes.md](../../docs/errores-comunes.md)

---

## Próximo paso

Con los diagnósticos completos, continúa con el descanso activo en:

**[03_firmware/retos/](../retos/) — Fase 3: Easter Egg — Juegos en OLED**

O si ya terminaste los retos, avanza directamente a la integración:

**[03_firmware/mochi_unified_5/](../mochi_unified_5/) — Fase 5: El cerebro de Mochi**
