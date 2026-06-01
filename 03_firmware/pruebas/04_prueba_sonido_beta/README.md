# Sketch — Prueba Sonido Beta

Prueba básica del buzzer pasivo usando la API `ledcWriteTone` del ESP32.
Al tocar el sensor TTP223 reproduce una escala ascendente de 4 notas (Do-Mi-Sol-Do).

> Si ya pasaste `prueba_buzzer`, este sketch no es necesario — es una versión simplificada pensada para verificar que `ledcAttach` / `ledcWriteTone` funcionen antes de usar `tone()`.

---

## Pines

| Componente | GPIO |
|-----------|------|
| Buzzer pasivo | 19 |
| Sensor táctil TTP223 | 2 |

---

## Compilar y subir

```bash
arduino-cli compile --fqbn esp32:esp32:esp32c6 .
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 .
```

---

## Uso

1. Sube el sketch
2. Toca el sensor → escucharás 4 tonos ascendentes (262 → 330 → 392 → 523 Hz)
3. Si no suena nada → revisa la conexión del buzzer en GPIO 19
4. Si suena un tono fijo sin cambiar → tu buzzer es **activo** (no pasivo); usa uno pasivo

---

## Diferencia con prueba_buzzer

| Sketch | API | OLED | Tests automáticos | Tetris |
|--------|-----|------|-------------------|--------|
| `prueba_sonido_beta` | `ledcWriteTone` | No | No | No |
| `prueba_buzzer` | `tone()` | Sí | Sí (sweep + escala + tonos firmware) | Sí |

Usa `prueba_buzzer` para una verificación completa.
