# Reto 10 — Tu propia API ⭐⭐ Medio

Conecta Mochi a una API REST de tu elección y muestra sus datos en la pantalla OLED.

## Objetivo

Hasta ahora Mochi consulta OpenWeatherMap y envía audio a un servidor local. Ambos siguen el mismo patrón:

```
ESP32 ──[WiFi / HTTP GET]──► API externa ──► JSON ──► parsear ──► mostrar en OLED
```

Este reto te pide repetir ese patrón con **cualquier API que tú elijas**. El objetivo no es seguir instrucciones — es que entiendas el ciclo lo suficientemente bien para aplicarlo a un caso nuevo.

## Nivel

Medio. Requiere leer documentación de una API, hacer una petición HTTP GET, y parsear el JSON resultante.

---

## Antes de empezar

Completa la **Prueba 07 — WiFi** y, preferiblemente, la **Prueba 08 — Clima**. El código de clima es tu mejor punto de partida — tiene el WiFi, la petición HTTP, el parseo JSON y el display OLED ya funcionando.

---

## Paso 1 — Elige tu API

Algunas APIs gratuitas sin necesidad de registro:

| API | URL base | Qué devuelve | Formato |
|-----|----------|-------------|---------|
| **Advice Slip** | `api.adviceslip.com/advice` | Un consejo aleatorio en inglés | JSON: `slip.advice` |
| **Open-Meteo** | `api.open-meteo.com/v1/forecast` | Clima sin clave de API | JSON: `current.temperature_2m` |
| **Frankfurter** | `api.frankfurter.app/latest` | Tipos de cambio de divisas | JSON: `rates.USD`, `rates.EUR`... |
| **Dog CEO** | `dog.ceo/api/breeds/image/random` | URL de imagen de perro aleatorio | JSON: `message` |
| **Bored API** | `www.boredapi.com/api/activity` | Una actividad aleatoria para hacer | JSON: `activity`, `type` |

También puedes usar cualquier otra API pública que te parezca interesante.

---

## Paso 2 — Entiende qué devuelve tu API

Antes de escribir código, prueba la API en el navegador o con `curl`:

```bash
curl https://api.adviceslip.com/advice
```

Resultado:
```json
{"slip": {"id": 117, "advice": "Smile. It confuses people."}}
```

Anota:
- ¿Qué campo del JSON contiene el dato que quieres mostrar?
- ¿La URL necesita parámetros extra (ciudad, moneda, etc.)?
- ¿La API devuelve el tipo de contenido `application/json`?

---

## Paso 3 — Copia el sketch de clima como base

```powershell
# Copia el directorio
xcopy /E /I 03_firmware/mochi_unified_5/pruebas/08_prueba_clima mi_api_reto
```

O simplemente abre `08_prueba_clima/prueba_clima.ino` y trabaja sobre él.

---

## Paso 4 — Cambia la URL y el parseo

Localiza estas dos secciones en el sketch y modifícalas:

**Cambio 1 — La URL:**
```cpp
// Antes (OpenWeatherMap):
String url = "https://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + apiKey;

// Después (ejemplo: Advice Slip):
String url = "https://api.adviceslip.com/advice";
```

**Cambio 2 — El parseo del JSON:**
```cpp
// Antes (OpenWeatherMap):
float temp = doc["main"]["temp"];
String desc = doc["weather"][0]["description"];

// Después (ejemplo: Advice Slip):
String consejo = doc["slip"]["advice"];
```

**Cambio 3 — Lo que se muestra en la OLED:**
```cpp
display.clearDisplay();
display.setTextSize(1);
display.setCursor(0, 0);
display.println("Consejo del dia:");
display.println();
display.println(consejo);
display.display();
```

---

## Paso 5 — Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 .
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 .
arduino-cli monitor -p COM3 --config baudrate=115200
```

---

## Paso 6 — Verificar en el Monitor Serie

Añade `Serial.println(payload)` justo después de recibir la respuesta HTTP para ver el JSON crudo. Así puedes verificar el campo exacto que necesitas parsear antes de mostrarlo en pantalla.

---

## Variantes de mayor dificultad

Una vez que funciona la versión básica, prueba alguna de estas extensiones:

**Nivel 1 — Actualización automática**
Usa `millis()` para que la API se consulte cada 60 segundos sin bloquear el loop.

**Nivel 2 — Manejo de textos largos**
Algunos campos de JSON pueden ser más largos que la pantalla (128 px ancho). Implementa un scroll horizontal o recorta el texto con `substring(0, 20)`.

**Nivel 3 — API con parámetros del usuario**
Elige una API que acepte parámetros (ej: tipo de cambio de una moneda específica). Usa el sensor táctil para ciclar entre diferentes opciones — 1 tap = siguiente divisa, hold = actualizar.

**Nivel 4 — Dos fuentes de datos**
Combina dos APIs: muestra el clima de Open-Meteo en la mitad superior de la pantalla y un consejo de Advice Slip en la mitad inferior.

---

## Resultado esperado

El ESP32 se conecta a WiFi, consulta tu API elegida, parsea el JSON, y muestra el resultado en la OLED. Al tocar el sensor, fuerza una nueva consulta.

Si llegas hasta aquí, entiendes el ciclo completo de un dispositivo IoT conectado: hardware → firmware → red → API → datos → interfaz física.
