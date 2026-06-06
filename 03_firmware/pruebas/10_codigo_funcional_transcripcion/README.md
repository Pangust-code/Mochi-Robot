# codigo_funcional_transcripcion — Guía completa

> Sketch de integración para el robot **Mochi** (ESP32-C6 Supermini).  
> Captura audio por micrófono I2S, lo envía a un servidor con IA y recibe un comando de emoción que se refleja en la pantalla OLED con ojos animados.

---

## ¿Qué hace este sketch?

Este sketch es el **cerebro de la interacción hablada** de Mochi. El ciclo completo es:

```
Usuario habla → ESP32 graba (PCM16) → servidor IA analiza
     → devuelve { mood_id, intent } → Mochi muestra emoción en pantalla
```

A diferencia de un sketch de transcripción simple, aquí el servidor no solo convierte audio a texto: **interpreta la intención** del mensaje y devuelve un comando emocional (`mood_id`) que cambia la expresión del robot en tiempo real.

### Flujo visual en pantalla

| Fase | Ojos | Texto inferior |
|------|------|----------------|
| **Listo** | NORMAL (parpadeo natural) | `hold=mic  3tap=M0` |
| **Grabando** | SUSPICIOUS (entrecerrado) | Barra de nivel + porcentaje |
| **Enviando** | DIZZY (girando) | `enviando...` |
| **Resultado OK** | Según `mood_id` recibido | Intent (ej. `saludo`) |
| **Error** | SAD | Código HTTP (ej. `HTTP 500`) |

---

## Diferencia con `transcripcion.ino`

| Característica | `transcripcion.ino` | **Este sketch** |
|---|---|---|
| Formato de audio | WAV (con cabecera) | PCM16 crudo |
| Servidor | Python local (Whisper) | Servidor externo con IA |
| Respuesta del servidor | `{"text": "hola"}` | `{"mood_id": 1, "intent": "saludo"}` |
| Pantalla OLED | ✗ No usa | ✓ Ojos animados con 10 emociones |
| Resultado visible | Solo monitor serial | Pantalla del robot |

---

## Hardware necesario

- **Microcontrolador:** ESP32-C6 Supermini
- **Pantalla:** OLED SH1106G 128×64 px (I2C, dirección `0x3C`)
- **Micrófono:** INMP441 (digital I2S)
- **Sensor táctil:** TTP223
- **Buzzer:** pasivo

### Diagrama de pines

```
ESP32-C6 Supermini
│
├── GPIO 6  ──► OLED SDA
├── GPIO 7  ──► OLED SCL
├── GPIO 2  ──► TTP223 (sensor táctil)
├── GPIO 19 ──► Buzzer pasivo
│
├── GPIO 4  ──► INMP441 SCK  (reloj de bit)
├── GPIO 1  ──► INMP441 WS   (word select / LRCLK)  ⚠ ver nota
└── GPIO 3  ──► INMP441 SD   (datos de audio)
```

> ⚠ **Atención con el pin WS del micrófono:** en este sketch es **GPIO 1**, no GPIO 5 como en otros sketches del proyecto. Verifica tu cableado o cambia `I2S_WS_PIN` en el código si tu hardware está conectado al GPIO 5.

---

## Configuración antes de subir

Abre `codigo_funcional_transcripcion.ino` y edita estas tres líneas al inicio del archivo:

```cpp
const char* WIFI_SSID  = "TU_RED_WIFI";                    // nombre de tu red WiFi
const char* WIFI_PASS  = "TU_CONTRASENA";                   // contraseña de tu red
const char* SERVER_URL = "https://TU_SERVIDOR/audio/pcm16"; // URL del endpoint de audio
```

Opcionalmente puedes cambiar también el identificador del dispositivo:

```cpp
const char* DEVICE_ID = "esp32_01"; // se envía como header HTTP al servidor
```

---

## Dependencias de Arduino

Instala estas dos librerías antes de compilar:

```bash
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
```

Las demás librerías (`WiFi`, `HTTPClient`, `driver/i2s`) vienen incluidas con el paquete del ESP32.

