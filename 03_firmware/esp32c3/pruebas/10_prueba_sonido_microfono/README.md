# 🎙️ Módulo de Adquisición de Audio I2S: ESP32-C3 Super Mini + INMP441

[cite_start]Este repositorio contiene la herramienta de validación de hardware para el micrófono digital omnidireccional INMP441[cite: 292]. [cite_start]Este script de diagnóstico no solo verifica que el protocolo de comunicación I2S esté operando correctamente, sino que procesa el búfer de audio en tiempo real para extraer amplitudes, suavizar señales y clasificar el entorno acústico[cite: 292, 293].

Es una prueba integral que debe ejecutarse *después* de validar la continuidad eléctrica de los cables, garantizando que el microcontrolador es capaz de "escuchar" e interpretar sonido ambiente antes de implementar lógicas reactivas complejas.

---

## 🛠️ Arquitectura de Hardware y Pinout

El INMP441 es un dispositivo digital que se comunica mediante el bus I2S. [cite_start]Las conexiones deben realizarse estrictamente de la siguiente manera[cite: 294, 295, 310]:

| Pin INMP441 | Pin ESP32-C3 Super Mini | Función Lógica (I2S) | Descripción |
| :--- | :--- | :--- | :--- |
| **VDD** | 3.3V | Alimentación | Voltaje operativo del micrófono. |
| **GND** | GND | Tierra | Referencia común de potencial. |
| **L/R** | GND | Selección de Canal | [cite_start]Al conectarlo a Tierra, el micrófono transmite datos por el canal izquierdo (Left). |
| **SCK** | **GPIO 4** | Reloj de Bits (BCLK) | [cite_start]Sincronización serial de los bits de datos[cite: 294, 295]. |
| **WS** | **GPIO 1** | Word Select (LRCK) | [cite_start]Selección de canal para la trama (izquierdo o derecho)[cite: 294, 295]. |
| **SD** | **GPIO 3** | Serial Data | [cite_start]Salida digital del flujo de audio PCM[cite: 294, 295]. |

> [cite_start]⚠️ **Importante:** El pin **L/R** debe estar forzosamente puenteado a **GND**. Si se deja flotando, el canal de datos no estará definido correctamente.

---

## 🧠 Desglose Lógico e Ingeniería de Audio

Este script transforma datos crudos de hardware en información acústica útil empleando varias técnicas de procesamiento digital de señales (DSP) básicas:

### 1. Configuración del Driver I2S (DMA)
[cite_start]El microcontrolador se configura en modo Maestro-Receptor (`I2S_MODE_MASTER | I2S_MODE_RX`), controlando los relojes del bus[cite: 296, 297]. [cite_start]La captura se realiza a una frecuencia de muestreo de **16,000 Hz** con una resolución de **16 bits** en formato mono (solo canal izquierdo)[cite: 297]. [cite_start]Utiliza 4 búferes DMA de 256 muestras para evitar cuellos de botella en el procesador principal[cite: 295, 297].

### 2. Cálculo de Amplitud (Pico a Pico)
En lugar de procesar frecuencias complejas (FFT), el sistema evalúa la energía bruta de la señal. [cite_start]Captura las 256 muestras, busca el valor máximo y el valor mínimo dentro de ese paquete, y calcula la diferencia absoluta[cite: 302, 304, 305]. Este método filtra automáticamente los desplazamientos de corriente continua (DC Offset).

### 3. Suavizado Exponencial de Señal
Para evitar que el renderizado de la barra visual sea errático y difícil de leer, se aplica un filtro matemático de media móvil exponencial: 
[cite_start]`suavizado = (suavizado * 5 + amp * 3) / 8`[cite: 318]. 
[cite_start]Esto da mayor peso al historial reciente, creando una animación de barra ASCII fluida en la consola[cite: 293, 318].

### 4. Clasificación Dinámica de Umbrales
[cite_start]El sistema decodifica el nivel de amplitud en cuatro estados humanos comprensibles:
* [cite_start]**Silencio ambiental:** `< 2,000` (El ruido de fondo normal de una habitación)[cite: 294, 320].
* [cite_start]**Ruido / Voz:** `2,000 – 10,000` (Conversaciones a volumen normal)[cite: 294, 321].
* [cite_start]**Voz Fuerte:** `10,000 – 25,000` (Aplausos, hablar cerca del micrófono)[cite: 294, 322].
* [cite_start]**Saturación:** `> 25,000` (Gritos directos al sensor o golpes)[cite: 294, 323].

---

## 💻 Telemetría y Monitor Serie

[cite_start]Para evaluar el sistema, abre el monitor serie de tu IDE configurado a **115200 baudios**. [cite_start]Observarás un dashboard que se actualiza a 10 Hz (cada 100 ms) detallando la amplitud actual, el pico histórico, la clasificación de ruido y una barra ASCII gráfica[cite: 315, 316, 324]:

```text
========================================
  PRUEBA DE SONIDO — INMP441
========================================
  SCK=GPIO4  WS=GPIO1  SD=GPIO3
  L/R → GND
========================================
  Iniciando I2S...
  Listo.

  Guia de valores:
    Silencio      :     0 –  2000
    Ruido / voz   :  2000 – 10000
    Voz fuerte    : 10000 – 25000
    Muy fuerte    : > 25000

  Amp    | Pico  | Nivel        | Barra
  -------|-------|--------------|------------------------------------------
     850  |   850 | silencio     | |###                                     |
    5200  |  5200 | ruido / voz  | |#############                           |
   14300  | 14300 | voz fuerte   | |############################            |
   31000  | 31000 | MUY FUERTE!  | |#####################################!!!|
```
🔧 Solución de Problemas (Troubleshooting):

-> ERROR al instalar driver I2S: Ocurre si hay un conflicto de pines en el microcontrolador. Verifica que ningún otro periférico esté usando los GPIOs 3, 4 o 1.  

-> Valores estancados en 0 o 65535: El micrófono está energizado, pero la línea de datos (SD en el GPIO 3) está desconectada, o el pin L/R no está anclado a GND.  

-> No se leyeron datos del micrófono: El I2S inició, pero el bus no está generando la interrupción del reloj. Revisa exhaustivamente los cables SCK (GPIO 4) y WS (GPIO 1).  
