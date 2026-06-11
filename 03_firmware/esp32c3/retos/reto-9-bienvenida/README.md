# Reto 9 — Pantalla de inicio animada

## Objetivo

Haz que Mochi arranque con una pantalla de bienvenida animada en lugar de mostrar solo el mensaje de listo. La idea es que al encenderse tenga una pequeña identidad visual propia.

## Nivel

Fácil.

## Qué vas a tocar

- `03_firmware/esp32c3/software/software.ino`
- La función `setup()`
- Una función nueva para dibujar la animación de arranque

## Antes de empezar

1. El firmware principal debe compilar y arrancar sin errores.
2. La pantalla OLED debe estar funcionando.
3. Ten listo un texto corto para la bienvenida, por ejemplo `Hola, soy Mochi`.

## Paso a paso

### Paso 1 — Definir qué quieres mostrar al arrancar

Piensa en una mini secuencia de inicio de 2 a 5 segundos.

Puedes incluir:

- El nombre del robot
- Ojos que aparecen poco a poco
- Un icono pequeño o una carita
- Una frase corta de bienvenida

La regla es simple: debe verse bien, rápido y sin bloquear demasiado el arranque.

### Paso 2 — Crear una función de animación

Agrega una función nueva, por ejemplo `showBootAnimation()` o `drawWelcomeScreen()`.

Esa función debe encargarse de:

- Limpiar la pantalla
- Dibujar el fondo o marco
- Mostrar texto
- Cambiar algo entre frames si quieres animación

Si quieres mantenerlo simple, puedes hacer tres pantallas consecutivas con `delay()` corto.

### Paso 3 — Llamar la animación desde `setup()`

Ubica el momento en que la pantalla ya fue inicializada y antes de mostrar el estado final del sistema.

Ahí llama tu función de bienvenida.

Después de la animación, vuelve a mostrar el mensaje normal de arranque o pasa directamente al modo mascota.

### Paso 4 — Mantener el tiempo de arranque razonable

La animación debe ser corta.

Si dura demasiado, el robot se siente lento al encender.

Un buen rango es entre 2 y 4 segundos.

### Paso 5 — Hacer que se vea animada

Puedes lograr movimiento de varias formas:

- Desplazar un texto de izquierda a derecha
- Hacer aparecer los ojos en dos pasos
- Parpadear 2 o 3 veces
- Cambiar el tamaño del ícono o del texto

No necesitas gráficos complejos para que se note la animación.

### Paso 6 — Agregar un toque de sonido opcional

Si quieres, acompaña la bienvenida con un beep corto o una pequeña secuencia de notas.

Eso ayuda a que el encendido se sienta más “vivo”.

### Paso 7 — Probar el arranque varias veces

Reinicia el ESP32 varias veces y revisa:

- Que la animación siempre aparezca
- Que el texto se lea bien
- Que la pantalla no parpadee raro
- Que luego el firmware siga funcionando normal

### Paso 8 — Ajustar el estilo

Si quieres hacerlo más personal, prueba una de estas variantes:

- Modo elegante: fondo limpio y texto centrado
- Modo divertido: ojos grandes y saludo corto
- Modo técnico: nombre del proyecto y versión

## Resultado esperado

Cuando Mochi enciende, muestra una bienvenida animada y reconocible antes de entrar a su funcionamiento normal.
