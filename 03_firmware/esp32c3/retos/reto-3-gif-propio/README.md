# Reto 3 — Tu GIF en Mochi ⭐ Fácil

Personaliza el Modo 1 de Mochi con una animación que elijas tú de internet.

Al terminar, tu robot va a tener una animación que ningún otro robot del taller va a tener.

---

## El reto

Busca en internet el GIF que más te represente, que más te haga reír, o que simplemente quieras ver en la pantalla de 128×64 píxeles de tu robot. Conviértelo y súbelo.

Sitios donde buscar:
- [giphy.com](https://giphy.com)
- [tenor.com](https://tenor.com)
- Cualquier GIF que hayas guardado en tu celular o computadora

> **Restricción de tamaño:** GIFs de hasta ~30 frames funcionan bien. Uno muy largo puede no caber en la flash. Si dudas, elige algo corto y con movimiento simple.

---

## Cómo funciona la conversión

La pantalla OLED de Mochi solo muestra blanco y negro — no colores, no grises. El proceso de conversión hace esto automáticamente:

```
GIF de internet (color, cualquier tamaño)
        │
        ▼  gif.exe
carpeta/ frame_000.bin, frame_001.bin ...   ← un archivo por frame, blanco y negro
        │
        ▼  frame_to_bin_converter.exe
mi-animacion.bin                            ← un solo archivo listo para el ESP32
        │
        ▼  copiar a data/ + subir LittleFS
Modo 1 de Mochi
```

---

## Paso 1 — Descarga el GIF

Descarga el GIF a tu computadora. Anota en qué carpeta quedó.

---

## Paso 2 — Extraer los frames con `gif.exe`

1. Abre la carpeta `herramientas/convertidor_gifs/` del repositorio
2. Arrastra tu archivo `.gif` encima de **`gif.exe`**
3. Se crea una subcarpeta con el nombre del GIF conteniendo los frames:
   ```
   herramientas/convertidor_gifs/mi-animacion/
       frame_000.bin
       frame_001.bin
       ...
   ```

Cada frame es exactamente **1024 bytes** (128×64 px, 1 bit por píxel). Si el GIF tenía colores, ya quedaron convertidos a blanco y negro.

---

## Paso 3 — Crear el `.bin` final con `frame_to_bin_converter.exe`

1. Ejecuta **`frame_to_bin_converter.exe`** (en la misma carpeta `herramientas/convertidor_gifs/`)
2. Selecciona la carpeta de frames que se creó en el paso anterior
3. El programa genera `mi-animacion.bin` — este es el archivo que necesitas

---

## Paso 4 — Copiar el `.bin` a la carpeta `data/`

Usando el Explorador de archivos de Windows, copia `mi-animacion.bin` a:

```
03_firmware/esp32c3/software/data/
```

---

## Paso 5 — Subir el filesystem al ESP32

1. Abre el sketch `software.ino` en Arduino IDE
2. Verifica que el board y el puerto estén configurados en **Tools**
3. Ve a **Sketch → Upload LittleFS to ESP32**

Espera a que termine — verás `Done uploading` en la consola.

---

## Paso 6 — Ver el resultado

Entra al **Modo 1** de Mochi (avanza con hold hasta el modo GIFs). Tu animación aparece en rotación junto con las demás.

**Si no aparece:**

| Síntoma | Causa | Solución |
|---------|-------|---------|
| No aparece en el Modo 1 | El `.bin` no está en `data/` | Verifica que el archivo quedó en `03_firmware/esp32c3/software/data/` |
| No aparece aunque el archivo esté | Subiste el filesystem antes de copiar el `.bin` | Copia el archivo y vuelve a subir LittleFS |
| Imagen con ruido o estática | El GIF tenía muchos colores similares al gris medio | Elige otro con mayor contraste o fondo negro |
| Error al subir LittleFS | `data/` está demasiado llena | Borra algún `.bin` que no uses de la carpeta |

---

**← Anterior:** [Reto 2 — Nuevo mood: guiño](../reto-2-mood-wink/)
**Siguiente →** [Reto 4 — Contador de aplausos](../reto-4-aplausos/)