---

## Compilar y subir

```bash
# Compilar
arduino-cli compile --fqbn esp32:esp32:esp32c6 .

# Subir al ESP32 (ajusta el puerto COM según tu sistema)
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 .

# Abrir monitor serial para ver logs
arduino-cli monitor -p COM3 --config baudrate=115200
```

En Linux/macOS el puerto suele ser `/dev/ttyUSB0` o `/dev/ttyACM0` en lugar de `COM3`.

---

## Cómo usar el robot

El sensor táctil TTP223 reconoce dos gestos:

| Gesto | Acción |
|-------|--------|
| **Mantener presionado ≥ 800 ms** | Inicia la grabación (Mochi pone cara de SUSPICIOUS y muestra barra de nivel) |
| **Soltar** después del hold | Detiene la grabación y envía el audio al servidor |
| **3 taps cortos** (dentro de 600 ms) | Cambia de modo (preparado para expansiones futuras) |

> El buzzer emite un pip corto al iniciar la grabación y otro al recibir respuesta correcta del servidor.

---

## Arquitectura interna

### 1. Captura de audio (I2S)

El micrófono INMP441 se configura en modo I2S estándar a **8.000 Hz, 32 bits por muestra, canal izquierdo**. El sketch lee bloques de 4096 bytes en el loop principal y los convierte a **PCM16** (16 bits, signed) con la función `convert_i2s32_to_pcm16`.

Durante los primeros 100 ms de grabación hay un **período de calentamiento** (warmup) que se descarta: el INMP441 necesita unos ciclos para estabilizarse y evitar el "pop" inicial.

El buffer máximo es de **80.000 bytes** (5 segundos × 8000 muestras/s × 2 bytes), suficiente para la mayoría de comandos de voz cortos.

```
INMP441 → I2S 32bit → convert_i2s32_to_pcm16() → buffer _rec_buf[]
```

La conversión también aplica:
- **Filtro DC** (EMA exponencial) para eliminar el offset constante del micrófono
- **Saturación** para no desbordar el rango int16

### 2. Envío HTTP (tarea en Core 0)

Para no bloquear la animación de la pantalla, el envío se realiza en una **tarea FreeRTOS separada** en el Core 0 (`httpSendTask`). El Core 1 sigue dibujando los ojos con fluidez.

La petición es un `POST` con:

```
Content-Type: application/octet-stream   ← bytes PCM16 crudos
device-id:    esp32_01
session-id:   <timestamp en ms>
end:          true
```

Se usa `WiFiClientSecure` con `setInsecure()` para aceptar certificados TLS sin validar la cadena de confianza (útil para servidores propios con certificado autofirmado).

### 3. Parsing de la respuesta

El servidor devuelve un JSON mínimo:

```json
{
  "mood_id": 1,
  "intent": "saludo"
}
```

El sketch lo parsea manualmente (sin librería JSON) con dos funciones de búsqueda de strings:
- `parseMoodId()` → extrae el número entero de `mood_id`
- `parseIntent()` → extrae la cadena de `intent`

### 4. Animación de ojos (spring physics)

Cada ojo es una estructura `Eye` con física de resorte amortiguado. En cada frame:
- La posición actual se acerca a la posición objetivo con velocidad proporcional al error (spring `k=0.12`, damping `d=0.60`)
- La pupila se mueve igual pero más lenta (`pk=0.08`, `pd=0.50`)
- El parpadeo es automático cada 2–6 segundos (aleatorio)
- Las sacadas (movimientos rápidos de pupila) ocurren cada 0.5–3 s

La forma de los párpados varía según el mood activo:

| Mood | Efecto de párpado |
|------|-------------------|
| ANGRY | Cejas inclinadas hacia adentro (líneas diagonales) |
| SAD | Cejas inclinadas hacia afuera |
| HAPPY / LOVE / EXCITED | Recorte inferior (ojos en forma de "D" invertida) |
| SLEEPY | Párpado superior cubre mitad del ojo |
| SUSPICIOUS | Ojo izquierdo entrecerrado, ojo derecho normal |
| DIZZY | Oscilación senoidal del tamaño y posición de pupila |

