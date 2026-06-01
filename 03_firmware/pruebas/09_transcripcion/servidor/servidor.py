"""
servidor.py — Servidor local de transcripción para Mochi Robot

Recibe un archivo WAV enviado por el ESP32 vía HTTP POST,
lo transcribe con Whisper y devuelve el resultado como JSON.

Uso:
    python servidor.py

Endpoints:
    POST /transcribir   → body: bytes WAV   → {"text": "..."}
    GET  /estado        → {"estado": "ok", "modelo": "base"}

Requisitos:
    pip install -r requirements.txt
"""

import os
import tempfile
import logging
from flask import Flask, request, jsonify
import whisper

# ── Configuración ──────────────────────────────────────────────────────────
PUERTO  = 5000
MODELO  = "base"      # opciones: tiny | base | small | medium | large
                      # "tiny"  = más rápido, menos preciso (~75 MB)
                      # "base"  = balance ideal para pruebas (~150 MB)
                      # "small" = mejor precisión (~500 MB)
IDIOMA  = "es"        # "es" = español, None = detección automática

# ── Logging ────────────────────────────────────────────────────────────────
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s  %(levelname)s  %(message)s",
    datefmt="%H:%M:%S"
)
log = logging.getLogger(__name__)

# ── Cargar modelo (al iniciar, no en cada petición) ────────────────────────
log.info(f"Cargando modelo Whisper '{MODELO}'...")
modelo_whisper = whisper.load_model(MODELO)
log.info("Modelo listo.")

app = Flask(__name__)

# ── Endpoints ──────────────────────────────────────────────────────────────

@app.route("/transcribir", methods=["POST"])
def transcribir():
    """
    Recibe un archivo WAV como cuerpo de la petición (Content-Type: audio/wav)
    y devuelve la transcripción en JSON:
        {"text": "hola mundo", "idioma": "es", "duracion_s": 2.94}
    """
    wav_bytes = request.data
    if not wav_bytes:
        log.warning("Petición vacía recibida")
        return jsonify({"error": "cuerpo vacío"}), 400

    log.info(f"Recibidos {len(wav_bytes):,} bytes de audio")

    # Guardar en archivo temporal para que Whisper pueda leerlo
    tmp_path = None
    try:
        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
            f.write(wav_bytes)
            tmp_path = f.name

        log.info(f"Transcribiendo con modelo '{MODELO}'...")
        resultado = modelo_whisper.transcribe(
            tmp_path,
            language=IDIOMA,
            fp16=False       # fp16=False evita advertencias en CPU
        )

        texto    = resultado["text"].strip()
        idioma   = resultado.get("language", IDIOMA or "?")
        duracion = resultado.get("segments", [{}])[-1].get("end", 0)

        log.info(f"Transcripción: '{texto}' (idioma={idioma}, {duracion:.1f}s)")

        return jsonify({
            "text":      texto,
            "idioma":    idioma,
            "duracion_s": round(duracion, 2)
        })

    except Exception as e:
        log.error(f"Error al transcribir: {e}", exc_info=True)
        return jsonify({"error": str(e)}), 500

    finally:
        if tmp_path and os.path.exists(tmp_path):
            os.unlink(tmp_path)


@app.route("/estado", methods=["GET"])
def estado():
    """Health-check — útil para verificar que el servidor está activo."""
    return jsonify({"estado": "ok", "modelo": MODELO, "idioma": IDIOMA})


# ── Entrada ────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    import socket
    ip_local = socket.gethostbyname(socket.gethostname())
    log.info(f"Servidor escuchando en http://0.0.0.0:{PUERTO}")
    log.info(f"IP local de este equipo: {ip_local}")
    log.info(f"Configura el ESP32 con:  SERVER_URL = \"http://{ip_local}:{PUERTO}/transcribir\"")
    app.run(host="0.0.0.0", port=PUERTO, debug=False)
