#!/usr/bin/env python3
"""
convertir_gifs.py — Convierte carpetas de frames directamente a .bin para LittleFS

PIPELINE ANTERIOR (redundante):
    carpetas/frame_000.bin
        → covertidor.py  → nombre.h  (bytes → texto hex ASCII)
        → convertir_gifs.py          (texto hex → bytes de nuevo)
        → data/nombre.bin

PIPELINE ACTUAL (directo):
    carpetas/frame_000.bin  →  data/nombre.bin

La conversión a .h era un paso intermedio innecesario: convertía los bytes a
texto hexadecimal para luego parsearlo de vuelta a bytes. Este script elimina
ese paso y concatena los frames directamente en binario.

USO:
    python convertir_gifs.py              # convierte todas las carpetas con frames
    python convertir_gifs.py angry bee    # convierte solo las animaciones indicadas

REQUISITOS:
    - Las carpetas de frames deben estar en el mismo directorio que este script
    - Cada carpeta contiene archivos frame_000.bin, frame_001.bin, etc.
    - Cada frame es exactamente 1024 bytes (128×64 px, 1 bit por píxel)

SALIDA:
    data/nombre.bin  → listo para subir al ESP32-C6 con el plugin LittleFS
"""

import os
import sys

FRAME_SIZE   = 1024   # 128×64 / 8  — debe coincidir con el firmware
CARPETA_SALIDA = "data"
EXCLUIR      = {CARPETA_SALIDA, "__pycache__", ".git"}


def carpeta_a_bin(ruta_carpeta: str, ruta_salida: str) -> None:
    """Concatena todos los frame_XXX.bin de una carpeta en un único .bin."""

    frames = sorted(
        nombre for nombre in os.listdir(ruta_carpeta)
        if nombre.lower().endswith(".bin")
        and os.path.isfile(os.path.join(ruta_carpeta, nombre))
    )

    if not frames:
        print(f"  [SKIP] {os.path.basename(ruta_carpeta)}: sin archivos .bin")
        return

    datos = bytearray()
    for nombre in frames:
        with open(os.path.join(ruta_carpeta, nombre), "rb") as f:
            datos.extend(f.read())

    n_frames  = len(datos) // FRAME_SIZE
    sobrante  = len(datos) % FRAME_SIZE

    if n_frames == 0:
        print(f"  [ERROR] {os.path.basename(ruta_carpeta)}: datos insuficientes para un frame completo")
        return

    os.makedirs(os.path.dirname(ruta_salida), exist_ok=True)
    with open(ruta_salida, "wb") as f:
        f.write(datos[: n_frames * FRAME_SIZE])

    nombre_base = os.path.basename(ruta_salida)
    nota = f"  ({sobrante} bytes sobrantes ignorados)" if sobrante else ""
    print(f"  [OK]  {os.path.basename(ruta_carpeta):20s}  →  {nombre_base:20s}  "
          f"{n_frames} frames  ({n_frames * FRAME_SIZE:,} bytes){nota}")


def main() -> None:
    base = os.path.dirname(os.path.abspath(__file__))

    if len(sys.argv) > 1:
        nombres = sys.argv[1:]
    else:
        # Auto-detectar: subdirectorios que contengan al menos un .bin
        nombres = [
            d for d in sorted(os.listdir(base))
            if os.path.isdir(os.path.join(base, d))
            and d not in EXCLUIR
            and any(
                f.lower().endswith(".bin")
                for f in os.listdir(os.path.join(base, d))
                if os.path.isfile(os.path.join(base, d, f))
            )
        ]

    if not nombres:
        print("No se encontraron carpetas con frames .bin.")
        print("Ejecuta gif.exe primero para generar los frames.")
        return

    print(f"Convirtiendo {len(nombres)} animación(es)...\n")
    for nombre in nombres:
        ruta_carpeta = os.path.join(base, nombre)
        if not os.path.isdir(ruta_carpeta):
            print(f"  [SKIP] '{nombre}': carpeta no encontrada")
            continue
        ruta_salida = os.path.join(base, CARPETA_SALIDA, nombre + ".bin")
        carpeta_a_bin(ruta_carpeta, ruta_salida)

    print(f"\nListo. Sube la carpeta '{CARPETA_SALIDA}/' al ESP32-C6 con el plugin LittleFS.")


if __name__ == "__main__":
    main()
