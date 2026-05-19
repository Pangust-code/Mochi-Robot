# Convertidor de GIFs para Mochi Robot

Convierte animaciones GIF al formato `.bin` que usa LittleFS en el ESP32-C6.

---

## Archivos

| Archivo | Descripción |
|---------|-------------|
| `gif.exe` | Extrae los frames del GIF como archivos `.bin` individuales |
| `convertir_gifs.py` | Concatena los frames en un único `.bin` listo para LittleFS |
| `data/` | Carpeta de salida con los `.bin` generados *(gitignore — no se sube al repo)* |

---

## Flujo de conversión

```
GIF original
    │
    ▼  gif.exe
carpeta/frame_000.bin
carpeta/frame_001.bin
        ...
    │
    ▼  convertir_gifs.py
data/nombre.bin          ← listo para LittleFS
    │
    ▼  copiar a firmware
03_firmware/mochi_unified_5/data/nombre.bin
    │
    ▼  plugin LittleFS
ESP32-C6
```

---

## Paso 1 — Extraer frames con `gif.exe`

Coloca los GIFs que quieras convertir en esta misma carpeta y ejecuta:

```
gif.exe
```

Se generan subcarpetas `nombre/frame_000.bin`, `nombre/frame_001.bin`, etc.
Cada frame es exactamente **1024 bytes** (128×64 píxeles, 1 bit por píxel).

> Los GIFs de origen están en [recursos/gifs/](../../recursos/gifs/).
> Copia los que quieras usar a esta carpeta antes de ejecutar `gif.exe`.

---

## Paso 2 — Convertir frames a `.bin` con `convertir_gifs.py`

```bash
# Convierte todas las carpetas de frames encontradas
python convertir_gifs.py

# O convierte solo animaciones específicas
python convertir_gifs.py angry bee star
```

Los `.bin` se guardan en `data/`.

**Salida esperada:**
```
Convirtiendo 3 animación(es)...

  [OK]  angry                →  angry.bin             21 frames  (21,504 bytes)
  [OK]  bee                  →  bee.bin               66 frames  (67,584 bytes)
  [OK]  star                 →  star.bin              89 frames  (91,136 bytes)

Listo. Sube la carpeta 'data/' al ESP32-C6 con el plugin LittleFS.
```

---

## Paso 3 — Copiar al firmware

```
# Copiar los .bin generados a la carpeta del firmware
data/nombre.bin  →  03_firmware/mochi_unified_5/data/nombre.bin
```

---

## Paso 4 — Subir a LittleFS

Desde VS Code, usa las tareas preconfiguradas en [.vscode/tasks.json](../../.vscode/tasks.json)
o con Arduino CLI:

```bash
# Navega a la carpeta del firmware
cd 03_firmware/mochi_unified_5

# Sube el filesystem (requiere el plugin LittleFS para Arduino CLI)
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 --input-dir data/
```

---

## Formato del `.bin`

- Resolución: **128×64 píxeles**, monocromático (1 bit por píxel)
- Tamaño por frame: **1024 bytes** exactos
- Múltiples frames concatenados sin separadores ni cabecera
- El firmware lee `fileSize / 1024` para saber cuántos frames reproducir

---

## Notas sobre los GIFs

- La OLED es monocromática: los GIFs con alto contraste se ven mejor
- GIFs muy largos generan archivos `.bin` grandes (puede limitarse el espacio en LittleFS)
- Si el GIF tiene colores, `gif.exe` los convierte a blanco/negro por umbral de luminosidad
