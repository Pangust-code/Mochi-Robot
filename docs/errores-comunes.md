# Resolución de problemas — Mochi Robot

Esta guía recopila los errores más frecuentes al trabajar con Mochi, organizados por etapa del proceso.

---

## Tabla de diagnóstico rápido

| Síntoma | Sección |
|---------|---------|
| El ESP32 no aparece por USB | [→ Problemas de conexión USB](#problemas-de-conexión-usb) |
| Error `sketch too big` | [→ Errores de compilación](#errores-de-compilación) |
| Librería no encontrada | [→ Errores de compilación](#errores-de-compilación) |
| Error al subir / `Failed to connect` | [→ Errores al subir el firmware](#errores-al-subir-el-firmware) |
| Puerto ocupado / `port is busy` | [→ Errores al subir el firmware](#errores-al-subir-el-firmware) |
| Pantalla en blanco | [→ Problemas de hardware](#problemas-de-hardware) |
| GIFs no se reproducen | [→ Problemas de hardware](#problemas-de-hardware) |
| Reloj no sincroniza | [→ Problemas de hardware](#problemas-de-hardware) |
| Micrófono sin respuesta | [→ Problemas de hardware](#problemas-de-hardware) |
| Buzzer no suena | [→ Problemas de hardware](#problemas-de-hardware) |
| Sensor táctil no responde | [→ Problemas de hardware](#problemas-de-hardware) |

---

## Problemas de conexión USB

### El ESP32 no aparece como puerto COM

**Síntoma:** `arduino-cli board list` no muestra ningún dispositivo, o aparece solo el puerto sin nombre de placa.

**Causa 1 — Driver USB no instalado**

El chip USB-serie del ESP32-C6 Supermini puede ser CH340 o CP2102. El sistema operativo necesita el driver específico.

- **Windows:** descarga e instala el driver [CH340](https://www.wch-ic.com/downloads/CH341SER_EXE.html) o [CP2102](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers). Reinicia el PC después de instalar.
- **Mac:** macOS 10.14+ incluye CH340. Para CP2102, usa el driver de Silicon Labs.
- **Linux:** Los drivers suelen estar incluidos. Agrega tu usuario al grupo `dialout`: `sudo usermod -a -G dialout $USER` y reinicia sesión.

**Causa 2 — Cable USB solo para carga**

Algunos cables USB-C son solo para energía (no tienen pines de datos). Usa un cable que soporte transferencia de datos.

**Causa 3 — ESP32 en modo de bajo consumo**

Prueba presionar el botón **RST** del ESP32 y reconecta el cable.

---

### El dispositivo aparece pero `arduino-cli` no lo reconoce

**Síntoma:** El puerto COM aparece en el Administrador de dispositivos pero `arduino-cli board list` muestra `Unknown`.

Esto es normal con algunos chips CH340. El ESP32-C6 sigue ahí — solo usa el puerto manualmente:

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

---

## Errores de compilación

### `Image length ... doesn't fit in partition length`

**Causa:** El firmware de Mochi es grande y no cabe en la partición de programa por defecto.

**Solución:** Siempre compila y sube con `PartitionScheme=huge_app`:

```powershell
# ❌ Incorrecto
arduino-cli compile --fqbn esp32:esp32:esp32c6 .

# ✅ Correcto
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app .
```

---

### `fatal error: Adafruit_SH110X.h: No such file or directory`

**Causa:** La librería `Adafruit SH110X` no está instalada.

**Solución:**
```powershell
arduino-cli lib install "Adafruit SH110X"
arduino-cli lib install "Adafruit GFX Library"
```

---

### `fatal error: U8g2lib.h: No such file or directory`

**Causa:** La librería `U8g2` no está instalada. Es necesaria para el Modo 1 (GIFs).

**Solución:**
```powershell
arduino-cli lib install "U8g2"
```

---

### `Error: 13 FQBN ... not recognized`

**Causa:** El core de ESP32 no está instalado o el FQBN es incorrecto.

**Solución:**
```powershell
# Instalar el core
arduino-cli core update-index
arduino-cli core install esp32:esp32

# Verificar que el ESP32-C6 está disponible
arduino-cli board listall | grep esp32c6
```

Para el ESP32-C6 el FQBN correcto es: `esp32:esp32:esp32c6`  
(No `esp32:esp32:esp32`, que es para el ESP32 clásico WROOM.)

---

### `'ssid' was not declared in this scope` u otros errores de variable

**Causa:** El archivo `.ino` está ubicado en una carpeta con nombre distinto al del archivo.

Arduino CLI requiere que la carpeta y el `.ino` tengan el mismo nombre. Por ejemplo:
```
✅ mochi_unified_5/mochi_unified_5.ino
❌ firmware/mochi_unified_5.ino
```

---

## Errores al subir el firmware

### `A fatal error occurred: Failed to connect to ESP32-C6`

**Causa:** El ESP32 no entró en modo de carga (bootloader).

**Solución:**
1. Presiona y **mantén** el botón **BOOT** del ESP32.
2. Sin soltar BOOT, ejecuta el comando de upload.
3. Cuando veas `Connecting...`, suelta el botón BOOT.

---

### `Could not open COM3, the port is busy`

**Causa:** El monitor serial (u otro programa como Arduino IDE) tiene el puerto abierto.

**Solución:**
- Cierra el monitor serial con `Ctrl+C`.
- Cierra Arduino IDE si está abierto.
- Cierra cualquier terminal que esté usando ese puerto.
- Vuelve a ejecutar el upload.

---

### La subida parece exitosa pero el robot no responde

**Causa probable:** El firmware subido puede ser de un sketch de prueba en lugar del firmware principal.

**Verificación:** Abre el monitor serial y revisa qué mensajes aparecen:
```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

El firmware principal (`mochi_unified_5`) debe mostrar:
```
=== MOCHI v6 ===
LittleFS OK
-> Modo 0 (Mascota)
```

---

## Problemas de hardware

### Pantalla OLED en blanco

**Síntoma:** El ESP32 enciende (el LED parpadea o el monitor serial funciona) pero la pantalla no muestra nada.

| Causa | Verificación | Solución |
|-------|-------------|---------|
| Cable desconectado | Revisa SDA y SCL | SDA = GPIO 6, SCL = GPIO 7 |
| Voltaje incorrecto | Mide con voltímetro | Usa 3.3V, nunca 5V |
| Dirección I2C incorrecta | Sube scanner I2C | La dirección debe ser `0x3C` |
| Pantalla defectuosa | Prueba en otra | Reemplaza la pantalla |

Para diagnosticar aislado, sube el sketch `prueba_pantalla`:
```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/01_prueba_pantalla
```

---

### GIFs no se reproducen (Modo 1 en blanco)

**Síntoma:** El Modo 1 muestra una pantalla vacía o el mensaje `LittleFS: error. Sube los .bin primero.`

**Causa:** Los archivos `.bin` de animación no están en la memoria flash del ESP32.

**Solución:** Sube el sistema de archivos (ver Sección 2.5 del README principal). Los archivos `.bin` se encuentran en `03_firmware/mochi_unified_5/data/`.

En el monitor serial, tras iniciar correctamente verás:
```
LittleFS OK — 42/128 KB
```

Si ves `LittleFS: error`, los archivos no están subidos.

---

### El reloj (Modo 2) no muestra la hora

**Síntoma:** La pantalla muestra `WiFi: error` o la hora no cambia.

**Causas posibles:**

1. **Credenciales incorrectas** — edita `ssid` y `password` en `mochi_unified_5.ino`:
   ```cpp
   const char* ssid     = "TU_RED_WIFI";
   const char* password = "TU_CONTRASEÑA";
   ```
   Vuelve a compilar y subir.

2. **Red no disponible** — el ESP32 debe estar cerca del router durante el encendido.

3. **Red de 5 GHz** — el ESP32-C6 solo se conecta a redes de **2.4 GHz**. Si tu router tiene bandas separadas, conecta al SSID de 2.4 GHz.

4. **Servidor NTP bloqueado** — en algunas redes corporativas o escolares, el tráfico NTP puede estar bloqueado. Contacta al administrador de red o usa una red diferente.

---

### El micrófono no responde (barra siempre vacía o siempre llena)

**Síntoma en Modo 4:** La barra de volumen no se mueve o está siempre al máximo.

| Síntoma | Causa | Solución |
|---------|-------|---------|
| Barra siempre en cero | Micrófono desconectado | Verifica WS (GPIO 5), SCK (GPIO 4), SD (GPIO 3) |
| Barra siempre llena | Pin L/R no está a GND | Conecta el pin L/R del INMP441 a GND |
| Barra con ruido constante | Interferencia eléctrica | Aleja el micrófono de cables de alimentación |

Para diagnosticar, sube `prueba_sonido_microfono` y abre el monitor serial:
```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/08_prueba_sonido_microfono
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/08_prueba_sonido_microfono
```

---

### El buzzer no suena o solo emite un tono fijo

| Síntoma | Causa | Solución |
|---------|-------|---------|
| No suena nada | Cable desconectado | Verifica GPIO 19 (positivo) y GND (negativo) |
| Solo un tono fijo (no cambia) | Buzzer activo, no pasivo | Reemplaza por un buzzer **pasivo** |
| Sonido distorsionado | Voltaje incorrecto | Verifica que el positivo va a GPIO 19 (salida PWM) |

Para diagnosticar, sube `prueba_buzzer`:
```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/03_prueba_buzzer
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/03_prueba_buzzer
```

El sketch ejecuta un sweep de frecuencias. Si el tono no cambia de pitch, es un buzzer activo.

---

### El sensor táctil no responde

**Síntoma:** Tocar el sensor no produce ningún efecto.

| Causa | Verificación | Solución |
|-------|-------------|---------|
| Cable desconectado | Revisa la conexión | OUT = GPIO 2, VCC = 3.3V, GND = GND |
| Toca muy rápido | El firmware tiene debounce | Toca de forma deliberada (no muy rápido) |
| Sensibilidad alta | Sensor activa sin tocarlo | Aleja cables y manos del sensor cuando no lo uses |

Para diagnosticar, sube `prueba_touch`:
```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/02_prueba_touch
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/02_prueba_touch
```

---

## Problemas con Arduino CLI

### `arduino-cli: command not found` (o `not recognized`)

**Windows:** Cierra la terminal y vuelve a abrirla para que cargue el nuevo PATH. Si persiste, usa la ruta completa:
```powershell
& "C:\Program Files\Arduino CLI\arduino-cli.exe" version
```

**Linux/Mac:** Verifica que el binario está en el PATH:
```bash
which arduino-cli
echo $PATH | tr ':' '\n'
```

---

### `winget install` no encuentra el paquete

El nombre correcto del paquete para Arduino CLI en winget es `ArduinoSA.CLI`:
```powershell
winget install --id ArduinoSA.CLI -e --source winget
```

---

### La librería instalada no es la versión correcta

Para instalar una versión específica:
```powershell
arduino-cli lib install "Adafruit SH110X@2.1.10"
```

Para listar versiones disponibles:
```powershell
arduino-cli lib search "Adafruit SH110X"
```

---

## Matriz de diagnóstico por componente

| # | Componente | Carpeta | ¿Qué verifica? |
|---|-----------|---------|---------------|
| 1 | Pantalla OLED | `03_firmware/pruebas/01_prueba_pantalla` | Texto, píxeles, dirección I2C |
| 2 | Sensor táctil | `03_firmware/pruebas/02_prueba_touch` | Lectura HIGH/LOW del pin GPIO 2 |
| 3 | Buzzer | `03_firmware/pruebas/03_prueba_buzzer` | Sweep de frecuencias, escala, tonos del firmware |
| 5 | WiFi | `03_firmware/pruebas/05_prueba_wifi` | Conexión y asignación de IP |
| 6 | LittleFS | `03_firmware/pruebas/06_prueba_littlefs` | Lectura de archivos en la flash |
| 7 | Micrófono (conexión) | `03_firmware/pruebas/07_prueba_conexion_microfono` | Detecta si los pines I2S están conectados |
| 8 | Micrófono (sonido) | `03_firmware/pruebas/08_prueba_sonido_microfono` | Amplitud, barra de volumen en tiempo real |

> El número de carpeta indica el orden recomendado. Completa del 01 al 08 antes de subir el firmware principal.

**Orden de diagnóstico:**
1. `01_prueba_pantalla` → confirmar que la OLED funciona (necesaria para ver resultados)
2. `02_prueba_touch` → confirmar que el sensor táctil responde
3. `03_prueba_buzzer` → confirmar que el buzzer es pasivo y funciona
4. `05_prueba_wifi` → confirmar conectividad WiFi (para el modo reloj)
5. `06_prueba_littlefs` → confirmar que los archivos .bin están en la flash
6. `07_prueba_conexion_microfono` → verificar pines del INMP441
7. `08_prueba_sonido_microfono` → verificar que el micrófono capta audio
