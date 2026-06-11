# Sketch — Código Funcional de Transcripción

Test de integración completo que une todos los componentes de Mochi con un servidor de interpretación de comandos.

```
ESP32 ──[WiFi/HTTPS]──► servidor ──[IA]──► {"mood_id": 1, "intent": "saludo"}
  ▲                                                        │
  └──────────── ojos animados + buzzer ───────────────────┘
```

A diferencia de `transcripcion` (que solo transcribe texto), este sketch **interpreta comandos**: el servidor devuelve un `mood_id` que cambia el estado de ánimo del robot en tiempo real.

---

## Diferencia con transcripcion.ino

| Característica | `transcripcion` | `codigo_funcional_transcripcion` |
|---------------|-----------------|----------------------------------|
| Formato de audio | WAV (con cabecera) | PCM16 crudo |
| Servidor | Python local (Whisper) | Servidor externo con IA |
| Respuesta | `{"text": "hola"}` | `{"mood_id": 1, "intent": "saludo"}` |
| OLED | No | Sí — ojos animados con 10 moods |
| Resultado visible | Monitor serial | Pantalla del robot |

---

## Pines

| Componente | GPIO |
|-----------|------|
| OLED SDA | 6 |
| OLED SCL | 7 |
| Sensor táctil TTP223 | 2 |
| Buzzer pasivo | 19 |
| INMP441 SCK | 4 |
| INMP441 WS | **1** ← distinto al pin 5 de otros sketches |
| INMP441 SD | 3 |

> **Atención:** el pin WS del INMP441 es GPIO **1** en este sketch, no GPIO 5.
> Ajusta el cableado o cambia `I2S_WS_PIN` si tu hardware usa el pin 5.

---

## Configuración obligatoria

Abre `codigo_funcional_transcripcion.ino` y edita el arreglo `networks[]` y la URL del servidor:

```cpp
WiFiCredential networks[] = {
  // WPA2-Personal (red doméstica)
  {"NOMBRE_DE_TU_RED",  "TU_CONTRASEÑA",  false, "", ""},
  // WPA2-Enterprise (red universitaria — opcional)
  {"RED_UNIVERSITARIA", "",               true,
   "usuario@universidad.edu", "contraseña_eap"},
};
const char* SERVER_URL = "https://TU_SERVIDOR/audio/pcm16";  // URL del servidor
```

> Puedes agregar tantas redes como necesites. El ESP32 las intentará en orden hasta conectarse.

---

## Dependencias

```bash
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
```

---

## Compilar y subir

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c6 .
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 .
arduino-cli monitor -p COM3 --config baudrate=115200
```

---

## Uso

| Gesto | Acción |
|-------|--------|
| Mantener sensor (≥ 800 ms) | Inicia grabación (ojos SUSPICIOUS + barra de nivel) |
| Soltar después de hold | Detiene grabación y envía al servidor |
| 3 taps cortos | Cambia de modo (para futuras expansiones) |

### Estados en pantalla

| Estado | Ojos | Texto inferior |
|--------|------|----------------|
| Listo | NORMAL | `hold=mic  3tap=M0` |
| Grabando | SUSPICIOUS | barra de nivel + porcentaje |
| Enviando | DIZZY | `enviando...` |
| Resultado OK | según `mood_id` | intent recibido |
| Error | SAD | código HTTP |

---

## Formato de respuesta del servidor

```json
{
  "mood_id": 1,
  "intent": "saludo"
}
```

| `mood_id` | Estado de ánimo |
|-----------|----------------|
| 0 | NORMAL |
| 1 | HAPPY |
| 2 | SURPRISED |
| 3 | SLEEPY |
| 4 | ANGRY |
| 5 | SAD |
| 6 | EXCITED |
| 7 | LOVE |
| 8 | SUSPICIOUS |
| 9 | DIZZY |

---

## Solución de problemas

| Problema | Causa probable | Solución |
|----------|---------------|----------|
| OLED no enciende | Dirección I2C incorrecta | Verifica con `prueba_pantalla` |
| WiFi: FALLO | Credenciales incorrectas | Edita el arreglo `networks[]` con SSID y contraseña correctos |
| `HTTP -1` o `-4` | Sin conexión al servidor | Verifica URL y que el servidor esté activo |
| Sin audio (rms≈0) | Micrófono mal conectado o pin WS incorrecto | Verifica GPIO 1 para WS; corre `prueba_sonido_microfono` primero |
| `HTTP 500` | Error en el servidor | Revisa los logs del servidor |
