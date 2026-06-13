# Sketch — Transcripción de voz

Graba audio con el INMP441 al tocar el sensor táctil,
lo envía como WAV a un servidor Python en tu computadora
y muestra la transcripción recibida en el monitor serial.

```
ESP32 ──[WiFi]──► servidor.py ──[Whisper]──► {"text": "hola mundo"}
  ▲                                                      │
  └──────────────────────────────────────────────────────┘
```

---

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `transcripcion.ino` | Sketch del ESP32 |
| `servidor/servidor.py` | Servidor Flask + Whisper |
| `servidor/requirements.txt` | Dependencias Python |

---

## Paso 1 — Instalar el servidor

Necesitas Python 3.9 o superior.

```bash
cd servidor/
pip install -r requirements.txt
```

> La primera vez descarga el modelo Whisper (~150 MB para "base").
> Solo ocurre una vez; las siguientes ejecuciones son instantáneas.

---

## Paso 2 — Obtener la IP de tu computadora

El ESP32 necesita saber la IP de tu PC en la red local.

**Windows:**
```
ipconfig
# busca "Dirección IPv4" en tu adaptador WiFi, ej: 192.168.1.105
```

**Mac / Linux:**
```
ip addr   # o ifconfig
```

---

## Paso 3 — Iniciar el servidor

```bash
python servidor.py
```

Verás en la consola:
```
IP local de este equipo: 192.168.1.105
Configura el ESP32 con:  SERVER_URL = "http://192.168.1.105:5000/transcribir"
```

Verifica que el servidor responde:
```
curl http://192.168.1.105:5000/estado
# → {"estado": "ok", "modelo": "base", "idioma": "es"}
```

---

## Paso 4 — Configurar el sketch

Abre `transcripcion.ino` y edita el arreglo `networks[]` con tus credenciales:

```cpp
WiFiCredential networks[] = {
  // WPA2-Personal (red doméstica)
  {"NOMBRE_DE_TU_RED",  "TU_CONTRASEÑA",  false, "", ""},
  // WPA2-Enterprise (red universitaria — opcional)
  {"RED_UNIVERSITARIA", "",               true,
   "usuario@universidad.edu", "contraseña_eap"},
};
```

> Puedes agregar tantas redes como necesites. El ESP32 las intentará en orden hasta conectarse.
> Si solo usas redes personales (WPA2-Personal), deja `enterprise` en `false` y borra la entrada Enterprise.

Edita también la URL del servidor:

```cpp
const char* SERVER_URL = "http://192.168.1.105:5000/transcribir";  // IP de tu PC
```

---

## Paso 5 — Instalar la librería ArduinoJson

```bash
arduino-cli lib install "ArduinoJson"
```

---

## Paso 6 — Compilar y subir

```bash
# Desde la carpeta del sketch
arduino-cli compile --fqbn esp32:esp32:esp32c6 .
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 .
arduino-cli monitor -p COM3 --config baudrate=115200
```

---

## Uso

1. Con el servidor corriendo, abre el monitor serial (115200 baud)
2. **Toca el sensor táctil** → escucharás un beep corto
3. Habla durante 3 segundos
4. Escucharás un beep largo → grabación terminada
5. El ESP32 envía el audio al servidor
6. En el monitor serial aparecerá:

```
Grabando 3 segundos...
Grabados 96000 bytes
Enviando 96044 bytes a http://192.168.1.105:5000/transcribir
Respuesta HTTP: 200
JSON recibido: {"text":"hola mundo como están","idioma":"es","duracion_s":2.94}

┌─────────────────────────────┐
│ Transcripcion: hola mundo como están
└─────────────────────────────┘
```

---

## Cambiar la duración de grabación

En `transcripcion.ino`:
```cpp
#define RECORD_SECS  3    // segundos — aumentar si necesitas más tiempo
```

> Cada segundo agrega ~32 KB al buffer. Con 3 s usa ~96 KB del heap del ESP32.
> El máximo recomendado sin PSRAM es **5 segundos** (~160 KB).

---

## Cambiar el modelo de Whisper

En `servidor/servidor.py`:
```python
MODELO = "base"   # tiny | base | small | medium | large
```

| Modelo | Tamaño | Velocidad | Precisión |
|--------|--------|-----------|-----------|
| tiny | 75 MB | muy rápido | básica |
| base | 150 MB | rápido | buena |
| small | 500 MB | moderado | muy buena |
| medium | 1.5 GB | lento | excelente |

---

## Solución de problemas

| Problema | Causa | Solución |
|----------|-------|----------|
| `Error de conexión` | IP incorrecta o servidor no activo | Verifica que `servidor.py` esté corriendo y la IP sea correcta |
| `HTTP 500` | Error en el servidor | Revisa la consola del servidor para ver el error |
| `no hay memoria` | Buffer muy grande | Reduce `RECORD_SECS` a 2 |
| Grabación en blanco | Micrófono mal conectado | Corre `prueba_sonido_microfono` primero |
| `Error al parsear JSON` | El servidor no devolvió JSON válido | Verifica la respuesta del servidor en el monitor serial |

---

## ¿Qué aprendiste?

Este sketch demuestra un pipeline IoT completo:

```
Sensor físico (INMP441)
  → Microcontrolador graba audio en RAM
  → Empaqueta como WAV y envía por HTTP POST
  → Servidor Python recibe y transcribe con Whisper
  → Devuelve JSON con el texto
  → ESP32 muestra la transcripción
```

Cada eslabón de esa cadena es reutilizable. El servidor podría ser cualquier API. El audio podría ser cualquier input. El resultado podría mostrarse en cualquier pantalla.

---

## Reto propuesto

Ahora que entiendes cómo funciona la cadena HTTP → servidor → JSON → pantalla, el siguiente paso natural es: **¿qué pasa si el servidor no eres tú?**

**[Reto 10 — Tu propia API](../retos/reto-10-nueva-api/)** te desafía a conectar el ESP32 a una API pública de tu elección y mostrar sus datos en la OLED. No necesitas grabar audio — solo WiFi, una petición HTTP GET, y parsear el JSON que regresa.

Es el mismo patrón que acabas de aprender, sin el servidor intermedio.
