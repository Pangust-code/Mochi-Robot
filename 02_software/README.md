# Sección 1 — Preparación del entorno

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

## Paso 4 — Instalar el driver USB

Si el ESP32 no aparece como puerto COM al conectarlo:

- **CH340:** [wch-ic.com → CH341SER.EXE](https://www.wch-ic.com/downloads/CH341SER_EXE.html)
- **CP2102:** [silabs.com → VCP Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

Desconecta y vuelve a conectar el ESP32 después de instalar.

---

## Paso 5 — Verificar el puerto

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
| `Arduino: Monitor Serial (115200)` | Abre el monitor |
| `Arduino: Compilar + Subir` | Ambos en secuencia |

> Ve a **Terminal → Run Task** para ejecutarlas.

---

## ✅ Checklist — Sección 1

- [ ] `arduino-cli version` muestra una versión
- [ ] `arduino-cli board listall | grep esp32c6` devuelve resultados
- [ ] `arduino-cli lib list` muestra GFX, SH110X y U8g2
- [ ] El ESP32 aparece como puerto COM al conectarlo por USB

---

## Referencia adicional

Para más detalle sobre cada comando y solución de errores específicos de esta sección:
👉 [ArduinoCLI_tutorial.md](ArduinoCLI_tutorial.md)

---

**← Anterior:** [01_hardware/](../01_hardware/) — conexiones del hardware
**Siguiente →** [03_firmware/](../03_firmware/) — compilar y cargar el firmware
