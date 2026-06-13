# Pruebas de hardware — Diagnóstico por módulos

> **Prerrequisito:** [01_hardware/](../../01_hardware/) — componentes conectados según `circuit_image.png`.

Antes de escribir una sola línea del firmware final, necesitamos saber que cada componente funciona. En sistemas embebidos, los errores más frustrantes son los más difíciles de localizar: cuando algo falla integrado, ¿fue el código, el cable, el voltaje, o el componente mismo?

La respuesta es **aislar**. Un componente, una prueba, un resultado claro. Si pasa, avanzas. Si no, corriges ahí y solo ahí.

**Regla de oro:** si un sketch de prueba no pasa, no avances al siguiente. Corrige primero.

---

## Mapa de pruebas

| # | Sketch | Componente | Qué aprenderás |
|---|--------|-----------|----------------|
| 01 | `01_prueba_pantalla` | Pantalla OLED SH1106 | Protocolo I2C, direcciones de dispositivo, coordenadas de pantalla |
| 02 | `02_prueba_touch` | Sensor táctil TTP223 | GPIO digital, capacitancia, detección de gestos |
| 03 | `03_prueba_buzzer` | Buzzer pasivo | PWM, frecuencias de audio, diferencia activo/pasivo |
| 05 | `05_prueba_conexion_microfono` | Micrófono INMP441 | Protocolo I2S, sincronía de audio digital |
| 06 | `06_diagnostico_hardware` | Todos los componentes | Diagnóstico integral antes del firmware final |
| 08 | `08_prueba_wifi` | WiFi integrado | Redes 2.4 GHz, WPA2, HTTP como puerta a internet |
| 10 | `10_prueba_sonido_microfono` | Micrófono INMP441 | Amplitud, muestreo, umbrales de detección |

> Las pruebas 04, 07, 09, 11, 12 y 13 son opcionales o avanzadas. El orden recomendado es 01 → 02 → 03 → 05 → 06 → 08 → 10.

---

## Fase 1 — Diagnóstico de hardware

### Objetivo

Confirmar que la OLED y el WiFi funcionan. Son los dos componentes más críticos: sin pantalla no hay interfaz, sin WiFi no hay reloj ni clima.

### Requisitos

- ESP32-C3 Super Mini conectado al PC por USB-C
- Componentes cableados según `01_hardware/circuit_image.png`
- Arduino CLI instalado ([guía de instalación](../../02_software/README.md))
- Puerto COM identificado:

```powershell
arduino-cli board list
# Anota el puerto. Ejemplo: COM3, COM7, /dev/ttyUSB0
```

---

### Prueba 01 — Pantalla OLED

