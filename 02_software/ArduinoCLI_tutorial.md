# Referencia rápida — Arduino IDE con ESP32

Resumen de operaciones frecuentes en Arduino IDE para el proyecto Mochi.

---

## Compilar y subir

1. Abre el archivo `.ino` del sketch en Arduino IDE
2. Verifica que el board y el puerto están configurados correctamente en **Tools**
3. Usa **Sketch → Upload** (o `Ctrl+U`) para compilar y subir en un solo paso

Si solo quieres verificar que compila sin subir: **Sketch → Verify/Compile** (`Ctrl+R`).

---

## Abrir el Monitor Serial

Ve a **Tools → Serial Monitor** (o `Ctrl+Shift+M`). Asegúrate de que la velocidad esté en **115200 baud**.

> No puedes subir código mientras el Monitor Serial está abierto. Ciérralo antes de volver a subir.

---

## Configuración del board por microcontrolador

### ESP32-C6 Supermini

| Parámetro | Valor |
|-----------|-------|
| Board | `ESP32-C6 Dev Module` |
| USB CDC On Boot | `Enabled` |
| Partition Scheme | `Huge APP (3MB No OTA / 1MB SPIFFS)` |
| Upload Speed | `115200` |

### ESP32-C3 Super Mini

| Parámetro | Valor |
|-----------|-------|
| Board | `ESP32-C3 Dev Module` |
| USB CDC On Boot | `Enabled` |
| Partition Scheme | `Huge APP (3MB No OTA / 1MB SPIFFS)` |
| Upload Speed | `115200` |

---

## Subir los archivos LittleFS (GIFs y datos)

1. Coloca los archivos `.bin` en la subcarpeta `data/` del sketch
2. Selecciona el board y el puerto en **Tools**
3. Ve a **Sketch → Upload LittleFS to ESP32**

> Si no aparece esa opción, el plugin LittleFS no está instalado. Ver [README.md](README.md) Paso 6.

---

## Errores comunes

| Error | Causa | Solución |
|-------|-------|---------|
| `Image length doesn't fit in partition` | Partition Scheme incorrecto | Cambia a `Huge APP (3MB No OTA / 1MB SPIFFS)` |
| `Failed to connect to ESP32` | El ESP32 no entró en modo bootloader | Mantén presionado el botón **BOOT** del ESP32 mientras el IDE intenta conectar; suéltalo cuando empiece la subida |
| No aparece puerto COM | Driver USB no instalado | Instala el driver CP210X (ver README.md Paso 2) |
| Monitor Serial en blanco | USB CDC On Boot desactivado | Cambia USB CDC On Boot a `Enabled` en Tools |
| `Adafruit_GFX.h: No such file` | Librería no instalada | Instala Adafruit GFX Library desde el Library Manager |

---

**← Volver al README:** [README.md](README.md)
