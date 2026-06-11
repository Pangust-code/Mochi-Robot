# Reto 7 — Espejo emocional del micrófono

## Objetivo

Haz que Mochi cambie de mood en tiempo real según el volumen que detecta el micrófono. La idea es que el robot se vea calmado en silencio, curioso con ruido leve, emocionado con voz normal y sorprendido o molesto con un sonido fuerte.

## Nivel

Medio.

## Qué vas a tocar

- `03_firmware/esp32c3/software/software.ino`
- La lectura de amplitud con `leerAmplitud()`
- La lógica del modo mascota en `runMascotaMode()`
- Opcionalmente, el diseño visual en `updatePhysicsAndMood()`

## Antes de empezar

1. Sube y verifica el firmware principal.
2. Confirma que el micrófono funciona con el modo de prueba de volumen.
3. Asegúrate de tener el monitor serie abierto para ver valores reales de amplitud.

## Paso a paso

### Paso 1 — Medir el comportamiento del micrófono

Entra al modo micrófono del firmware y observa qué valores aparecen cuando hay silencio, voz normal, palmadas y ruido fuerte.

Anota tres rangos aproximados:

- Silencio
- Ruido medio
- Ruido fuerte

Esos rangos serán la base para decidir qué mood mostrar.

### Paso 2 — Definir una tabla de reacción

Decide una regla simple como esta:

- Silencio prolongado -> `MOOD_SLEEPY`
- Ruido bajo -> `MOOD_NORMAL`
- Voz o ruido medio -> `MOOD_HAPPY` o `MOOD_EXCITED`
- Ruido alto -> `MOOD_SURPRISED`
- Ruido muy alto -> `MOOD_ANGRY`

Lo importante es que la respuesta sea consistente y fácil de entender.

### Paso 3 — Guardar el mood anterior

Agrega una variable para recordar el mood anterior antes de cambiar por sonido. Eso te permite volver al estado previo cuando el ambiente se calme.

Ejemplo de comportamiento esperado:

- Si detecta un pico fuerte, cambia temporalmente a sorprendido.
- Cuando baja el sonido, vuelve al mood que tenía antes.

### Paso 4 — Integrar la lógica en el modo mascota

Haz que `runMascotaMode()` lea la amplitud en cada ciclo y compare contra los umbrales que definiste.

La idea es que el micrófono no solo active un susto ocasional, sino que influya de forma continua en la personalidad del robot.

### Paso 5 — Probar transiciones suaves

Prueba estas escenas una por una:

1. Deja silencio 5 a 10 segundos.
2. Habla cerca del micrófono.
3. Aplaude una vez.
4. Haz ruido sostenido.
5. Vuelve al silencio.

Verifica que el mood no cambie demasiado rápido ni quede “pegado” en un estado.

### Paso 6 — Ajustar los umbrales

Si el robot cambia de mood con demasiado poco ruido, sube los valores.
Si no reacciona, bájalos.

Busca un punto intermedio donde el cambio se sienta natural.

### Paso 7 — Añadir detalle visual opcional

Puedes reforzar la reacción con un pequeño ícono en pantalla:

- Zzz para silencio
- Corazón o brillo para voz agradable
- Signo de exclamación para sorpresa
- Gota o ceja para molestia

### Paso 8 — Prueba final

El reto queda bien resuelto si, al hablar o hacer ruido frente a Mochi, sus ojos cambian sin tocar el sensor y el comportamiento parece una respuesta emocional del robot.

## Resultado esperado

Mochi actúa como un espejo emocional: su expresión cambia según el ambiente sonoro sin necesidad de cambiar manualmente de modo.
