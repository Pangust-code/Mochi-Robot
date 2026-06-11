# Reto 8 — Alarma para el reloj

## Objetivo

Agrega una alarma simple al modo reloj de Mochi. Cuando la hora coincida con la que configures, el robot debe sonar, mostrar una alerta en pantalla y dejar claro que la alarma se activó.

## Nivel

Medio.

## Qué vas a tocar

- `03_firmware/mochi_unified_5/mochi_unified_5.ino`
- La función `runClockMode()`
- Variables de configuración global para guardar hora de alarma, minutos y estado de activación

## Antes de empezar

1. Verifica que el reloj ya sincroniza hora con WiFi y NTP.
2. Asegúrate de que el modo reloj funcione antes de agregar la alarma.
3. Ten una melodía corta lista para usar como aviso.

## Paso a paso

### Paso 1 — Definir la hora de alarma

Agrega variables globales para guardar:

- Hora de la alarma
- Minuto de la alarma
- Si la alarma está activa o desactivada
- Si la alarma ya sonó hoy

Puedes empezar con una sola alarma fija, por ejemplo 07:30.

### Paso 2 — Decidir cómo se activa y desactiva

Elige una interacción simple con el sensor táctil:

- 1 tap dentro del modo reloj -> activar o desactivar alarma
- 3 taps -> cambiar la hora de la alarma
- Hold largo -> volver al siguiente modo, como ya funciona el firmware

No necesitas hacer un menú complejo; con un control básico basta para el taller.

### Paso 3 — Agregar la comprobación dentro de `runClockMode()`

Después de obtener la hora actual, compara la hora y minuto reales con la alarma configurada.

Si coinciden y la alarma está activa:

- Reproduce un sonido con el buzzer
- Muestra un mensaje grande en pantalla
- Marca la alarma como disparada para no repetirla cada segundo

### Paso 4 — Evitar que se repita sin control

Agrega una condición para que la alarma no suene en bucle durante todo el minuto.

Una solución simple es guardar el día o un flag `alarmTriggeredToday`.

Cuando cambie el día, permite que la alarma vuelva a dispararse.

### Paso 5 — Diseñar la pantalla de alarma

Muestra algo visualmente claro, por ejemplo:

- `ALARMA`
- La hora configurada
- Un ícono de campana o exclamación
- Un mensaje breve como `tap = parar`

La pantalla debe quedar legible desde lejos.

### Paso 6 — Definir cómo se apaga

Cuando la alarma esté sonando, permite una acción simple para detenerla:

- Un tap corto la apaga
- O un hold largo la silencia y deja la alarma desactivada

Escoge una sola opción para que el reto siga siendo claro.

### Paso 7 — Probar el caso completo

Prueba el flujo completo:

1. Conecta WiFi.
2. Espera a que el reloj sincronice.
3. Ajusta la alarma a una hora cercana.
4. Espera el disparo.
5. Confirma que suena y se ve en pantalla.
6. Confirma que se puede apagar con el gesto elegido.

### Paso 8 — Hacerlo personal

Cuando ya funcione, puedes darle personalidad:

- Reproducir una melodía distinta según la hora
- Mostrar un emoji o icono distinto en la mañana y la noche
- Hacer que la alarma solo suene ciertos días

## Resultado esperado

Mochi no solo muestra la hora: también puede avisarte con una alarma visual y sonora cuando llega el momento configurado.
