# 🌐 Módulo de Conectividad y Reloj RTC: ESP32-C6 + WiFi + NTP

Este repositorio contiene la herramienta de diagnóstico de red diseñada para certificar las capacidades inalámbricas del hardware antes de integrarlo al firmware final del robot interactivo. [cite_start]Este script verifica la conexión WiFi y la sincronización de tiempo mediante NTP, utilizando la pantalla OLED, el sensor táctil y el buzzer pasivo como sistema unificado de retroalimentación[cite: 140]. 

[cite_start]El script reproduce de forma exacta la lógica y la interfaz gráfica del "Modo 2 (Reloj)" del firmware `mochi_unified_5`[cite: 141].

---

## 🛠️ Arquitectura de Hardware y Pinout

[cite_start]Para garantizar la interoperabilidad de todo el sistema interactivo, el hardware debe estar configurado bajo el siguiente esquema de pines[cite: 149]:

| Componente | Pin ESP32-C6 | Protocolo / Función Lógica |
| :--- | :--- | :--- |
| **OLED SH1106** | **GPIO 6** | [cite_start]SDA (Bus de Datos I2C)[cite: 144]. |
| **OLED SH1106** | **GPIO 7** | [cite_start]SCL (Bus de Reloj I2C)[cite: 144]. |
| **TTP223 Touch** | **GPIO 2** | [cite_start]Entrada Digital (Interrupción táctil)[cite: 144]. |
| **Buzzer Pasivo**| **GPIO 19**| [cite_start]Salida PWM (Generación de feedback de audio)[cite: 144]. |

---

## ⚙️ Flujo de Ejecución Automático

[cite_start]Al energizar el microcontrolador, el sistema ejecuta una secuencia de inicialización autónoma rigurosa[cite: 142]:

1. [cite_start]**Escaneo de Espectro:** Realiza un barrido de redes WiFi disponibles, listando los SSID y la potencia de la señal (RSSI en dBm) en la pantalla OLED[cite: 142, 162].
2. [cite_start]**Autenticación (Conexión):** Intenta establecer un enlace con la red preconfigurada mostrando una animación de progreso mediante barra de puntos (máximo 20 intentos)[cite: 142, 164, 165, 176].
3. [cite_start]**Sincronización NTP:** Tras obtener una IP, se conecta al servidor `pool.ntp.org` para solicitar la hora global y aplica la compensación configurada en código para Ecuador (UTC-5 / `-18000` segundos)[cite: 142, 145, 146].
4. [cite_start]**Despliegue de Interfaz:** Si todo es exitoso, renderiza un reloj digital en tiempo real calculando días y meses en formato texto[cite: 142, 168].

---

## 👆 Interfaz Interactiva y Máquina de Estados (UI)

El flujo del programa no utiliza retardos bloqueantes para el reloj. [cite_start]En su lugar, el bucle principal evalúa constantemente los flancos del sensor capacitivo empleando el gestor de estados `enum Vista { ESCANEANDO, CONECTANDO, RELOJ, DETALLES }`[cite: 150].

[cite_start]Los controles táctiles implementados son idénticos a los del firmware de producción[cite: 142]:
* [cite_start]**TAP x1 (Simple):** Alterna la pantalla entre la vista del "Reloj" en vivo y un dashboard de "Detalles de Red" que expone la IP local, la IP de la puerta de enlace (Gateway), el servidor DNS y la latencia RSSI[cite: 142, 172, 173].
* [cite_start]**TAP x3+ (Multi-Tap):** Aborta el estado actual y fuerza un nuevo escaneo físico de redes cercanas[cite: 142, 143, 196].
* [cite_start]**HOLD (Pulsación Larga):** Desconecta agresivamente el módulo WiFi del router actual, limpia las credenciales en caché y reinicia todo el ciclo de conexión y sincronización desde cero[cite: 143, 187, 188].

---

## 🔈 Sistema de Retroalimentación Acústica

[cite_start]Para minimizar la dependencia visual durante la depuración de hardware, el script incluye un sistema de notificaciones por hardware PWM (Buzzer)[cite: 143]:

* [cite_start]**Doble Beep Ascendente:** Confirmación de que el ESP32-C6 ha recibido una dirección IP del router[cite: 143].
* [cite_start]**Triple Beep Agudo:** Confirmación de que el paquete de tiempo (NTP) ha sido recibido y decodificado correctamente[cite: 143].
* [cite_start]**Tono Grave Descendente:** Alerta crítica que indica un timeout en la conexión WiFi o una nula respuesta del servidor NTP[cite: 143].

---

## 🚀 Guía de Despliegue y Configuración

Antes de compilar y subir el firmware al microcontrolador, es estrictamente necesario actualizar las credenciales de la red local.

1. Abre el archivo `prueba_wifi.ino`.
2. Dirígete a la sección de **Credenciales** (líneas ~40-41) y modifica las variables con la información de tu entorno local:
   ```cpp
   const char* WIFI_SSID = "TU_NOMBRE_DE_RED"; // Reemplazar "Pixel_4531" [cite: 144]
   const char* WIFI_PASS = "TU_CONTRASEÑA";    // Reemplazar "tigrillo" [cite: 145]
   ```
3. Verifica que tienes las librerías Adafruit_GFX y Adafruit_SH110X instaladas.

4. Compila, sube el código y observa el monitor serie (115200 baudios) para una telemetría paso a paso.