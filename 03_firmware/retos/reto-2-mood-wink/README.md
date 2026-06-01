# Reto 2 — Nuevo estado de ánimo: Guiño ⭐⭐ Medio

Agrega un undécimo estado de ánimo donde el robot guiña un ojo.

## Archivo a modificar

`03_firmware/mochi_unified_5/mochi_unified_5.ino`

Busca cada sección con `Ctrl+F` usando los términos indicados.

---

## Cambio 1 — Definir la constante del nuevo mood

**Busca:** `#define MOOD_DIZZY`

**Agrega debajo:**
```cpp
#define MOOD_WINK   10   // ← nuevo
#define MOOD_COUNT  11   // ← cambia de 10 a 11
```

---

## Cambio 2 — Definir el aspecto visual del guiño

**Busca:** `drawEyelidMask` y dentro de esa función busca `MOOD_SUSPICIOUS`.

**Agrega un nuevo bloque `else if` justo después del bloque `MOOD_SUSPICIOUS`:**
```cpp
} else if (mood == MOOD_WINK) {
  // Ojo izquierdo cerrado, ojo derecho normal
  if (isLeft) {
    // Cubre completamente el ojo izquierdo con un rectángulo negro
    display.fillRect(ix, iy, iw, ih + 2, SSD1306_BLACK);
  }
}
```

---

## Cambio 3 — Definir el tamaño de ojos para WINK

**Busca:** `switch(currentMood)` dentro de la función `updateEyes()`.

**Agrega un nuevo `case` al final del switch, antes del `}`:**
```cpp
case MOOD_WINK:
  leftEye.targetW  = 36; leftEye.targetH  = 3;   // ojo izquierdo casi cerrado
  rightEye.targetW = 36; rightEye.targetH = 36;  // ojo derecho abierto normal
  break;
```

---

## Probar el resultado

Agrega temporalmente esta línea al final de `setup()`:
```cpp
currentMood = MOOD_WINK;
```

Compila y sube. La pantalla debe mostrar un ojo cerrado y uno abierto.

Cuando funcione, **elimina esa línea** de `setup()` para que el mood aparezca en la rotación aleatoria al tocar el sensor.

---

## Resultado esperado

Al dar taps en el Modo Mascota, el robot alterna entre sus 11 moods incluyendo el guiño.
