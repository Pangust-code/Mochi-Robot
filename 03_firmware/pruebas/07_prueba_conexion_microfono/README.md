# 🎙️ Verificación de Continuidad I2S: ESP32-C6 + INMP441

[cite_start]Este repositorio contiene una herramienta de diagnóstico físico a bajo nivel diseñada para certificar la integridad del cableado eléctrico del bus I2S[cite: 275]. [cite_start]Su propósito es prevenir fallos silenciosos o cortocircuitos validando la continuidad de las pistas antes de conectar físicamente el micrófono INMP441 y encender el mini-robot reactivo definitivo[cite: 275].

A diferencia de las pruebas de software, este script es una prueba de **continuidad de hardware (Dry-Run)**.

---

## 🛠️ Arquitectura de Hardware y Pinout

[cite_start]Esta prueba está configurada para evaluar las líneas de datos del protocolo I2S de audio digital[cite: 276, 280].

| Señal INMP441 | Pin ESP32-C6 | Protocolo / Función Lógica |
| :--- | :--- | :--- |
| **SD** | **GPIO 3** | [cite_start]Serial Data (Línea de datos de audio)[cite: 276, 280]. |
| **SCK** | **GPIO 4** | [cite_start]Serial Clock (Reloj de bits o BCK)[cite: 276, 280]. |
| **WS** | **GPIO 5** | [cite_start]Word Select (Selección de canal izquierdo/derecho o LRCK)[cite: 276, 280]. |

> [cite_start]⚠️ **IMPORTANTE:** Para ejecutar esta prueba, el micrófono INMP441 **NO DEBE** estar conectado a la placa[cite: 276].

---

## 🧠 Desglose Lógico: Resistencias Pull-Up

[cite_start]El script se basa en la topología eléctrica de los microcontroladores modernos para aislar problemas de soldadura o cables defectuosos[cite: 276, 277].

1. [cite_start]**Estado de Reposo (HIGH):** El código configura los tres pines de audio mediante la instrucción nativa `INPUT_PULLUP`[cite: 277]. Esto conecta internamente los pines a 3.3V a través de una resistencia, manteniendo su estado lógico en `HIGH` ("no conectado a GND") por defecto[cite: 277, 282].
2. [cite_start]**Caída de Tensión Forzada (LOW):** Al puentear manualmente el pin físico hacia un pin de Tierra (`GND`) utilizando un cable Dupont, se vence la resistencia de pull-up, forzando un estado `LOW`[cite: 278, 288].
3. [cite_start]**Validación:** Si al tocar el pin con Tierra el estado no cambia a `LOW` en el sistema, existe una ruptura física, una soldadura fría o un cable Dupont dañado que impide que la señal eléctrica llegue al núcleo del ESP32-C6[cite: 277, 279].

---

## 🚀 Guía de Pruebas (Paso a Paso)

1. [cite_start]**Desconexión:** Asegúrate de que el micrófono INMP441 esté completamente desconectado de los cables[cite: 276].
2. **Carga:** Sube el código `prueba_conexion_microfono.ino` al ESP32-C6.
3. [cite_start]**Monitor:** Abre el Monitor Serie de tu IDE configurado estrictamente a **115200 baudios**[cite: 276].
4. **Test de Corto a Tierra:** Toma un cable Dupont macho suelto. Conecta un extremo a cualquier pin `GND` del ESP32-C6.
5. [cite_start]**Comprobación:** Con la otra punta del cable, toca momentáneamente las terminales metálicas correspondientes a los GPIO 3, 4 y 5[cite: 276].
6. **Éxito:** Por cada toque, la consola debe registrar un cambio inmediato. [cite_start]Cuando el sistema detecte que los tres canales lograron bajar a `LOW` exitosamente, arrojará un mensaje de confirmación habilitando el ensamblaje final[cite: 285, 286].

---

## 💻 Telemetría Esperada en el Monitor Serie

Al interactuar físicamente con los pines, el sistema reaccionará a los cambios de estado imprimiendo este registro estructurado:

```text
========================================
  PRUEBA DE CONEXIONES — INMP441
========================================
  Toca cada pin a GND con un dupont.
  Debe aparecer: LOW ✓
========================================

----------------------------------------
  SD  (datos)  GPIO 3 : HIGH  — no conectado a GND
  SCK (reloj)  GPIO 4 : HIGH  — no conectado a GND
  WS  (frame)  GPIO 5 : HIGH  — no conectado a GND
----------------------------------------

----------------------------------------
  SD  (datos)  GPIO 3 : LOW  ✓ cable responde OK
  SCK (reloj)  GPIO 4 : HIGH  — no conectado a GND
  WS  (frame)  GPIO 5 : HIGH  — no conectado a GND
----------------------------------------

----------------------------------------
  SD  (datos)  GPIO 3 : LOW  ✓ cable responde OK
  SCK (reloj)  GPIO 4 : LOW  ✓ cable responde OK
  WS  (frame)  GPIO 5 : LOW  ✓ cable responde OK

  >>> TODOS LOS PINES RESPONDIERON ✓
      Conexiones físicas verificadas.
      Ya puedes conectar el micrófono.
----------------------------------------