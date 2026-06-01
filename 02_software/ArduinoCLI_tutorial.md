# Tutorial — Arduino CLI con ESP32-C6 en VS Code

Esta guía explica cómo instalar y usar Arduino CLI para compilar y subir código al **ESP32-C6 Supermini** sin necesidad del IDE de Arduino. Todo desde la terminal o desde VS Code.

---

## ¿Por qué Arduino CLI?

El IDE de Arduino es fácil para empezar, pero cuando trabajas en proyectos más serios necesitas poder:
- Compilar desde la terminal (y scripts).
- Integrar con VS Code y sus tareas.
- Controlar exactamente qué versión de librería usas.

Arduino CLI te da todo eso en un solo ejecutable.

---

## 1. Instalar Arduino CLI

### Windows

```powershell
winget install --id ArduinoSA.CLI -e --source winget
```

Cierra y vuelve a abrir la terminal. Verifica:

```powershell
arduino-cli version
# → arduino-cli  Version: 0.35.x  ...
```

Si el comando no se reconoce, prueba con la ruta completa:

```powershell
& "C:\Program Files\Arduino CLI\arduino-cli.exe" version
```

### Mac

```bash
brew install arduino-cli
arduino-cli version
```

### Linux

```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/
arduino-cli version
```

---

## 2. Instalar el core de ESP32

El "core" es el paquete que le enseña a Arduino CLI cómo compilar para ESP32. Solo se instala una vez.

```powershell
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

La instalación descarga varios cientos de MB. Cuando termine, verifica que el ESP32-C6 esté disponible:

```powershell
arduino-cli board listall | grep esp32c6
# → esp32:esp32:esp32c6   ESP32-C6 Dev Module
```

---

## 3. Instalar las librerías

Las librerías son código externo que tu sketch necesita. Para Mochi, instala estas tres:

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
arduino-cli lib install "U8g2"
```

Verifica que quedaron instaladas:

```powershell
arduino-cli lib list
```

Deberías ver las tres en la lista.

---

## 4. Conectar el ESP32 e identificar el puerto

Conecta el ESP32-C6 por USB a tu computadora y ejecuta:

```powershell
arduino-cli board list
```

Busca en la salida una línea que incluya `esp32c6`. El puerto será algo como `COM3`, `COM4` (Windows) o `/dev/ttyUSB0` (Linux/Mac).

```
Port    Protocol  Type              Board Name        FQBN
COM3    serial    Serial Port USB   ESP32-C6 Dev Mod  esp32:esp32:esp32c6
```

> Si no aparece ningún ESP32, puede ser que el driver USB no esté instalado. Ver la guía de [errores comunes](../docs/errores-comunes.md).

---

## 5. Compilar el firmware

Desde la raíz del repositorio:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

**¿Qué significa ese FQBN?**

| Parte | Significado |
|-------|------------|
| `esp32` | Fabricante del core |
| `esp32` | Familia de chips |
| `esp32c6` | Modelo específico (ESP32-C6) |
| `PartitionScheme=huge_app` | Usa partición grande (obligatorio para Mochi) |

Una compilación exitosa termina con algo como:
```
Sketch uses 987654 bytes (47%) of program storage space.
Global variables use 54321 bytes (8%) of dynamic memory.
```

Si ves `Image length ... doesn't fit in partition length`, es porque usaste el FQBN sin `PartitionScheme=huge_app`.

---

## 6. Subir al ESP32

Reemplaza `COM3` con el puerto que encontraste en el paso 4:

```powershell
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5
```

Una subida exitosa termina con:
```
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

Si ves `A fatal error occurred: Failed to connect to ESP32-C6`:
- Presiona y mantén el botón **BOOT** del ESP32 mientras ejecutas el comando.
- Suelta el botón cuando empiece la subida.

---

## 7. Abrir el monitor serial

El firmware usa `Serial.begin(115200)`. Abre el monitor a esa velocidad:

```powershell
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

Para salir del monitor, usa `Ctrl+C`.

> **Importante:** No puedes subir código mientras el monitor serial está abierto. El monitor bloquea el puerto. Ciérralo antes de volver a subir.

---

## 8. Comandos de uso diario

```powershell
# Ver la versión instalada
arduino-cli version

# Listar los ESP32 conectados
arduino-cli board list

# Compilar Mochi
arduino-cli compile --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5

# Subir Mochi (reemplaza COM3 con tu puerto)
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6:PartitionScheme=huge_app 03_firmware/mochi_unified_5

# Abrir monitor serial
arduino-cli monitor -p COM3 -b esp32:esp32:esp32c6:PartitionScheme=huge_app -c baudrate=115200
```

---

## 9. Compilar un sketch de prueba

Los sketches de prueba están en `03_firmware/pruebas/`. Para compilar uno:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/prueba_buzzer
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/prueba_buzzer
```

> Los sketches de prueba **no necesitan** `PartitionScheme=huge_app` porque son pequeños.

---

## 10. Usar VS Code con tareas preconfiguradas

El repositorio incluye `.vscode/tasks.json` con tareas listas:

| Tarea | Acción |
|-------|--------|
| `Arduino: Compilar ESP32-C6 (huge_app)` | Compila el firmware principal |
| `Arduino: Subir al ESP32-C6` | Sube al puerto que tú indiques |
| `Arduino: Monitor Serial (115200)` | Abre el monitor |
| `Arduino: Compilar + Subir` | Hace ambos en secuencia |

Para usarlas en VS Code:
1. Abre el repositorio: `File → Open Folder`
2. Ve a `Terminal → Run Task`
3. Selecciona la tarea que necesitas

O usa `Ctrl+Shift+B` para compilar directamente.

---

## Errores comunes y soluciones rápidas

| Error | Solución |
|-------|---------|
| `arduino-cli: command not found` | Cierra y vuelve a abrir la terminal, o usa la ruta completa |
| `Error: 13 FQBN not recognized` | Verifica que el core está instalado: `arduino-cli core list` |
| `Image length ... doesn't fit` | Agrega `PartitionScheme=huge_app` al FQBN |
| `Could not open COM3, the port is busy` | Cierra el monitor serial antes de subir |
| `A fatal error: Failed to connect` | Presiona BOOT en el ESP32 mientras ejecutas el upload |
| `lib not found` | Instala la librería faltante con `arduino-cli lib install` |

Guía de errores completa → [../docs/errores-comunes.md](../docs/errores-comunes.md)
