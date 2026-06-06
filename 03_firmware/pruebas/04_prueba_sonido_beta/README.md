# 🤖 Suite de Pruebas de Hardware y Diagnóstico: ESP32-C6 Supermini

Este repositorio contiene un conjunto de herramientas de diagnóstico y scripts de validación modular diseñados para aislar, testear y certificar los componentes periféricos esenciales (**Pantalla OLED SH1106**, **Sensor Táctil Capacitivo TTP223** y **Buzzer Pasivo**) antes de su integración en el firmware unificado principal del ecosistema robótico (`mochi_unified_5`).

El propósito fundamental de esta suite es proporcionar un entorno controlado de pruebas físicas (*hardware verification*) que permita descartar fallas de cableado, problemas de direccionamiento I2C, defectos físicos en los paneles o incompatibilidades de componentes (como el uso erróneo de buzzers activos).

---

## 🛠️ Arquitectura Global de Hardware y Pinout Consolidado

Para la ejecución correcta de cualquiera de los cuatro scripts de diagnóstico, la placa de desarrollo **ESP32-C6 Supermini** debe conectarse siguiendo este mapa estricto de pines e interconexiones lógicas:

| Componente Periférico | Pin del Componente | Pin ESP32-C6 | Función Lógica | Descripción del Canal de Control |
| :--- | :--- | :--- | :--- | :--- |
| **OLED SH1106 128×64**| VCC | 3.3V | Alimentación | Voltaje operativo de los diodos orgánicos. |
| **OLED SH1106 128×64**| GND | GND | Tierra | Referencia de potencial común estricta. |
| **OLED SH1106 128×64**| SDA | **GPIO 6** | Bus Datos I2C | Línea bidireccional de datos gráficos. |
| **OLED SH1106 128×64**| SCL | **GPIO 7** | Bus Reloj I2C | Señal de sincronización de reloj a 400 kHz. |
| **TTP223 Touch Sensor**| SIG / OUT | **GPIO 2** | Entrada Digital | Captura de flancos lógicos de interacción. |
| **Buzzer Pasivo / Spk** | SIG / PWM | **GPIO 19** | Salida PWM | Generación de frecuencias de audio variables. |

> ⚠️ **Nota de Seguridad:** Todos los periféricos operan nativamente a un nivel lógico de **3.3V**. No alimente el sensor táctil ni la pantalla con la salida de 5V para evitar daños permanentes en las compuertas lógicas del ESP32-C6.
> 📌 **Dirección de Red I2C:** La pantalla OLED utiliza la dirección por hardware fija `0x3C`.

---

## 📂 Guía Técnica de Módulos Incorporados

A continuación, se detalla la lógica de ingeniería y el comportamiento operativo de cada archivo incluido en la suite:

### 1. 📺 Diagnóstico Gráfico Avanzado (`prueba_pantalla.ino`)
Implementa un pipeline automático y secuencial de 7 pruebas diseñadas para estresar el búfer del driver gráfico y validar los canales físicos de la pantalla.

* **Fases del Diagnóstico:**
    1.  **Scanner I2C:** Inspecciona de forma activa el bus para asegurar la respuesta del dispositivo en `0x3C` o `0x3D`. Si el hardware no responde, bloquea preventivamente el script.
    2.  **Mapeo Cartesiano:** Valida la correcta alineación en los ejes X/Y imprimiendo texto estático centrado.
    3.  **Escala de Fuentes:** Modifica dinámicamente el tamaño de la tipografía (escalas 1, 2 y 3) para comprobar la rasterización del motor de `Adafruit_GFX`.
    4.  **Geometría Vectorial:** Renderiza primitivas vectoriales de prueba (rectángulos y círculos, tanto vacíos como rellenos) y líneas divisorias.
    5.  **Test de Píxeles Muertos:** Forza un encendido completo (*All Píxeles ON*) de los 8,192 píxeles de la pantalla. Permite identificar imperfecciones físicas en el panel OLED.
    6.  **Inversión por Hardware:** Invoca de forma nativa la inversión de contraste del chip (`invertDisplay`).
    7.  **Refresco Dinámico:** Renderiza una barra de carga dinámica, calculando los porcentajes en tiempo real con la función matemática `map()`.

---

### 2. 👆 Decodificador de Eventos Táctiles Temporales (`prueba_touch.ino`)
Este módulo convierte lecturas lógicas binarias puras (0 y 1) en interpretaciones complejas basadas en patrones de comportamiento humano. Utiliza programación **no bloqueante** gobernada por marcas de tiempo síncronas (`millis()`), manteniendo idénticos umbrales que el firmware final.

* **Parámetros Críticos de Tiempo:**
    * `HOLD_MS (800ms)`: Tiempo mínimo continuo de presión para declarar un toque largo.
    * `MULTITAP_MS (400ms)`: Ventana de escucha máxima para acumular toques sucesivos y rápidos.
* **Eventos Capturados:**
    * **TAP x1:** Un solo toque corto.
    * **TAP x2:** Doble pulsación rápida.
    * **TAP x3+:** Ráfagas de tres o más pulsaciones sucesivas.
    * **HOLD:** Pulsación larga detectada y emitida inmediatamente de forma prioritaria en cuanto se cumple el tiempo umbral, sin esperar a que el usuario libere el sensor.
* **Feedback en Pantalla:** Despliega un panel en tiempo real con el nombre del último evento procesado, el estado crudo instantáneo del pin con su temporizador interno en milisegundos y un contador acumulativo total de eventos.

