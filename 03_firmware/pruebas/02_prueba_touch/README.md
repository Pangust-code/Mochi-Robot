# 🤖 Suite de Pruebas de Hardware: ESP32-C6 Supermini + OLED SH1106 + TTP223

[cite_start]Este repositorio contiene las herramientas de diagnóstico y validación esenciales para verificar el correcto funcionamiento físico y lógico de los componentes antes de su integración en firmwares complejos[cite: 48]. [cite_start]El objetivo de estos scripts es aislar y asegurar de forma independiente el comportamiento de la pantalla OLED SH1106 de 128×64  [cite_start]y el sensor táctil capacitivo TTP223.

---

## 🛠️ Arquitectura de Hardware y Pinout

Para que los scripts funcionen correctamente, el cableado entre el microcontrolador ESP32-C6 y los periféricos debe realizarse siguiendo estrictamente la siguiente distribución de pines:

| Componente | Pin Periférico | Pin ESP32-C6 | Función Lógica | Descripción |
| :--- | :--- | :--- | :--- | :--- |
| **OLED SH1106** | VCC | 3.3V | Alimentación | [cite_start]Voltaje operativo del panel[cite: 3]. |
| **OLED SH1106** | GND | GND | Tierra | Referencia común de potencial. |
| **OLED SH1106** | SDA | **GPIO 6** | Bus de Datos I2C | [cite_start]Transmisión de datos bidireccional[cite: 3, 49]. |
| **OLED SH1106** | SCL | **GPIO 7** | Bus de Reloj I2C | [cite_start]Señal de sincronización de reloj[cite: 3, 49]. |
| **TTP223 Touch** | SIG | **GPIO 2** | Entrada Digital | [cite_start]Lectura del estado del sensor[cite: 49]. |

* [cite_start]**Dirección I2C por Defecto:** `0x3C`[cite: 4, 49].

---

## 📂 Desglose de los Módulos de Diagnóstico

### [cite_start]1. 📺 Diagnóstico Gráfico Integral (`prueba_pantalla.ino`) 

[cite_start]Este módulo ejecuta un pipeline automático secuencial de 7 pruebas diseñadas para estresar el búfer gráfico de la pantalla y validar el canal de comunicación I2C[cite: 1, 2].

#### Flujo de los Tests Visuales:
* [cite_start]**Test 1 — Scanner I2C:** Realiza un barrido síncrono en todo el bus (direcciones 1 a 126) [cite: 9] [cite_start]para confirmar que el display responde en la dirección configurada. [cite_start]Si no detecta dispositivos, emite una alerta por puerto serie[cite: 13, 14].
* [cite_start]**Test 2 — Texto Básico:** Limpia el búfer y renderiza el mensaje estático "PANTALLA OK :)" para validar el mapa de coordenadas cartesianas básico[cite: 16, 17].
* [cite_start]**Test 3 — Tamaños de Texto:** Imprime tres líneas consecutivas variando la escala tipográfica entre los tamaños 1, 2 y 3 para verificar la legibilidad y fuentes integradas[cite: 18, 19, 20].
* [cite_start]**Test 4 — Formas Geométricas:** Dibuja de forma vectorial rectángulos (vacíos y con relleno), círculos (vacíos y con relleno) y líneas de separación, validando la precisión geométrica del driver de pantalla[cite: 21, 22, 23, 24, 25].
* [cite_start]**Test 5 — Todos los Píxeles ON:** Enciende simultáneamente cada píxel de la matriz de 128x64 de forma individual[cite: 27, 28]. [cite_start]Esto permite realizar una inspección a simple vista para identificar "píxeles muertos" o zonas dañadas en el panel físico[cite: 30].
* [cite_start]**Test 6 — Inversión de Contraste:** Invoca el comando nativo `invertDisplay()` alternando entre visualización normal e invertida (texto negro sobre fondo blanco)[cite: 31, 33].
* [cite_start]**Test 7 — Animación de Barra de Carga:** Modifica en tiempo real el ancho de un rectángulo interior basándose en incrementos forzados[cite: 34, 35]. [cite_start]Traduce el ancho del gráfico a un valor numérico porcentual mediante la función matemática `map()` para certificar la tasa de refresco fluida[cite: 36].

---

### [cite_start]2. 👆 Intérprete de Eventos Táctiles Avanzado (`prueba_touch.ino`) 

[cite_start]Este script implementa un algoritmo de lectura asíncrona no bloqueante utilizando marcas de tiempo nativas (`millis()`) [cite: 51, 52, 66] [cite_start]para decodificar intenciones del usuario sobre el sensor capacitivo TTP223.

#### Configuración de Umbrales Temporales:
```cpp
#define HOLD_MS       800    // Ventana mínima para pulsación larga (HOLD) [cite: 49]
#define MULTITAP_MS   400    // Ventana máxima para acumular taps sucesivos [cite: 49]
```