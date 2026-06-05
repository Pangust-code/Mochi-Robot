#!/usr/bin/env python3
"""
convertir_gifs_gui.py — GUI para convertir carpetas de frames a .bin para LittleFS

Complemento de gif.exe: después de extraer los frames con gif.exe, usa esta
herramienta para concatenarlos en archivos .bin listos para subir al ESP32-C6
con el plugin LittleFS.

PIPELINE:
    gif.exe  →  carpetas/frame_000.bin ...
    convertir_gifs_gui.exe  →  data/nombre.bin  (listo para LittleFS)

COMPILAR:
    pyinstaller --onefile --windowed --name convertir_gifs convertir_gifs_gui.py
    (o ejecutar build.bat)
"""

import os
import sys
import threading
import queue
import tkinter as tk
from tkinter import ttk, messagebox

FRAME_SIZE     = 1024        # 128×64 / 8 — debe coincidir con el firmware
CARPETA_SALIDA = "data"
EXCLUIR        = {CARPETA_SALIDA, "__pycache__", ".git"}


# ── Helpers de negocio ────────────────────────────────────────────────────────

def get_base_dir() -> str:
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return os.path.dirname(os.path.abspath(__file__))


def detectar_carpetas(base: str) -> list:
    resultado = []
    for d in sorted(os.listdir(base)):
        ruta = os.path.join(base, d)
        if not os.path.isdir(ruta) or d in EXCLUIR:
            continue
        if any(
            f.lower().endswith(".bin") and os.path.isfile(os.path.join(ruta, f))
            for f in os.listdir(ruta)
        ):
            resultado.append(d)
    return resultado


def carpeta_a_bin(ruta_carpeta: str, ruta_salida: str, log_fn) -> bool:
    nombre = os.path.basename(ruta_carpeta)

    frames = sorted(
        f for f in os.listdir(ruta_carpeta)
        if f.lower().endswith(".bin") and os.path.isfile(os.path.join(ruta_carpeta, f))
    )

    if not frames:
        log_fn(f"[SKIP]  {nombre}: sin archivos .bin", "warn")
        return False

    datos = bytearray()
    for f in frames:
        with open(os.path.join(ruta_carpeta, f), "rb") as fh:
            datos.extend(fh.read())

    n_frames = len(datos) // FRAME_SIZE
    sobrante = len(datos) % FRAME_SIZE

    if n_frames == 0:
        log_fn(f"[ERROR] {nombre}: datos insuficientes para un frame completo", "error")
        return False

    os.makedirs(os.path.dirname(ruta_salida), exist_ok=True)
    with open(ruta_salida, "wb") as fh:
        fh.write(datos[: n_frames * FRAME_SIZE])

    nombre_bin = os.path.basename(ruta_salida)
    nota = f"  ({sobrante} B ignorados)" if sobrante else ""
    log_fn(
        f"[OK]    {nombre:<20s}  →  {nombre_bin:<22s} "
        f"{n_frames} frames  ({n_frames * FRAME_SIZE:,} B){nota}",
        "ok",
    )
    return True


