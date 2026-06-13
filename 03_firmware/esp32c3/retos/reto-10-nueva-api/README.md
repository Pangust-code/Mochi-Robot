# Reto 10 — Tu propia API

**Nivel:** ⭐⭐ Medio  
**Prerrequisito:** Prueba 08 (WiFi) funcionando. Opcional: haber completado la prueba 08 de clima, que ya usa este mismo patrón.

---

## Objetivo

Conecta Mochi a cualquier API REST pública y muestra sus datos en la OLED. El patrón es siempre el mismo:

```
ESP32 → HTTP GET → API externa → JSON → parsear → mostrar en OLED
```

---

## APIs gratuitas sin registro

Estas APIs funcionan directamente desde el sketch, sin crear cuenta ni API key:

| API | URL | Campo que te interesa | Ejemplo de respuesta |
|-----|-----|-----------------------|---------------------|
| Advice Slip | `api.adviceslip.com/advice` | `slip.advice` | `"Take time to smell the flowers."` |
| Open-Meteo | `api.open-meteo.com/v1/forecast?latitude=4.71&longitude=-74.07&current=temperature_2m` | `current.temperature_2m` | `18.4` |
| Frankfurter | `api.frankfurter.app/latest?from=USD&to=COP` | `rates.COP` | `4230.5` |
| Dog CEO | `dog.ceo/api/breeds/image/random` | `message` | URL de imagen (para debug) |
| Bored API | `www.boredapi.com/api/activity` | `activity` | `"Learn to play a new instrument"` |

---

## Pasos

### Paso 1 — Elige una API y entiende su JSON

Abre la URL en tu navegador. Verás el JSON que devuelve. Identifica qué campo quieres mostrar.

Por ejemplo, para Advice Slip:
```json
{
  "slip": {
    "id": 42,
    "advice": "Ask for help when you need it."
  }
}
```
El campo que quieres es `slip.advice`.

### Paso 2 — Copia el sketch de clima como base

```powershell
# El sketch de clima ya hace exactamente este patrón
# Copia y renombra:
#   03_firmware/esp32c3/pruebas/08_prueba_wifi_clima/ → tu nueva carpeta
```

El sketch de clima usa `HTTPClient` + `ArduinoJson` para hacer GET y parsear JSON. Solo debes cambiar la URL y los campos que extraes.

### Paso 3 — Cambia la URL

En tu sketch, encuentra:
```cpp
http.begin("http://api.open-meteo.com/...");
```
Reemplázala con la URL de tu API elegida.

### Paso 4 — Cambia el parsing

Para Advice Slip, el parsing cambia de:
```cpp
float temp = doc["current"]["temperature_2m"];
```
A:
```cpp
const char* consejo = doc["slip"]["advice"];
```

### Paso 5 — Compilar y verificar

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/08_prueba_wifi_clima
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/pruebas/08_prueba_wifi_clima
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c3 -c baudrate=115200
```

Verifica en el Monitor Serie que el JSON se recibe correctamente antes de adaptar la pantalla OLED.

### Paso 6 — Mostrar en OLED

Usa las mismas funciones que el sketch de clima para mostrar el texto en pantalla. Para textos largos, considera usar `setTextSize(1)` y cortar el string si supera los 128 píxeles.

---

## Variantes de mayor dificultad

| Variante | Qué agrega |
|----------|-----------|
| Auto-actualización cada 60 s | `millis()` para refrescar sin `delay()` bloqueante |
| Scroll de texto largo | Desplazar texto en pantalla con un offset X que decrementa en el `loop()` |
| Parámetros del usuario | Selector con el sensor táctil para elegir ciudad o moneda |
| Dos APIs combinadas | Mostrar temperatura Y consejo en pantalla alternando cada 5 s |

---

**← Anterior:** [Prueba 11 — Transcripción](../../pruebas/11_transcripcion/)  
**Volver a retos →** [Índice de retos](../README.md)
