# Reto 1 — Tu propia melodía ⭐ Fácil

Reemplaza el tema de Tetris por una melodía de tu elección.

## Archivos

| Archivo | Contenido |
|---------|-----------|
| `reto-1-melodia.ino` | Sketch standalone para probar tu melodía |

## Cómo usar

1. **Abre `reto-1-melodia.ino`** en VS Code.
2. **Modifica el array `mel[]`** con tus notas. Cada par es `{nota, duración_ms}`.
3. **Compila y sube** al ESP32:
   ```powershell
   arduino-cli compile --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/retos/reto-1-melodia
   arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c3 03_firmware/esp32c3/retos/reto-1-melodia
   ```
4. **Toca el sensor** → escucha tu melodía.
5. Cuando suene como quieres, **aplica los mismos cambios** a `tetrisTheme()` en `03_firmware/esp32c3/software/software.ino`.

## Tabla de notas

| Nota | Constante | Hz |
|------|----------|----|
| Do | C4 | 262 |
| Re | D4 | 294 |
| Mi | E4 | 330 |
| Fa | F4 | 349 |
| Sol | G4 | 392 |
| La | A4 | 440 |
| Si | B4 | 494 |
| Do alto | C5 | 523 |
| Re alto | D5 | 587 |
| Mi alto | E5 | 659 |
| Fa alto | F5 | 698 |
| Sol alto | G5 | 784 |
| La alto | A5 | 880 |

> `SIL` (= 0) inserta un silencio entre notas.

## Resultado esperado

Al dar 3 taps rápidos en el Modo Mascota del firmware principal, el robot toca tu melodía en lugar del tema de Tetris.