---

## Moods disponibles

| `mood_id` | Nombre | Descripción visual |
|-----------|--------|--------------------|
| 0 | NORMAL | Ojos redondos, parpadeo suave |
| 1 | HAPPY | Ojos anchos con recorte inferior |
| 2 | SURPRISED | Ojos muy altos (verticales) |
| 3 | SLEEPY | Ojos semicerrados |
| 4 | ANGRY | Cejas en V invertida |
| 5 | SAD | Cejas en V |
| 6 | EXCITED | Ojos grandes con recorte |
| 7 | LOVE | Igual que HAPPY |
| 8 | SUSPICIOUS | Asimétrico: un ojo cerrado |
| 9 | DIZZY | Tamaño y pupila oscilantes |

---

## Parámetros ajustables en el código

| Constante | Valor por defecto | Efecto |
|-----------|-------------------|--------|
| `REC_SAMPLE_RATE` | 8000 Hz | Calidad de audio. Subir consume más memoria |
| `REC_DURATION_S` | 5 s | Duración máxima de grabación |
| `CHUNK_SIZE` | 4096 bytes | Tamaño de lectura I2S por ciclo de loop |
| `GAIN_SHIFT` | 14 | Amplificación del micrófono (mayor = más sensible) |
| `WARMUP_DURATION_MS` | 100 ms | Tiempo de descarte al inicio de la grabación |
| `HOLD_MS` | 800 ms | Tiempo mínimo de presión para iniciar grabación |
| `TAP_WINDOW_MS` | 600 ms | Ventana para contar taps consecutivos |

---

## Solución de problemas

| Síntoma | Causa probable | Solución |
|---------|---------------|----------|
| OLED no enciende o muestra basura | Dirección I2C incorrecta o pines SDA/SCL invertidos | Verifica GPIO 6 (SDA) y GPIO 7 (SCL); corre el sketch `prueba_pantalla` |
| "WiFi: FALLO" en pantalla | Credenciales incorrectas o red fuera de rango | Edita `WIFI_SSID` / `WIFI_PASS`; verifica que el ESP32 esté cerca del router |
| `HTTP -1` o `HTTP -4` | Sin conexión al servidor o URL incorrecta | Verifica `SERVER_URL`; comprueba que el servidor esté activo y accesible |
| `HTTP 500` | Error interno en el servidor | Revisa los logs del servidor; verifica que el endpoint `/audio/pcm16` exista |
| `rms≈0` en el monitor serial | Micrófono mal conectado o pin WS incorrecto | Verifica que WS esté en **GPIO 1**; corre el sketch `prueba_sonido_microfono` |
| Ojos se congelan durante el envío | Normal — el envío HTTP puede tardar varios segundos | El mood DIZZY (oscilante) está diseñado para cubrir esta espera |
| Robot no responde al tacto | `TOUCH_PIN` flotante o sensor desconectado | Verifica conexión de TTP223 a GPIO 2 |

---

## Estructura del código

```
setup()
  ├── Inicializa Serial, pines, OLED, ojos
  ├── Conecta WiFi (bloquea si falla)
  └── initMicrophone() → configura I2S

loop()
  ├── procesarTouch()          ← detecta gestos (hold, taps)
  ├── Lógica de estado         ← ST_LISTO / ST_GRABANDO / ST_ENVIANDO / ST_RESULTADO / ST_ERROR
  ├── Captura I2S              ← solo activo en ST_GRABANDO
  ├── Chequeo _rec_done        ← cuando httpSendTask termina, actualiza estado y mood
  └── switch(appState)         ← renderiza pantalla según estado

httpSendTask() [Core 0, FreeRTOS]
  ├── POST PCM16 → servidor
  ├── parseMoodId() + parseIntent()
  ├── setMood() → cambia currentMood
  └── _rec_done = true → notifica al loop principal
```