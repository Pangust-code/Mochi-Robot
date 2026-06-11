# 📺 Diagnóstico Integral: Pantalla OLED SH1106 + ESP32-C6

Este repositorio contiene una herramienta de diagnóstico por software para verificar la correcta conexión física, la comunicación I2C y el renderizado visual de una pantalla OLED SH1106. 

Está diseñado como un script de validación rápida para descartar fallos de hardware en el desarrollo de sistemas embebidos e interfaces IoT.

## 🛠️ Hardware y Conexiones

* **Cerebro del Sistema:** ESP32-C6 Supermini
* **Periférico Visual:** Pantalla OLED SH1106 (128x64 píxeles)
* **Protocolo de Comunicación:** I2C (Dirección por defecto: `0x3C`)

### Pinout Configurado

| Pin OLED | Pin ESP32-C6 | Función |
| :--- | :--- | :--- |
| **VCC** | 3.3V | Alimentación del panel |
| **GND** | GND | Tierra común |
| **SDA** | GPIO 6 | Bus de Datos (Serial Data) |
| **SCL** | GPIO 7 | Bus de Reloj (Serial Clock) |

> ⚠️ **Importante:** Asegúrate de proveer exactamente 3.3V. Un voltaje incorrecto puede dañar el controlador de la pantalla o impedir la comunicación.

## 📦 Dependencias del Entorno

Para compilar este código correctamente, asegúrate de tener instaladas las siguientes librerías en tu entorno de desarrollo (Arduino IDE):

* `Wire.h` (Librería nativa para I2C)
* `Adafruit GFX Library` (Motor gráfico base)
* `Adafruit SH110X` (Controlador específico para el chip SH1106)

## 🚀 Secuencia de Pruebas (Tests Automáticos)

El script está programado para ejecutar una secuencia lógica de 7 pruebas durante la fase de `setup()`. Todo el progreso se reporta simultáneamente por el puerto serie.

1. **Scanner I2C (Prueba de Vida):** Busca dispositivos activos en el bus I2C. Si no detecta la pantalla (direcciones `0x3C` o `0x3D`), aborta las pruebas para evitar bloqueos por software.
2. **Texto Básico:** Verifica el mapeo correcto de coordenadas (X, Y) renderizando el mensaje "PANTALLA OK".
3. **Escala de Tipografías:** Prueba el renderizado dinámico cambiando los tamaños de texto a escalas 1, 2 y 3.
4. **Formas Geométricas:** Evalúa las funciones de dibujo vectorial renderizando líneas, círculos y rectángulos (vacíos y con relleno).
5. **Estrés de Píxeles (Todo ON):** Enciende simultáneamente todos los píxeles del búfer. Ideal para identificar físicamente "píxeles muertos" (puntos negros).
6. **Inversión de Contraste:** Utiliza el comando de hardware `invertDisplay()` para alternar los colores (texto negro sobre fondo blanco) y probar el contraste del panel.
7. **Animación de Renderizado (Barra de Carga):** Simula un proceso de carga incrementando el ancho de un rectángulo y calculando el porcentaje dinámico usando la función `map()`. Evalúa la tasa de refresco de la pantalla.

## 💻 Instrucciones de Uso y Depuración

1. Realiza las conexiones físicas guiándote por la tabla de **Pinout**.
2. Abre el archivo `prueba_pantalla.ino` en tu IDE.
3. Compila y sube el código a la placa ESP32-C6.
4. Abre inmediatamente el **Monitor Serie** configurado a `115200 baudios`.
5. Observa el *log* en la consola mientras comparas los resultados visuales en la pantalla OLED física.

---
*Herramienta de validación de hardware desarrollada para garantizar la integridad de componentes antes de la implementación de lógicas más complejas.*