**¿Qué aprenderás?** El protocolo I2C permite que varios dispositivos compartan solo dos cables (SDA y SCL). Cada dispositivo tiene una dirección única — la pantalla usa `0x3C`. Esta prueba ejecuta 7 tests automáticos: texto, formas geométricas, todos los píxeles encendidos, inversión de contraste y animación de carga.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/01_prueba_pantalla
```

**Resultado esperado en Monitor Serie:**
```
[INIT] Inicializando display...  OK
[TEST 1] Texto básico            OK
[TEST 2] Escalas de texto        OK
[TEST 3] Formas geométricas      OK
[TEST 4] Todos los píxeles ON    OK
[TEST 5] Inversión de contraste  OK
[TEST 6] Animación de carga      OK
```

**Verificación:**

| Lo que ves | Diagnóstico |
|-----------|-------------|
| 7 pruebas en pantalla | Todo bien |
| Pantalla en blanco | Revisa SDA (GPIO 8) y SCL (GPIO 9) |
| `No I2C device found` en Monitor Serie | Verifica voltaje: 3.3V, no 5V |

> Usa siempre 3.3V. El ESP32-C3 Super Mini no tolera 5V en sus pines GPIO.

**Mini-reto:** Abre `prueba_pantalla.ino` y busca la línea `display.println("PANTALLA OK")`. Cambia el texto por tu nombre. ¿Cuántos caracteres caben en una línea con `setTextSize(1)`? ¿Y con `setTextSize(2)`?

Guía completa → [01_prueba_pantalla/README.md](01_prueba_pantalla/README.md)

---

### Prueba 08 — WiFi

**¿Qué aprenderás?** El ESP32-C3 soporta WPA2-Personal (red de casa) y WPA2-Enterprise (red universitaria con usuario y contraseña separados). Una vez conectado, puede hacer llamadas HTTP a cualquier API en internet.

Abre `08_prueba_wifi/prueba_wifi.ino` y edita el arreglo `networks[]`:

```cpp
WiFiCredential networks[] = {
  // WPA2-Personal (red doméstica)
  {"NOMBRE_DE_TU_RED",  "CONTRASEÑA",  false, "", ""},
  // WPA2-Enterprise (red universitaria — opcional)
  {"RED_UNIVERSITARIA", "",            true,
   "usuario@universidad.edu", "contraseña_eap"},
};
```

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/08_prueba_wifi
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/08_prueba_wifi
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
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
| IP asignada | WiFi funciona |
| `Connecting...` sin terminar | Verifica SSID y contraseña |
| `Connection failed` | Red de 5 GHz — el ESP32-C3 solo soporta 2.4 GHz |
| Monitor Serie vacío | Verifica baudrate = 115200 |

**Para pensar:** Una vez que el ESP32 tiene IP, cualquier dispositivo en la misma red puede comunicarse con él. ¿Qué implica eso para el servidor de transcripción de la Prueba 11?

---

### Checklist — Fase 1

- [ ] La pantalla OLED muestra las 7 pruebas sin errores
- [ ] El Monitor Serie muestra una IP asignada para WiFi

---

## Fase 2 — Calibración de sensores

### Objetivo

Medir los valores reales de tu sensor táctil y tu micrófono en **tu** entorno. Los valores de amplitud cambian según la habitación y el ruido de fondo — no hay umbrales universales.

> Anota los valores que obtengas. Los usarás al configurar el firmware principal.

---

### Prueba 02 — Sensor táctil

**¿Qué aprenderás?** El TTP223 es un sensor capacitivo: detecta cambios en el campo eléctrico cuando un conductor (tu dedo) se acerca a su superficie. Es la misma tecnología de la pantalla de tu teléfono. Este sketch distingue entre tap simple, tap doble, tap triple y hold largo — los mismos gestos que usa Mochi.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/02_prueba_touch
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/02_prueba_touch
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

**Qué hacer:**
1. No toques el sensor. Observa el valor en reposo.
2. Toca con el dedo. Anota el valor.
3. Prueba tocar con un lápiz o una moneda. ¿El sensor reacciona igual?

**Tabla de registro:**

| Estado | Valor observado | Ejemplo típico |
|--------|----------------|----------------|
| Reposo (sin tocar) | ____________ | `0` |
| Al tocar con dedo | ____________ | `1` |

**Mini-reto:** El timing entre taps está controlado por `MULTITAP_MS = 400` (ms). Abre el sketch y cambia ese valor a `600`. ¿Es más fácil o más difícil registrar un doble tap? ¿Qué valor te parece el más natural?

Guía completa → [02_prueba_touch/README.md](02_prueba_touch/README.md)

---

### Prueba 05 — Conexión del micrófono (pines I2S)

**¿Qué aprenderás?** El INMP441 usa el bus I2S, diseñado específicamente para audio digital. Transmite muestras continuas de audio a velocidad fija (16,000 Hz en esta configuración). Esta prueba solo verifica que el bus I2S se inicialice — todavía no procesa sonido.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/05_prueba_conexion_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/05_prueba_conexion_microfono
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

**Resultado esperado:**
```
I2S: inicializado correctamente
Leyendo muestras... OK
```

Guía completa → [05_prueba_conexion_microfono/README.md](05_prueba_conexion_microfono/README.md)

---

### Prueba 06 — Diagnóstico integral de hardware

**¿Qué aprenderás?** Este sketch prueba todos los componentes en secuencia (OLED, touch, buzzer, micrófono) en un solo sketch. Cuando algo falla, señala exactamente qué falló. Es el paso previo más útil antes de cargar el firmware completo.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/06_diagnostico_hardware
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/06_diagnostico_hardware
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

Guía completa → [06_diagnostico_hardware/README.md](06_diagnostico_hardware/README.md)

---

### Prueba 10 — Nivel de sonido del micrófono

**¿Qué aprenderás?** El micrófono convierte presión de aire en números. Cuanto más fuerte el sonido, mayor la diferencia entre el valor máximo y mínimo dentro de cada paquete de 256 muestras — eso es la **amplitud**. No procesamos frecuencias aquí, solo energía bruta. Es suficiente para distinguir silencio, voz y aplausos.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/10_prueba_sonido_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/10_prueba_sonido_microfono
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

**Resultado en tiempo real:**
```
  Amp    | Pico  | Nivel        | Barra
  -------|-------|--------------|------------------------------------------
     850  |   850 | silencio     | |###                                     |
    5200  |  5200 | ruido / voz  | |#############                           |
   14300  | 14300 | voz fuerte   | |############################            |
   31000  | 31000 | MUY FUERTE!  | |#####################################!!!|
