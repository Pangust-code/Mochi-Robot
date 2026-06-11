# prueba_canvas — Guía completa

> Sketch de prueba del **modo Canvas** para el robot Mochi (ESP32-C3 Super Mini).  
> Permite que un servidor externo tome control total de la pantalla OLED en tiempo real, enviando fotogramas completos a través de una conexión WebSocket persistente.

---

## ¿Qué hace este sketch?

Este sketch convierte el ESP32 en un **terminal de pantalla remota**. El servidor puede enviar cualquier imagen en formato bitmap y esta aparece instantáneamente en el OLED del robot, sin que el ESP32 sepa ni le importe qué contiene la imagen.

El ciclo completo es:

```
Servidor (Railway / local)
    │
    │  WebSocket (wss://)
    │
    ▼
ESP32 recibe JSON:
  {"type": "canvas_update", "bitmap": "<base64>"}
    │
    ▼
Decodifica base64 → bitmap 1024 bytes
    │
    ▼
memcpy directo al buffer del SH1106 → pantalla actualizada
```

Cuando el servidor envía `canvas_exit`, el robot vuelve a su pantalla de estado normal mostrando IP y estado de conexión.

---

## Los dos modos de operación

| Modo | Quién controla la pantalla | Qué se muestra |
|------|---------------------------|----------------|
| **NORMAL** | El propio ESP32 | IP local, estado WiFi, estado WebSocket |
| **CANVAS** | El servidor remoto | Cualquier imagen bitmap enviada por WebSocket |

El robot arranca siempre en modo NORMAL y permanece ahí hasta recibir el primer `canvas_update`.

---

## Hardware necesario

- **Microcontrolador:** ESP32-C3 Super Mini
- **Pantalla:** OLED SH1106 128×64 px (I2C, dirección `0x3C`)

### Pines

```
ESP32-C3 Super Mini
│
├── GPIO 8 ──► OLED SDA
└── GPIO 9 ──► OLED SCL
```

No hay micrófono, buzzer ni sensor táctil en este sketch. Es únicamente una prueba de la funcionalidad de pantalla remota.

---

## Configuración antes de subir

Abre `prueba_canvas.ino` y edita el bloque de configuración personal al inicio del archivo:

```cpp
#define DEVICE_ID  "especito"    // ← nombre único para este robot (sin espacios)

const char* WIFI_SSID = "TU_RED_WIFI";     // nombre de tu red WiFi
const char* WIFI_PASS = "TU_CONTRASENA";   // contraseña

const char* WS_HOST = "dasaimochiservidor-production.up.railway.app"; // dominio del servidor (sin https://)
const int   WS_PORT = 443;                 // 443 para TLS (Railway), 80 para servidor local sin TLS
```

El `DEVICE_ID` se incluye en la URL del WebSocket como parámetro:

```
wss://TU_SERVIDOR/ws?device_id=especito
```

Así el servidor puede identificar qué robot se está conectando y enviarle mensajes dirigidos a él.

---

## Dependencias de Arduino

Instala estas librerías desde el Library Manager del Arduino IDE:

| Librería | Autor | Para qué se usa |
|----------|-------|-----------------|
| **WebSockets** | Markus Sattler | Conexión WebSocket (clase `WebSocketsClient`) |
| **ArduinoJson** | Benoit Blanchon | Parseo de los mensajes JSON del servidor |
| **U8g2** | oliver | Control del display SH1106 |

Las librerías `WiFi` y `Wire` vienen incluidas con el core ESP32 y no requieren instalación adicional.

---

## Compilar y subir

```bash
# Compilar (modo producción — TLS activo)
arduino-cli compile --fqbn esp32:esp32:esp32c3 .

# Subir al ESP32
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 .

# Monitor serial para ver logs
arduino-cli monitor -p COM3 --config baudrate=115200
```

En Linux/macOS el puerto suele ser `/dev/ttyUSB0` o `/dev/ttyACM0`.

### Modo debug local (sin TLS)

Si quieres probar contra un servidor local en lugar de Railway, compila con la macro `DEBUG_LOCAL` definida:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3 --build-property "compiler.cpp.extra_flags=-DDEBUG_LOCAL" .
```

Con `DEBUG_LOCAL` activo el sketch usa `ws://` en lugar de `wss://`, lo que permite conectar a un servidor HTTP local sin certificado TLS.

---

## Protocolo de mensajes WebSocket

El servidor envía mensajes de texto en formato JSON. El ESP32 reconoce dos tipos:

### `canvas_update` — actualizar la pantalla

```json
{
  "type": "canvas_update",
  "bitmap": "<string base64 de exactamente 1024 bytes>"
}
```

El campo `bitmap` es un bitmap de 128×64 píxeles en formato de página SH1106, codificado en base64. El ESP32 lo decodifica y lo vuelca directamente en el buffer del display.

### `canvas_exit` — volver a modo normal

```json
{
  "type": "canvas_exit"
}
```

