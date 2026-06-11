# Sketch — Prueba WiFi + NTP

Verifica que el ESP32-C6 puede conectarse a una red WiFi, obtener una IP y sincronizar la hora con NTP. Reproduce el comportamiento exacto del Modo 2 (Reloj) del firmware principal.

**Secuencia automática:**
1. Escaneo — lista redes disponibles en el OLED y serial
2. Conexión — conecta a la primera red disponible con animación de progreso
3. NTP — sincroniza la hora con `pool.ntp.org` (UTC-5 Ecuador)
4. Reloj — muestra hora y fecha en vivo

**Controles touch (mientras muestra el reloj):**

| Acción | Resultado |
|--------|-----------|
| TAP x1 | Alterna entre vista reloj y detalles de red (IP, RSSI, gateway) |
| TAP x3+ | Vuelve a escanear redes |
| HOLD (>800 ms) | Desconecta y reconecta desde cero |

---

## Paso 1 — Configurar credenciales

Abre `prueba_wifi.ino` y edita el arreglo `networks[]`:

```cpp
WiFiCredential networks[] = {
  // WPA2-Personal (red doméstica)
  {"NOMBRE_DE_TU_RED",  "TU_CONTRASEÑA",  false, "", ""},
  // WPA2-Enterprise (red universitaria — opcional)
  {"RED_UNIVERSITARIA", "",               true,
   "usuario@universidad.edu", "contraseña_eap"},
};
```

> Puedes agregar tantas redes como necesites. El ESP32 las intentará en orden hasta conectarse.
> Para redes WPA2-Personal simples, borra la entrada Enterprise o deja `enterprise: false`.

---

## Paso 2 — Compilar y subir

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/07_prueba_wifi
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c6 03_firmware/pruebas/07_prueba_wifi
arduino-cli monitor -p COM3 --config baudrate=115200
```

---

## Resultado esperado

El Monitor Serie muestra:
```
========================================
  PRUEBA WIFI + NTP
========================================
  Redes configuradas: 2
    [1] NOMBRE_DE_TU_RED
    [2] RED_UNIVERSITARIA (Enterprise)
  NTP:    pool.ntp.org  UTC-5
========================================

Escaneando redes WiFi...
  [1] NOMBRE_DE_TU_RED   -55 dBm
Conectando a NOMBRE_DE_TU_RED...
Conectado a NOMBRE_DE_TU_RED. IP: 192.168.1.XX  RSSI: -55 dBm
Hora sincronizada: 14:32:05  06/06/2026
```

El OLED muestra la hora en grande y la fecha debajo.

---

## Verificación

| Lo que ves | Diagnóstico |
|-----------|-------------|
| Hora en OLED | Todo bien |
| `Timeout en red 1/2` pero conecta a la siguiente | Normal si la primera red no está disponible |
| `ERROR: no se pudo conectar a ninguna red` | Verifica SSID, contraseña y que la red sea 2.4 GHz |
| Hora incorrecta | Verifica `GMT_OFFSET` (Ecuador = `-18000` = UTC-5) |

---

## Pines

| Componente | Pin |
|-----------|-----|
| OLED SDA | GPIO 6 |
| OLED SCL | GPIO 7 |
| Touch TTP223 | GPIO 2 |
| Buzzer | GPIO 19 |