```

**Tabla de registro:**

| Condición | Amplitud observada | Ejemplo típico |
|-----------|-------------------|----------------|
| Silencio | ____________ | `200 – 800` |
| Voz normal | ____________ | `3 000 – 8 000` |
| Aplauso | ____________ | `15 000 – 30 000` |

> Tu umbral para "sonido fuerte" debe estar entre voz normal y aplauso. Un valor entre `12 000` y `18 000` es un buen punto de partida.

**Mini-reto:** El sketch aplica suavizado: `suavizado = (suavizado * 5 + amp * 3) / 8`. Cambia el `5` por `2` y el `3` por `6`. ¿La barra sube y baja más rápido o más lento? ¿Cuál configuración sería mejor para un detector de aplausos?

**Retos disponibles a partir de aquí:**
- [Reto 4 — Contador de aplausos](../retos/reto-4-aplausos/): usa estos umbrales para contar aplausos en un nuevo modo de Mochi.
- [Reto 7 — Espejo emocional](../retos/reto-7-espejo-emocional/): hace que los ojos del robot cambien según el ambiente sonoro.

Guía completa → [10_prueba_sonido_microfono/README.md](10_prueba_sonido_microfono/README.md)

---

### Checklist — Fase 2

- [ ] Valor en silencio: ______
- [ ] Valor con voz normal: ______
- [ ] Valor con aplauso: ______
- [ ] Umbral elegido para "sonido fuerte": ______

---

## Prueba 03 — Buzzer pasivo

**¿Qué aprenderás?** El buzzer pasivo es un parlante sin oscilador — necesita que el microcontrolador genere la señal. `tone(pin, frecuencia)` produce una onda cuadrada: a 262 Hz suena Do, a 440 Hz suena La. Un buzzer **activo** tiene el oscilador integrado y solo emite un tono fijo; con él no puedes hacer melodías.

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/03_prueba_buzzer
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/03_prueba_buzzer
```

Al arrancar, escucharás un sweep de 200 Hz → 3000 Hz → 200 Hz. Si el tono sube gradualmente, es pasivo. Si suena constante, es activo — cámbialo.

Después del diagnóstico automático, el sketch entra en modo interactivo: 1 tap = tono corto, 3 taps = tema de Tetris, hold = cambio de modo.

**Mini-reto:** En el sketch, busca el array de la escala musical (Do, Re, Mi...). Agrega una nota más alta al final: `523` Hz es Do5. ¿Puedes agregar `587` Hz (Re5) y `659` Hz (Mi5)? ¿Reconoces la melodía que forman esas tres notas juntas?

**Reto disponible:** El [Reto 1 — Nueva melodía](../retos/reto-1-melodia/) tiene un sketch con el array de notas vacío. Llénalo con cualquier canción.

Guía completa → [03_prueba_buzzer/README.md](03_prueba_buzzer/README.md)

---

## Prueba 09 — Estación meteorológica (clima)

**¿Qué aprenderás?** El ESP32 puede consultar cualquier API REST. OpenWeatherMap devuelve temperatura, humedad y pronóstico en JSON. El sketch lo parsea y lo muestra en la OLED.

Antes de compilar, edita `09_prueba_clima/prueba_clima.ino`:

```cpp
WiFiCredential networks[] = {
  {"NOMBRE_DE_TU_RED", "CONTRASEÑA", false, "", ""},
};
const char* OWM_API_KEY = "TU_CLAVE";   // registro gratuito en openweathermap.org
const char* OWM_CITY    = "Cuenca";
const char* OWM_COUNTRY = "EC";
```

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/09_prueba_clima
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/09_prueba_clima
```

| Acción | Resultado |
|--------|-----------|
| Tap corto | Cicla entre vistas |
| Hold largo | Fuerza actualización inmediata |
| Automático | Actualiza cada 10 minutos |

**Para explorar:** El JSON de OpenWeatherMap tiene muchos más campos de los que se muestran: `feels_like`, `pressure`, `visibility`. ¿Puedes agregar uno a alguna de las vistas?

**Reto disponible:** Una vez que entiendes cómo funciona una llamada REST, puedes conectarte a cualquier otra API. El [Reto 10 — Tu propia API](../retos/reto-10-nueva-api/) te desafía a elegir una diferente.

---

## Prueba 11 — Transcripción de voz

**¿Qué aprenderás?** El ESP32 graba audio en RAM, lo empaqueta como WAV y lo envía por HTTP POST a un servidor Python local. Ese servidor usa Whisper para transcribir el audio y devuelve el texto.

Este es el primer pipeline IoT completo del taller: **sensor físico → microcontrolador → servidor → modelo de IA → respuesta en tiempo real**.

Guía completa → [11_transcripcion/README.md](11_transcripcion/README.md)

**Reto disponible:** [Reto 10 — Tu propia API](../retos/reto-10-nueva-api/): diseña tu propio pipeline conectando el ESP32 a la API de tu elección.

---

## Solución de problemas frecuentes

| Síntoma | Causa más probable | Acción |
|---------|-------------------|--------|
| Monitor Serie en blanco | Baudrate incorrecto | Cambia a 115200 |
| `Connecting...` sin terminar | Red de 5 GHz | Conéctate al SSID de 2.4 GHz |
| Pantalla en blanco tras subir | Pines I2C invertidos | SDA = GPIO 8, SCL = GPIO 9 |
| `Failed to connect to ESP32` | ESP32 no entró en modo upload | Mantén BOOT mientras ejecutas el upload |
| `port is busy` | Monitor Serie abierto en otro terminal | Cierra con Ctrl+C |

Guía ampliada → [docs/errores-comunes.md](../../docs/errores-comunes.md)

---

## Próximo paso

Con los diagnósticos completos, avanza al descanso activo:

**[03_firmware/esp32c3/retos/](../retos/) — Fase 3: Easter Egg — Juegos en OLED**

O si ya terminaste los retos, avanza directamente a la integración:

**[03_firmware/esp32c3/software/](../software/) — Fase 5: El cerebro de Mochi**
