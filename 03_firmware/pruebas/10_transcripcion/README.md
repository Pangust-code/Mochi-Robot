# 🎙️ Sistema de Transcripción de Voz: ESP32-C6 + INMP441 + Servidor Whisper

Este repositorio contiene un sistema cliente-servidor para capturar audio a través de un micrófono I2S, procesarlo en un microcontrolador ESP32-C6 y enviarlo a un servidor local en Python impulsado por el modelo Whisper de OpenAI para obtener su transcripción en texto.

---

## 🏗️ Arquitectura del Sistema

El flujo de trabajo divide la carga de hardware: el ESP32 se encarga exclusivamente de la captura y empaquetado del audio, mientras que la computadora local realiza la inferencia de Inteligencia Artificial (IA).

    ESP32-C6 ──[HTTP POST (WAV)]──► servidor.py ──[Whisper AI]──► {"text": "hola mundo"}
       ▲                                                                │
       └───────────────────────[Respuesta JSON]─────────────────────────┘

### Pinout y Hardware Requerido
| Componente | Pin ESP32-C6 | Función |
| :--- | :--- | :--- |
| **Touch TTP223** | **GPIO 2** | Disparador de grabación. |
| **Buzzer Pasivo**| **GPIO 19** | Feedback acústico de inicio/fin. |
| **INMP441 (SCK)**| **GPIO 4** | Reloj de bits I2S. |
| **INMP441 (WS)** | **GPIO 5** | Word Select I2S. |
| **INMP441 (SD)** | **GPIO 3** | Datos de audio I2S. |
| **INMP441 (L/R)**| **GND** | Forzar lectura de canal mono. |

---

## 🧠 Desglose del Código del Cliente (`transcripcion.ino`)

El firmware del ESP32 realiza un procesamiento avanzado de memoria y manipulación de bytes para enviar datos compatibles con el servidor.

### 1. Gestión Dinámica de Memoria (Heap)
Para grabar 3 segundos de audio a **16,000 Hz** en **16 bits**, el sistema requiere un búfer masivo de aproximadamente **96 KB**. 
En lugar de declarar este búfer de forma global (lo que bloquearía la memoria RAM del chip), el código utiliza `malloc(WAV_TOTAL)` justo en el momento en que se toca el sensor. Al finalizar el envío HTTP, el sistema libera la memoria inmediatamente usando `free(wavBuf)`. Si el búfer supera el 80% de la memoria libre, el sistema lanza una advertencia en consola.

### 2. Construcción de Cabeceras WAV (En crudo)
Los micrófonos I2S capturan modulación de código de pulsos (PCM) en bruto. Whisper requiere un archivo `.wav` estructurado. El ESP32 construye manualmente la cabecera WAV estándar de **44 bytes** escribiendo las etiquetas `"RIFF"`, `"WAVE"`, `"fmt "` y `"data"`, junto con las tasas de bits y tamaños de los *chunks* utilizando operaciones de desplazamiento de bits (Bitwise).

### 3. Máquina de Estados y HTTP POST
El microcontrolador implementa la máquina de estados `enum Estado { ESPERANDO, GRABANDO, ENVIANDO }` para evitar disparos múltiples del sensor. 
Una vez capturado el audio, empaqueta el búfer con un tipo de contenido `audio/wav` y lo envía vía POST. Dado que la inferencia de Whisper puede demorar, el cliente HTTP tiene un *timeout* extendido de **30,000 ms** (30 segundos). Tras recibir el código `200 OK`, la librería `ArduinoJson` decodifica la respuesta para extraer el valor `"text"`.

---

## 🚀 Guía de Despliegue y Configuración

### Paso 1: Levantar el Servidor Python Local
Requisito: Python 3.9 o superior.
```bash
cd servidor/
pip install -r requirements.txt
python servidor.py
```
Nota: La primera ejecución descargará el modelo Whisper (~150 MB para la versión "base"). Solo ocurre una vez.

## Paso 2: Obtener IP y Configurar el ESP32
Obtén la IP local de tu computadora (ipconfig en Windows, ip addr en Linux/Mac). Luego, abre transcripcion.ino y edita estrictamente estas tres líneas:
```C++
const char* WIFI_SSID  = "TU_RED_WIFI";       // Nombre de tu red WiFi
const char* WIFI_PASS  = "TU_CONTRASENA";     // Contraseña de tu red WiFi
const char* SERVER_URL = "[http://192.168.1.](http://192.168.1.)XXX:5000/transcribir"; // IP de tu PC
```
## Paso 3: Dependencias del ESP32
Instala la librería de JSON (versión 7+ requerida) a través de tu IDE o CLI:
```bash
arduino-cli lib install "ArduinoJson"
```
Compila y sube el código al ESP32-C6.

## 🎤 Instrucciones de Uso
1. Abre el Monitor Serial a 115200 baudios.

2. Toca el sensor capacitivo: El buzzer emitirá un beep corto agudo (1200 Hz) indicando el inicio de la grabación.

3. Habla claramente hacia el micrófono durante los 3 segundos programados.

4. El buzzer emitirá un beep largo más grave (800 Hz), marcando el fin de la captura.

5. Observa la consola para ver el progreso de envío y la transcripción devuelta:

```Plaintext
Grabando 3 segundos...
Grabados 96000 bytes
Enviando 96044 bytes a [http://192.168.1.](http://192.168.1.)XXX:5000/transcribir
Respuesta HTTP: 200
JSON recibido: {"text":"hola mundo como están","idioma":"es","duracion_s":2.94}
┌─────────────────────────────┐
│ Transcripcion: hola mundo como están
└─────────────────────────────┘
```

# ⚙️ Ajustes Avanzados
## Tiempo de Grabación (transcripcion.ino)
Modifica la directiva #define RECORD_SECS 3 para alterar la duración de la captura.

> ⚠️ Límite Físico: Cada segundo consume ~32 KB de RAM. No superes los 5 segundos sin memoria PSRAM externa, o el microcontrolador se quedará sin Heap y colapsará.

## Modelo de Inferencia (servidor/servidor.py)
Puedes cambiar la constante MODELO = "base" para modificar el balance entre precisión y velocidad:

1. tiny (75 MB): Muy rápido / Precisión básica.

2. base (150 MB): Rápido / Buena precisión.

3. small (500 MB): Moderado / Muy buena precisión.

## 🔧 Solución de Problemas (Troubleshooting)

| Problema | Causa Probable | Solución |
| :--- | :--- | :--- |
| **Error de conexión** | IP incorrecta / Servidor inactivo | Asegúrate de que `servidor.py` esté corriendo y que el ESP32 tenga la misma IP local. |
| **HTTP 500** | Error interno del servidor | Revisa la consola del script de Python para ver el detalle del error de ejecución. |
| **ERROR: no hay memoria** | Búfer excesivo | Reduce `RECORD_SECS` a 2 o 3. |
| **Error al parsear JSON** | Respuesta inválida del servidor | El servidor no devolvió JSON válido. Verifica la respuesta en el monitor serial. |
| **Grabación en blanco** | Falla de hardware | Ejecuta el script de diagnóstico `prueba_sonido_microfono` primero para verificar el INMP441. |