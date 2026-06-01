# Reto 3 — Animación GIF propia ⭐ / ⭐⭐

Agrega una animación GIF personalizada al Modo 1 del robot.

## Nivel

- ⭐ Fácil si usas un GIF existente de `recursos/gifs/`
- ⭐⭐ Medio si creas el GIF desde cero

---

## Paso 1 — Conseguir o crear un GIF

### Opción A — Usar uno de los GIFs del repositorio
Hay 99 GIFs en `recursos/gifs/`. Elige el que más te guste.

### Opción B — Crear tu propio GIF
Herramientas recomendadas:
- **Piskel** (web, gratuito): [piskelapp.com](https://www.piskelapp.com)
- **Aseprite** (de pago, muy completo)
- **GIMP** (gratuito, más complejo)

**Requisitos del GIF:**
- Tamaño: **128 × 64 píxeles** (exacto)
- Colores: **blanco y negro** (escala de grises si no puedes hacer 1-bit)
- Frames: entre 4 y 30 (más frames = más espacio en la flash)

---

## Paso 2 — Convertir el GIF a formato `.bin`

El ESP32 no puede leer GIFs directamente. Necesitas convertirlo al formato binario que usa Mochi.

Usa la herramienta incluida en el repositorio:

```
herramientas/convertidor_gifs/gif.exe
```

**Windows:**
```powershell
# Arrastra el GIF encima de gif.exe
# O desde la terminal:
.\herramientas\convertidor_gifs\gif.exe mi-animacion.gif
```

El resultado es un archivo `mi-animacion.bin` en el mismo directorio.

---

## Paso 3 — Copiar el `.bin` a la carpeta `data/`

```powershell
copy mi-animacion.bin 03_firmware/mochi_unified_5/data/
```

---

## Paso 4 — Subir el filesystem al ESP32

Repite el proceso de subida de LittleFS (Sección 2, Paso 6) para que el ESP32 encuentre el nuevo archivo.

Con el plugin Arduino IDE:
1. Abre el sketch `mochi_unified_5.ino`
2. Ve a **Herramientas → ESP32 LittleFS Data Upload**

---

## Paso 5 — Verificar

Entra al **Modo 1** (avanza con hold hasta GIFs) y espera la rotación. Tu animación debe aparecer entre los demás GIFs.

Si no aparece:
- Verifica que el `.bin` está en `03_firmware/mochi_unified_5/data/`
- Verifica que subiste el filesystem DESPUÉS de copiar el archivo
- En el monitor serial verás `LittleFS OK — XX/128 KB` — si el número aumentó, el archivo está ahí

---

## Resultado esperado

Tu animación personalizada se reproduce en el Modo 1 en rotación con las demás.