---

### 3. 🔈 Suite de Audio y Retroalimentación Interactiva (`prueba_buzzer.ino`)
Este script fusiona el control analítico del sensor táctil, el despliegue del panel visual OLED y el control de frecuencias del buzzer pasivo. Su objetivo primordial es validar que el buzzer instalado tiene la capacidad física de modular su tono de manera dinámica.

* **Validación Crítica de Hardware (El Test Sweep):**
    * Al iniciar, ejecuta un barrido lineal de frecuencias (*Sweep*) que sube desde 200 Hz hasta 3000 Hz, y vuelve a descender.
    * 🚨 **Criterio de Rechazo:** Si el componente emite un sonido constante con un tono fijo, se confirma que es un **Buzzer Activo**. Este tipo de componente **no es apto** para el proyecto final. Debe reemplazarse obligatoriamente por un **Buzzer Pasivo** para poder reproducir las escalas y melodías dinámicas.
* **Pruebas Adicionales Incorporadas:**
    * **Escala Musical:** Ejecuta una secuencia armónica completa de 8 notas (Do, Re, Mi, Fa, Sol, La, Si, Do) imprimiendo los nombres de las notas en el OLED.
    * **Simulación de Firmwares:** Reproduce los efectos de sonido exactos diseñados para el robot final (sonido de interacción de ánimo, sonido de cambio de modo y alertas de temporizadores Pomodoro).
* **Modo Interactivo:** Tras los tests, permite interactuar con el sensor táctil. Un `TAP x1` genera un sonido corto de feedback, un `HOLD` simula un cambio de modo y un `TAP x3+` ejecuta la melodía completa del tema clásico de Tetris, incrementando su velocidad de reproducción en un 25% en cada iteración.

---

### 4. 🎛️ Diagnóstico Base de Temporizadores por Hardware (`prueba_sonido_beta.ino`)
Un script minimalista de bajo nivel diseñado como prueba de concepto (PoC). Prescinde completamente de las librerías estándar de Arduino (como `tone()`) y manipula directamente la API nativa de control de periféricos PWM del microcontrolador ESP32 (`ledc`).

* **Control del Periférico Nativo:**
    * Asigna el pin del altavoz mediante `ledcAttach` especificando una frecuencia de muestreo base y una resolución exacta del temporizador por hardware a 8 bits (256 niveles de modulación).
* **Comportamiento:** Al registrar el estado `HIGH` del sensor táctil, interrumpe el flujo principal de forma controlada y manipula los registros de tiempo de los osciladores internos a través de `ledcWriteTone()`. Genera un arpegio ascendente armónico en Do Mayor (262Hz → 330Hz → 392Hz → 523Hz) para validar la respuesta inmediata del silicio. Integra un retardo de silencio y un mecanismo rudimentario de antirrebote (*debounce* de 500ms) por software.

---

## 💻 Monitor de Telemetría (Salidas por Consola Serie)

Para realizar una depuración profesional, configure la consola del puerto serie a **115200 baudios**. Las tramas de texto estructuradas esperadas se listan a continuación:

### Inicialización y Diagnóstico Gráfico (`prueba_pantalla.ino`):
```text
========================================
  PRUEBA DE PANTALLA — SH1106 OLED
========================================
  SDA=GPIO6  SCL=GPIO7  Dir=0x3C
========================================

[TEST] Scanner I2C
      Buscando dispositivos en el bus I2C...
      Dispositivo encontrado en 0x3C  ← OLED SH1106 ✓
      Total: 1 dispositivo(s) encontrado(s).

[INIT] Inicializando display...
  display.begin() OK


========================================
  PRUEBA BUZZER — pasivo GPIO 19
========================================
  Si el tono no cambia en el sweep:
  -> tu buzzer es ACTIVO, debes cambiarlo.
========================================

[TEST 1] Sweep de frecuencias (200 Hz → 3000 Hz → 200 Hz)
         Si el tono no cambia de pitch: es buzzer ACTIVO, no sirve.
         Sweep completo.
[TEST 2] Escala musical (Do Re Mi Fa Sol La Si Do)
         DO  262 Hz
         RE  294 Hz
         MI  330 Hz
         ...
[TEST 3] Tonos del firmware mochi_unified_5
         Tono tap de animo (800 Hz, 60 ms)...
         Tono cambio de modo (1000 Hz, 80 ms)...
         Tono Pomodoro listo (880 + 1046 Hz)...
         Tonos del firmware completos.

Tests automaticos completos.
Modo interactivo:
  TAP x1  → tono tap
  TAP x3+ → Tetris theme
  HOLD    → tono cambio de modo

🚀 Protocolo de Despliegue Rápido
Entorno de Desarrollo: Abra su entorno de configuración (Arduino IDE o PlatformIO) y configure la placa de desarrollo seleccionando ESP32-C6 Dev Module o su equivalente directo.

Instalación de Dependencias: Diríjase al Administrador de Librerías e instale las siguientes bibliotecas de soporte oficiales:

Adafruit GFX Library (Motor de renderizado vectorial).

Adafruit SH110X (Driver específico para pantallas OLED SH1106).

Montaje Físico: Realice el conexionado eléctrico sobre su protoboard basándose en la sección de Pinout Consolidado.

Carga del Firmware: Suba el código de prueba individual deseado a la placa.

Validación de Resultados: Abra el Monitor Serie para contrastar las métricas de telemetría, verifique los gráficos desplegados en la pantalla OLED y compruebe las variaciones armónicas generadas por el buzzer.