# ── Aplicación ────────────────────────────────────────────────────────────────

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Convertir Frames → .BIN  |  Mochi Robot")
        self.resizable(False, False)
        self.base = get_base_dir()
        self._log_queue: queue.Queue = queue.Queue()
        self._vars: dict = {}

        self._build_ui()
        self._refresh()
        self._poll_log()

    # ── Construcción de UI ────────────────────────────────────────────────────

    def _build_ui(self):
        PAD = 10

        # Header
        header = tk.Frame(self, bg="#1e1e2e")
        header.pack(fill="x")
        tk.Label(
            header,
            text="  Convertir frames .BIN  →  animación .BIN para LittleFS",
            bg="#1e1e2e", fg="#cdd6f4",
            font=("Segoe UI", 11, "bold"), pady=10,
        ).pack(side="left")
        tk.Label(
            header,
            text="Mochi Robot  ",
            bg="#1e1e2e", fg="#6c7086",
            font=("Segoe UI", 9),
        ).pack(side="right")

        # Cuerpo principal
        body = tk.Frame(self, padx=PAD, pady=PAD)
        body.pack(fill="both", expand=True)

        # Panel izquierdo: lista de carpetas
        left = tk.LabelFrame(body, text=" Carpetas detectadas ", padx=8, pady=8)
        left.grid(row=0, column=0, sticky="nsew", padx=(0, PAD))

        canvas = tk.Canvas(left, width=210, height=270, highlightthickness=0)
        sb = ttk.Scrollbar(left, orient="vertical", command=canvas.yview)
        canvas.configure(yscrollcommand=sb.set)
        sb.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)

        self._check_frame = tk.Frame(canvas)
        canvas.create_window((0, 0), window=self._check_frame, anchor="nw")
        self._check_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all")),
        )
        self._canvas = canvas

        # Botones de selección rápida
        btn_row = tk.Frame(left)
        btn_row.pack(fill="x", pady=(8, 0))
        tk.Button(btn_row, text="Todo",     width=7,  command=self._select_all).pack(side="left")
        tk.Button(btn_row, text="Ninguno",  width=7,  command=self._deselect_all).pack(side="left", padx=3)
        tk.Button(btn_row, text="↺ Buscar", width=8,  command=self._refresh).pack(side="right")

        # Panel derecho: log
        right = tk.LabelFrame(body, text=" Log de conversión ", padx=8, pady=8)
        right.grid(row=0, column=1, sticky="nsew")

        self._log_text = tk.Text(
            right, width=58, height=19,
            state="disabled",
            font=("Consolas", 9),
            bg="#1e1e1e", fg="#d4d4d4",
            relief="flat", wrap="none",
            selectbackground="#264f78",
        )
        log_sb = ttk.Scrollbar(right, orient="vertical", command=self._log_text.yview)
        self._log_text.configure(yscrollcommand=log_sb.set)
        log_sb.pack(side="right", fill="y")
        self._log_text.pack(fill="both", expand=True)

        self._log_text.tag_configure("ok",    foreground="#a6e3a1")
        self._log_text.tag_configure("warn",  foreground="#f9e2af")
        self._log_text.tag_configure("error", foreground="#f38ba8")
        self._log_text.tag_configure("info",  foreground="#89b4fa")
        self._log_text.tag_configure("dim",   foreground="#6c7086")

        # Barra inferior: estado + botón
        footer = tk.Frame(self, padx=PAD, pady=PAD)
        footer.pack(fill="x")

        self._status_var = tk.StringVar(value="Listo.")
        tk.Label(
            footer, textvariable=self._status_var,
            anchor="w", fg="#888", font=("Segoe UI", 8),
        ).pack(side="left", fill="x", expand=True)

        self._btn = tk.Button(
            footer,
            text="  Convertir seleccionadas  ",
            font=("Segoe UI", 10, "bold"),
            bg="#0078d4", fg="white",
            activebackground="#005fa3", activeforeground="white",
            relief="flat", padx=12, pady=7,
            command=self._run_conversion,
        )
        self._btn.pack(side="right")

        body.columnconfigure(1, weight=1)

    # ── Lógica de UI ──────────────────────────────────────────────────────────

    def _refresh(self):
        for w in self._check_frame.winfo_children():
            w.destroy()
        self._vars.clear()

        carpetas = detectar_carpetas(self.base)
        if not carpetas:
            tk.Label(
                self._check_frame,
                text="Sin carpetas con frames.\nEjecuta gif.exe primero.",
                justify="left", fg="#888", font=("Segoe UI", 9),
            ).pack(anchor="w", pady=6, padx=4)
        else:
            for nombre in carpetas:
                var = tk.BooleanVar(value=True)
                self._vars[nombre] = var
                n_frames = self._contar_frames(nombre)
                etiqueta = f"{nombre}  ({n_frames} frames)"
                tk.Checkbutton(
                    self._check_frame, text=etiqueta,
                    variable=var, anchor="w",
                    font=("Segoe UI", 9),
                ).pack(fill="x", padx=2, pady=1)

        n = len(carpetas)
        self._status_var.set(
            f"{n} carpeta(s) detectada(s)   |   salida → {CARPETA_SALIDA}/"
        )

    def _contar_frames(self, nombre: str) -> int:
        ruta = os.path.join(self.base, nombre)
        try:
            return sum(
                1 for f in os.listdir(ruta)
                if f.lower().endswith(".bin") and os.path.isfile(os.path.join(ruta, f))
            )
        except OSError:
            return 0

    def _select_all(self):
        for v in self._vars.values():
            v.set(True)

    def _deselect_all(self):
        for v in self._vars.values():
            v.set(False)

    def _run_conversion(self):
        seleccionadas = [n for n, v in self._vars.items() if v.get()]
        if not seleccionadas:
            messagebox.showwarning("Sin selección", "Selecciona al menos una carpeta.")
            return

        self._btn.config(state="disabled")
        self._clear_log()
        self._log(f"Convirtiendo {len(seleccionadas)} animación(es)...", "info")
        self._log(f"Destino: {os.path.join(self.base, CARPETA_SALIDA)}/\n", "dim")

        threading.Thread(
            target=self._worker, args=(seleccionadas,), daemon=True
        ).start()

    def _worker(self, nombres: list):
        ok = 0
        for nombre in nombres:
            ruta_carpeta = os.path.join(self.base, nombre)
            ruta_salida  = os.path.join(self.base, CARPETA_SALIDA, nombre + ".bin")
            if carpeta_a_bin(ruta_carpeta, ruta_salida, self._log):
                ok += 1

        total = len(nombres)
        self._log(
            f"\n{'─' * 55}\n"
            f"Resultado: {ok}/{total} convertidas exitosamente.\n"
            f"Sube la carpeta '{CARPETA_SALIDA}/' al ESP32-C6 con el plugin LittleFS.",
            "info",
        )
        self.after(0, lambda: self._btn.config(state="normal"))
        self.after(0, lambda: self._status_var.set(
            f"{ok}/{total} convertidas   |   {CARPETA_SALIDA}/  listo para LittleFS"
        ))

    # ── Log (thread-safe) ─────────────────────────────────────────────────────

    def _log(self, msg: str, tag: str = ""):
        self._log_queue.put((msg, tag))

    def _clear_log(self):
        self._log_text.config(state="normal")
        self._log_text.delete("1.0", "end")
        self._log_text.config(state="disabled")

    def _poll_log(self):
        try:
            while True:
                msg, tag = self._log_queue.get_nowait()
                self._log_text.config(state="normal")
                self._log_text.insert("end", msg + "\n", (tag,) if tag else ())
                self._log_text.see("end")
                self._log_text.config(state="disabled")
        except queue.Empty:
            pass
        self.after(50, self._poll_log)


if __name__ == "__main__":
    app = App()
    app.mainloop()