El ESP32 descarta el modo canvas y vuelve a mostrar su pantalla de estado (IP + estado de conexión). Cualquier otro tipo de mensaje (por ejemplo `transcription`, `intent`) es ignorado silenciosamente en este sketch de prueba.

---

## Arquitectura interna

### 1. Formato del bitmap (1024 bytes)

La pantalla SH1106 organiza los 64 píxeles de alto en **8 páginas de 8 píxeles cada una**. Cada página tiene 128 columnas. Un byte por columna por página: 8 × 128 = **1024 bytes** en total.

```
byte[page * 128 + col]
  bit 0 → pixel superior de la página
  bit 7 → pixel inferior de la página
```

Este es exactamente el mismo layout que usa internamente U8g2 en modo `_F_` (full frame buffer). Por eso la función `drawBitmap` puede hacer un `memcpy` directo al buffer interno sin ninguna transformación:

```cpp
void drawBitmap(const uint8_t* bitmap) {
  uint8_t* buf = u8g2.getBufferPtr();
  memcpy(buf, bitmap, 1024);  // copia directa — sin transformación
  u8g2.sendBuffer();          // envía al display por I2C
}
```

Esto hace que el renderizado sea extremadamente rápido: no hay cálculos de píxeles individuales, solo una transferencia de memoria.

### 2. Decodificación Base64 propia

Para no depender de una librería extra, el sketch incluye su propio decodificador Base64 de ~25 líneas. Procesa la cadena de caracteres en grupos de 4 y produce grupos de 3 bytes, que es el esquema estándar Base64. Los caracteres de padding `=` se manejan correctamente y la función devuelve el número de bytes escritos para que el código pueda verificar que llegaron exactamente 1024.

### 3. WebSocket persistente con reconexión automática

La librería `WebSocketsClient` mantiene la conexión abierta y llama al callback `onWsEvent` cada vez que ocurre un evento. Los eventos manejados son:

| Evento | Acción |
|--------|--------|
| `WStype_CONNECTED` | Marca `wsConnected = true`, refresca pantalla de estado |
| `WStype_DISCONNECTED` | Marca `wsConnected = false`, refresca pantalla de estado |
| `WStype_TEXT` | Parsea el JSON y actúa según el tipo de mensaje |
| `WStype_ERROR` | Imprime el error en el monitor serial (sin acción adicional) |

Con `setReconnectInterval(5000)` la librería intenta reconectarse automáticamente cada 5 segundos si se pierde la conexión, sin necesidad de reiniciar el ESP32.

### 4. Pantalla de estado (modo NORMAL)

Cuando el robot está en modo NORMAL, el loop refresca la pantalla cada 3 segundos (no bloqueante con `millis()`). Muestra:

- Título centrado: `Mochi Canvas`
- IP local asignada por WiFi (o `sin conexion` si no hay red)
- Estado del WebSocket: `conectado` / `esperando...`
- Hint inferior: `usa canvas.html` cuando está listo, `conectando...` mientras no

---

## Flujo de arranque

```
setup()
  ├── Inicializa Serial y OLED → muestra pantalla de estado vacía
  ├── Conecta WiFi (espera hasta 20 s, no bloquea indefinidamente)
  ├── Construye la URL: /ws?device_id=DEVICE_ID
  └── Inicia WebSocket (SSL o plano según DEBUG_LOCAL)

loop()
  ├── webSocket.loop()          ← procesa mensajes entrantes y mantiene la conexión viva
  └── cada 3 s en MODE_NORMAL → drawStatusScreen()
```

---

## Solución de problemas

| Síntoma | Causa probable | Solución |
|---------|---------------|----------|
| OLED en blanco al arrancar | Pines SDA/SCL incorrectos o dirección I2C distinta | Verifica GPIO 8 (SDA) y GPIO 9 (SCL); prueba con un scanner I2C |
| "WiFi: no se pudo conectar" en serial | Credenciales incorrectas o red fuera de rango | Edita `WIFI_SSID` / `WIFI_PASS` |
| "WS: esperando..." no cambia a "conectado" | URL del servidor incorrecta o servidor caído | Verifica `WS_HOST`; comprueba que Railway esté activo |
| Bitmap aparece corrupto o girado | El servidor envía el bitmap en un formato de página diferente | El servidor debe usar el mismo layout de 8 páginas × 128 columnas |
| `bitmap incorrecto: esperaba 1024, recibí N` | El base64 está mal codificado o el bitmap tiene tamaño incorrecto | El servidor debe enviar exactamente 1024 bytes antes de codificar en base64 |
| La pantalla no responde a `canvas_exit` | El mensaje llega mal formado | Verifica en el monitor serial que el JSON sea exactamente `{"type":"canvas_exit"}` |
| Conexión cae frecuentemente | Señal WiFi débil o timeout del servidor | Acerca el ESP32 al router; el sketch reconecta automáticamente cada 5 s |
