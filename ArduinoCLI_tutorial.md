# Tutorial: usar Arduino CLI con tu ESP32 desde VS Code

Este tutorial resume cómo usar `arduino-cli` sin PlatformIO, usando VS Code y el proyecto `esp32WROOMsinIA`.

## 1. Instalar Arduino CLI

En Windows, se puede instalar con `winget`:

```powershell
winget install --id ArduinoSA.CLI -e --source winget
```

Si el comando no aparece en una terminal nueva, cierra y vuelve a abrir PowerShell o VS Code para que Windows recargue el PATH.

## 2. Verificar que Arduino CLI funciona

```powershell
arduino-cli version
```

Si Windows no encuentra el comando, prueba la ruta completa:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' version
```

## 3. Instalar el core de ESP32 y librerías

Primero actualiza el índice de paquetes e instala el core de ESP32:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' core update-index
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' core install esp32:esp32
```

Luego instala las librerías externas usadas por el sketch:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' lib install "Adafruit GFX Library"
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' lib install "Adafruit SH110X"
```

## 4. Compilar el sketch

Tu proyecto está en:

```powershell
c:\Users\User\Documents\mochi_robot\esp32WROOMsinIA
```

Para compilarlo con una partición grande, usé este FQBN:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' compile --fqbn 'esp32:esp32:esp32:PartitionScheme=huge_app' 'c:\Users\User\Documents\mochi_robot\esp32WROOMsinIA'
```

La opción `PartitionScheme=huge_app` fue necesaria porque el binario del sketch es grande y no cabía en la partición por defecto.

## 5. Subir al ESP32

Primero identifica el puerto. En mi caso apareció como `COM4`:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' board list
```

Después sube el programa:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' upload -p COM4 --fqbn 'esp32:esp32:esp32:PartitionScheme=huge_app' 'c:\Users\User\Documents\mochi_robot\esp32WROOMsinIA'
```

## 6. Abrir el Monitor Serie

Tu sketch usa:

```cpp
Serial.begin(115200);
```

Por eso el monitor debe abrirse a `115200`:

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' monitor -p COM4 -b 'esp32:esp32:esp32:PartitionScheme=huge_app' -c baudrate=115200
```

## 7. Qué hice en tu proyecto

- Leí el archivo `esp32WROOMsinIA.ino`.
- Confirmé que usa bibliotecas del core ESP32 y librerías externas de Adafruit.
- Instalé `Arduino CLI` con `winget`.
- Verifiqué que el ejecutable quedó en `C:\Program Files\Arduino CLI\arduino-cli.exe`.
- Instalé el core `esp32:esp32`.
- Verifiqué que `Adafruit GFX Library` y `Adafruit SH110X` ya estaban instaladas.
- Compilé el sketch.
- Detecté el puerto `COM4`.
- Subí el sketch al ESP32.
- Detecté que el sketch no cabía en la partición por defecto y usé `PartitionScheme=huge_app`.
- Probé el monitor serie.

## 8. Por qué salía el error

Salian varios errores distintos:

### `choco` no se reconoce
Porque `choco` no estaba instalado en tu PC. Es un gestor de paquetes externo, no viene por defecto en Windows.

### `scoop` no se reconoce
Por la misma razón: `scoop` tampoco estaba instalado.

### `arduino-cli` no se reconoce
Porque Windows todavía no tenía el comando en el PATH de la sesión abierta. El programa sí estaba instalado, pero PowerShell no lo encontraba como comando corto.

### `No package found matching input criteria`
Porque `winget search arduino-cli` no encontró el nombre simple, pero el paquete correcto en Winget era `ArduinoSA.CLI`.

### `Image length ... doesn't fit in partition length ...`
Porque el sketch ocupaba más espacio que la partición por defecto del ESP32. La solución fue compilar y subir con `PartitionScheme=huge_app`.

### `Could not open COM4, the port is busy`
Porque el Monitor Serie ya estaba abierto y mantenía bloqueado el puerto. Cerrar ese monitor permitió volver a subir el programa.

## 9. Comandos rápidos de uso diario

```powershell
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' version
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' board list
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' compile --fqbn 'esp32:esp32:esp32:PartitionScheme=huge_app' 'c:\Users\User\Documents\mochi_robot\esp32WROOMsinIA'
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' upload -p COM4 --fqbn 'esp32:esp32:esp32:PartitionScheme=huge_app' 'c:\Users\User\Documents\mochi_robot\esp32WROOMsinIA'
& 'C:\Program Files\Arduino CLI\arduino-cli.exe' monitor -p COM4 -b 'esp32:esp32:esp32:PartitionScheme=huge_app' -c baudrate=115200
```

## 10. Recomendación final

Si vas a seguir usando VS Code sin PlatformIO, lo más cómodo es dejar un `tasks.json` con tareas para:

- compilar
- subir
- abrir el monitor serie

Así no tendrás que escribir los comandos cada vez.
