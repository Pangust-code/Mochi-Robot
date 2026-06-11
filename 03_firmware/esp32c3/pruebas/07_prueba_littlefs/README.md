# 📁 Módulo de Sistema de Archivos y Renderizado de Animaciones: ESP32-C3 Super Mini + LittleFS

[cite_start]Este repositorio contiene la herramienta de diagnóstico encargada de validar la memoria flash del microcontrolador (LittleFS) y el motor de renderizado de video en blanco y negro[cite: 213]. [cite_start]Es un script crítico para garantizar que el ESP32-C3 Super Mini puede leer y decodificar archivos binarios (`.bin`) a altas velocidades antes de implementar las expresiones faciales dinámicas del robot[cite: 213].

Este documento está estructurado para servir como material de exposición técnica, explicando la convivencia de dos motores gráficos y la gestión directa de memoria.

---

## 🛠️ 1. Hardware y Configuración

[cite_start]El hardware mantiene la arquitectura base, sumando la capacidad de almacenar datos persistentes en la partición flash del microcontrolador[cite: 217]:

| Componente | Pin ESP32-C3 Super Mini | Protocolo / Función Lógica |
| :--- | :--- | :--- |
| **OLED SH1106** | **GPIO 8 / 9** | [cite_start]I2C (SDA / SCL) a dirección `0x3C`[cite: 217]. |
| **TTP223 Touch** | **GPIO 2** | [cite_start]Entrada Digital (Control de reproducción)[cite: 217]. |
| **Buzzer** | **GPIO 10** | [cite_start]Salida PWM (Alertas de sistema)[cite: 217]. |

---

## 🧠 2. Desglose del Código: Ingeniería de Software

Este script presenta un nivel avanzado de programación para sistemas embebidos. Aquí se explican los bloques principales para su exposición:

### Parte A: El Sistema de Archivos (LittleFS)
A diferencia del código tradicional de Arduino que se borra al apagarse, este sistema reserva una porción de la memoria flash del ESP32 como si fuera un disco duro.
* **Inicialización:** El sistema ejecuta `LittleFS.begin()`. Si falla, alerta inmediatamente con el buzzer y bloquea el sistema. [cite_start]Esto suele ocurrir si el desarrollador olvidó subir la carpeta `data/` al chip[cite: 254, 255].
* [cite_start]**Escaneo de Directorio:** El código recorre la raíz (`/`) buscando específicamente archivos con la extensión `.bin` (hasta un máximo de 32 archivos) e ignora el resto, calculando cuánto pesan y verificando la salud de la memoria disponible[cite: 258, 259, 260, 261].

### Parte B: El "Hack" de los Dos Motores Gráficos
Para lograr máxima eficiencia, el código instancia **dos librerías gráficas distintas** apuntando a la misma pantalla física:
1. [cite_start]`Adafruit_SH110X`: Se usa para dibujar texto, interfaces y menús (UI) porque es amigable y fácil de formatear[cite: 217, 218].
2. [cite_start]`U8g2lib`: Se usa exclusivamente para reproducir el video[cite: 217, 219]. 
* **¿Por qué?** Porque U8g2 permite acceso directo a su búfer de memoria a bajo nivel. [cite_start]El script utiliza la función `memcpy` de C++ para volcar fragmentos binarios crudos (`frameBuffer`) directamente al procesador de la pantalla, logrando una tasa estable de **20 FPS** sin sobrecargar el microcontrolador[cite: 218, 248, 249].
* [cite_start]Las funciones `activarAdafruit()` y `activarU8g2()` se encargan de alternar el control de la pantalla entre ambos motores sin que colisionen[cite: 219, 220].

### Parte C: Matemáticas del Búfer de Video
[cite_start]Cada cuadro (frame) de la animación pesa exactamente `1024` bytes[cite: 217]. 
* **Explicación para la audiencia:** La pantalla tiene 128 píxeles de ancho por 64 de alto (8,192 píxeles en total). Como la pantalla es monocromática, 1 píxel equivale a 1 bit. [cite_start]Si dividimos 8,192 bits entre 8, obtenemos exactamente **1024 bytes por frame**[cite: 217]. [cite_start]El código divide el peso total del archivo `.bin` entre 1024 para deducir matemáticamente cuántos frames componen la animación[cite: 243, 260].

### Parte D: Máquina de Estados Asíncrona (Touch)
[cite_start]Mientras el video se reproduce dentro de un bucle `for`, la función `procesarTouch()` intercepta interacciones sin usar `delay()`, evaluando[cite: 225, 244]:
* [cite_start]**TAP x1:** Activa `ev_siguiente`, forzando el cierre del archivo actual (`f.close()`) y saltando a la siguiente animación[cite: 214, 231, 245].
* [cite_start]**TAP x3+:** Activa `ev_reiniciar`, devolviendo el índice de animación a 0[cite: 214, 231, 267].
* [cite_start]**HOLD:** Activa un bucle secundario que bloquea el envío de frames a la pantalla (Pausa), mostrando un letrero de "PAUSA" hasta que se detecta un nuevo Hold para reanudar[cite: 214, 246, 247].

---

## ⚠️ 3. Solución de Problemas (Troubleshooting)

Al exponer o probar este módulo, estos son los errores controlados por el sistema operativo del script:

1. **Mensaje "LittleFS ERROR":** El sistema de archivos no existe. 
   * [cite_start]*Solución:* Utiliza el plugin de tu IDE (PlatformIO o Arduino) para ejecutar la tarea "Upload File System Image" o "Upload LittleFS", lo cual transferirá los archivos `.bin` alojados en tu carpeta local `/data` hacia el microcontrolador[cite: 214, 215].
2. **Mensaje "0 archivos" / "Sin archivos .bin":** La partición se montó con éxito, pero está vacía. 
   * [cite_start]*Solución:* Asegúrate de que los archivos de animación tienen la extensión `.bin` estricta y que están dentro de la carpeta `/data` correcta antes de flashear[cite: 215, 260, 263, 264].
3. [cite_start]**Alerta "Frame count = 0":** El archivo fue leído, pero está corrupto o vacío (0 bytes)[cite: 216].

---

## 🚀 4. Guía de Despliegue

1. [cite_start]Instala las dependencias: `Adafruit_GFX`, `Adafruit_SH110X`, `U8g2` y `LittleFS`[cite: 217].
2. Crea una carpeta llamada `data` en el mismo directorio que tu archivo `prueba_littlefs.ino`.
3. Coloca dentro de `data` al menos un archivo binario de animación (ej: `anim1.bin`).
4. **Sube el File System:** Flashea primero el sistema de archivos (data) al ESP32.
5. **Sube el Código:** Compila y sube el archivo `.ino`. 
6. [cite_start]Revisa el monitor serie a 115200 baudios para visualizar el reporte de la memoria KB usada, la memoria libre y la lista detallada de cada archivo procesado[cite: 251, 257, 262].
