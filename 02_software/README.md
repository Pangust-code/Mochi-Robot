# Sección 2 — Preparación del entorno

> **Antes de empezar:** Asegúrate de tener el repositorio clonado.
> ```bash
> git clone https://github.com/Pangust-code/Mochi-Robot.git
> cd Mochi-Robot
> ```

Esta carpeta contiene el tutorial completo de Arduino CLI y las tareas de VS Code preconfiguradas. Sigue los pasos en orden.

---

## ¿Qué vas a instalar?

| Herramienta | Para qué sirve |
|-------------|----------------|
| **Arduino CLI** | Compilar y subir código desde la terminal (sin IDE de Arduino) |
| **Core ESP32** | Enseña a Arduino CLI a compilar para el ESP32-C6 |
| **Adafruit GFX Library** | Motor gráfico para la pantalla OLED |
| **Adafruit SH110X** | Driver del controlador SH1106 |
| **U8g2** | Renderiza los GIFs en el Modo 1 |
| **LittleFS** | Sistema de archivos en flash del ESP32 (imágenes, datos) |
| **Driver USB** | Para que el PC reconozca el ESP32 al conectarlo |

---

## Paso 1 — Instalar Arduino CLI

**Windows:**
```powershell
winget install --id ArduinoSA.CLI -e --source winget
```
Cierra y vuelve a abrir la terminal.

**Mac:**
```bash
brew install arduino-cli
```

**Linux:**
```bash
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
sudo mv bin/arduino-cli /usr/local/bin/
```

**Verificar:**
```powershell
arduino-cli version
# → arduino-cli  Version: 0.35.x  ...
```

> Si en Windows el comando no se reconoce: `& "C:\Program Files\Arduino CLI\arduino-cli.exe" version`

---

## Paso 2 — Instalar el core de ESP32

```powershell
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

Descarga varios cientos de MB — solo se hace una vez.

**Verificar que el ESP32-C6 está disponible:**
```powershell
arduino-cli board listall | grep esp32c6
# → esp32:esp32:esp32c6   ESP32-C6 Dev Module
```

---

## Paso 3 — Instalar las librerías

```powershell
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "Adafruit SH110X"
arduino-cli lib install "U8g2"
```

**Verificar:**
```powershell
arduino-cli lib list
# Deben aparecer las tres librerías
```

---

## Paso 4 — Instalar LittleFS

El firmware de Mochi almacena archivos en la flash del ESP32 usando LittleFS. Necesitas la herramienta de subida de datos.

### 4.1 — Plugin para Arduino IDE (si usas IDE)

Sigue el tutorial en video:
**👉 [Instalar LittleFS en Arduino IDE — YouTube](https://www.youtube.com/watch?v=vICDKOLizrU)**

Pasos resumidos:
1. Descarga el archivo `.zip` del plugin desde el enlace del video.
2. Descomprime en la carpeta `tools/` dentro de tu sketchbook de Arduino.
   - Windows: `Documents\Arduino\tools\`
   - Mac/Linux: `~/Arduino/tools/`
3. Reinicia el Arduino IDE.
4. Verifica que aparece **Tools → ESP32 LittleFS Data Upload**.

### 4.2 — Arduino CLI / VS Code (línea de comandos)

Con Arduino CLI no hay plugin visual; la subida del filesystem se hace con `esptool.py` y `mklittlefs`, ambos incluidos en el core de ESP32.

El repositorio ya tiene una tarea de VS Code preconfigurada para esto (ver sección de tareas más abajo).

---

## Paso 5 — Configuración del board (ESP32-C6)

Antes de compilar o subir, el board debe configurarse con estas opciones. Son críticas para que el firmware funcione correctamente.

| Parámetro | Valor requerido |
|-----------|----------------|
| **USB CDC On Boot** | `Enabled` |
| **Partition Scheme** | `Huge APP (3MB No OTA / 1MB SPIFFS)` |
| **Upload Speed** | `115200` |

### En Arduino CLI (FQBN completo)

Usa siempre este FQBN al compilar y subir:

```powershell
arduino-cli compile --fqbn "esp32:esp32:esp32c6:CDCOnBoot=cdc,PartitionScheme=huge_app" .
arduino-cli upload  --fqbn "esp32:esp32:esp32c6:CDCOnBoot=cdc,PartitionScheme=huge_app" --port COM3 .
```

> Reemplaza `COM3` con el puerto real de tu ESP32 (ver Paso 6).

Las tareas de VS Code ya incluyen este FQBN — no necesitas escribirlo a mano.

### En Arduino IDE

1. **Tools → Board → ESP32 Arduino → ESP32-C6 Dev Module**
2. **Tools → USB CDC On Boot → Enabled**
3. **Tools → Partition Scheme → Huge APP (3MB No OTA / 1MB SPIFFS)**
4. **Tools → Upload Speed → 115200**

> **¿Por qué USB CDC On Boot: Enabled?**
> Activa el puerto serie nativo del ESP32-C6 por USB. Sin esto, el monitor serial y los logs de arranque no se muestran correctamente.

---

## Paso 6 — Instalar el driver USB

Si el ESP32 no aparece como puerto COM al conectarlo:

- **CH340:** [wch-ic.com → CH341SER.EXE](https://www.wch-ic.com/downloads/CH341SER_EXE.html)
- **CP2102:** [silabs.com → VCP Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

Desconecta y vuelve a conectar el ESP32 después de instalar.

---

## Paso 7 — Verificar el puerto

Conecta el ESP32 por USB y ejecuta:
```powershell
arduino-cli board list
# → COM3   esp32:esp32:esp32c6   ...
```

Anota el puerto (`COM3`, `COM4`, `/dev/ttyUSB0`…). Lo usarás en la siguiente sección.

---

## VS Code — Tareas preconfiguradas

El repositorio incluye [`.vscode/tasks.json`](../.vscode/tasks.json) con estas tareas listas:

| Tarea | Acción |
|-------|--------|
| `Arduino: Compilar ESP32-C6 (huge_app)` | `Ctrl+Shift+B` |
| `Arduino: Subir al ESP32-C6` | Sube al puerto indicado |
| `Arduino: Monitor Serial (115200)` | Abre el monitor a 115200 baud |
| `Arduino: Compilar + Subir` | Ambos en secuencia |
| `Arduino: Subir LittleFS` | Sube la carpeta `data/` al filesystem |

> Ve a **Terminal → Run Task** para ejecutarlas.

---

## ✅ Checklist — Sección 2

- [ ] `arduino-cli version` muestra una versión
- [ ] `arduino-cli board listall | grep esp32c6` devuelve resultados
- [ ] `arduino-cli lib list` muestra GFX, SH110X y U8g2
- [ ] Plugin LittleFS instalado (IDE) o tarea VS Code disponible
- [ ] USB CDC On Boot configurado en `Enabled`
- [ ] Partition Scheme configurado en `Huge APP (3MB No OTA / 1MB SPIFFS)`
- [ ] Upload Speed configurado en `115200`
- [ ] El ESP32 aparece como puerto COM al conectarlo por USB

---

## Referencia adicional

Para más detalle sobre cada comando y solución de errores específicos de esta sección:
👉 [ArduinoCLI_tutorial.md](ArduinoCLI_tutorial.md)

---

**← Anterior:** [01_hardware/](../01_hardware/) — conexiones del hardware
**Siguiente →** [03_firmware/](../03_firmware/) — compilar y cargar el firmware
