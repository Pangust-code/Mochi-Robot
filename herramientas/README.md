# Herramientas — Pipeline de assets para Mochi

Esta carpeta contiene las herramientas necesarias para convertir animaciones GIF al formato que entiende el ESP32-C6 y subirlas a la memoria interna del robot.

---

## Fase 4 — Darle personalidad a Mochi

Mochi puede mostrar cualquier animación que quepa en su pantalla OLED de 128×64 píxeles. El proceso para agregar una animación nueva tiene cuatro pasos:

```
GIF original (.gif)
       │
       ▼  1. gif.exe — extrae frames individuales
carpeta_del_gif/
  ├── frame_000.bin
  ├── frame_001.bin
  └── ...
       │
       ▼  2. frame_to_bin_converter.exe — concatena en un solo archivo
herramientas/convertidor_gifs/data/
  └── mi_animacion.bin
       │
       ▼  3. Copiar al firmware
03_firmware/mochi_unified_5/data/
  └── mi_animacion.bin
       │
       ▼  4. LittleFS Data Upload — subir al ESP32
Robot mostrando tu animación en el Modo 1
```

---

## Herramientas disponibles

| Carpeta / Archivo | Descripción |
|-------------------|-------------|
| [convertidor_gifs/](convertidor_gifs/) | Carpeta principal del pipeline de conversión |
| `convertidor_gifs/gif.exe` | Extrae los frames de un GIF como archivos `.bin` individuales |
| `convertidor_gifs/frame_to_bin_converter.exe` | Concatena los frames en un único `.bin` listo para LittleFS |
| `convertidor_gifs/data/` | Carpeta de salida con los `.bin` generados (no se sube al repositorio) |

> **Nota:** `frame_to_bin_converter.exe` reemplaza al script Python anterior.
> Si Windows bloquea la ejecución del `.exe`, haz clic derecho → Propiedades → "Desbloquear" → Aceptar.

---

## Paso 1 — Elegir y preparar el GIF

Antes de convertir, el GIF debe cumplir estas condiciones para verse bien en la OLED:

| Criterio | Recomendación |
|----------|---------------|
| Resolución | 128×64 px (o menor, se escala) |
| Colores | Blanco y negro, o muy alto contraste |
| Duración | 15–30 frames (archivos más cortos) |
| Estilo | Expresivo, formas grandes, sin texto pequeño |

Los GIFs disponibles para usar están en [`recursos/gifs/`](../recursos/gifs/).

Copia el GIF que quieres convertir a la carpeta `convertidor_gifs/` antes de continuar.

---

## Paso 2 — Extraer frames con `gif.exe`

Coloca el archivo `.gif` dentro de `convertidor_gifs/` y ejecuta:

```powershell
# Desde la carpeta herramientas/convertidor_gifs/
.\gif.exe
```

Se genera una subcarpeta con el nombre del GIF que contiene los frames:

```
convertidor_gifs/
└── nombre_del_gif/
    ├── frame_000.bin   ← 1024 bytes exactos (128×64 px, 1 bit/px)
    ├── frame_001.bin
    └── ...
```

---

## Paso 3 — Convertir a `.bin` con `frame_to_bin_converter.exe`

```powershell
# Desde la carpeta herramientas/convertidor_gifs/
.\frame_to_bin_converter.exe
```

El ejecutable detecta todas las subcarpetas de frames y genera un `.bin` por cada una en la carpeta `data/`:

```
convertidor_gifs/data/
└── nombre_del_gif.bin   ← todos los frames concatenados, listo para LittleFS
```

**Salida esperada:**
```
Convirtiendo nombre_del_gif... 24 frames → nombre_del_gif.bin (24576 bytes)
Listo.
```

---

## Paso 4 — Copiar al firmware

Copia el `.bin` generado a la carpeta de datos del firmware:

```
convertidor_gifs/data/nombre_del_gif.bin
          │
          ▼  Copiar (Ctrl+C / Ctrl+V)
03_firmware/mochi_unified_5/data/nombre_del_gif.bin
```

Anota el nombre exacto del archivo — lo necesitas en la Fase 5 para configurar el firmware.

---

## Paso 5 — Subir a LittleFS

Los archivos `.bin` no son parte del firmware compilado: deben subirse por separado al sistema de archivos del ESP32 usando la extensión **arduino-littlefs-upload** para VS Code.

### Instalación del plugin (una sola vez)

> Guía en video: [youtube.com/watch?v=vICDKOLizrU](https://www.youtube.com/watch?v=vICDKOLizrU)

1. Descarga el archivo `.vsix` desde:
   **[arduino-littlefs-upload-1.5.4.vsix](https://github.com/earlephilhower/arduino-littlefs-upload/releases/download/1.5.4/arduino-littlefs-upload-1.5.4.vsix)**

2. En VS Code, abre el panel de **Extensiones** (`Ctrl+Shift+X`).

3. Haz clic en el ícono `···` (tres puntos) → **Instalar desde VSIX...**

4. Selecciona el archivo `.vsix` descargado y confirma la instalación.

5. Reinicia VS Code cuando te lo pida.

### Uso del plugin

Una vez instalado, cada vez que quieras subir los archivos de `data/`:

1. Asegúrate de que el Monitor Serie esté **cerrado** (el plugin no puede subir si el puerto está ocupado).
2. Abre la paleta de comandos: `Ctrl+Shift+P`
3. Escribe y selecciona: **Upload LittleFS to Pico/ESP8266/ESP32**
4. El plugin sube automáticamente todo el contenido de `mochi_unified_5/data/` al ESP32.

**Salida esperada en la terminal de VS Code:**
```
[LittleFS] Uploading filesystem image...
[LittleFS] Done uploading.
```

### Opción alternativa — arduino-cli + esptool

Si prefieres la línea de comandos:
```powershell
mklittlefs -c 03_firmware/mochi_unified_5/data -p 256 -b 4096 -s 0x150000 littlefs.bin
esptool.py --chip esp32c6 --port COM3 write_flash 0x290000 littlefs.bin
```

---

## Verificación

Después de subir el filesystem, abre el Monitor Serie y reinicia el robot:

```
LittleFS OK — XX/128 KB
```

Navega al Modo 1 (mantén el sensor táctil hasta escuchar el segundo beep). Si tu `.bin` está en la lista, aparecerá en la rotación de animaciones.

---

## Solución de problemas

| Síntoma | Causa | Solución |
|---------|-------|---------|
| `gif.exe` no ejecuta | Bloqueado por Windows | Clic derecho → Propiedades → Desbloquear |
| La animación se ve borrosa | GIF con colores/gradientes | Busca un GIF con mayor contraste |
| El Modo 1 muestra error | `.bin` no subido a LittleFS | Repite el Paso 5 |
| El `.bin` generado es muy grande | GIF con demasiados frames | Reduce el GIF a máximo 30 frames online |
| Nombre con espacios causa error | Nombre del archivo con espacios | Renombra sin espacios: `mi_gif.bin` |

---

**Guía detallada del convertidor →** [convertidor_gifs/README.md](convertidor_gifs/README.md)

**GIFs disponibles para convertir →** [../recursos/gifs/](../recursos/gifs/)

**Siguiente paso →** [03_firmware/mochi_unified_5/README.md](../03_firmware/mochi_unified_5/README.md) — Fase 5: configurar e integrar el firmware
