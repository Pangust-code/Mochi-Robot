# Arquitectura del sistema — Mochi Robot

Este documento describe la arquitectura completa del robot Mochi con diagramas Mermaid.

---

## Diagrama de componentes de hardware

```mermaid
graph TD
    subgraph Hardware["🔧 Hardware físico"]
        ESP["ESP32-C6 Supermini\n(microcontrolador)"]
        OLED["OLED SH1106 128×64\nI2C → GPIO 6/7"]
        MIC["Micrófono INMP441\nI2S → GPIO 3/4/5"]
        TOUCH["Sensor TTP223\nGPIO 2"]
        BUZZ["Buzzer pasivo\nGPIO 19"]
        FLASH["Flash interna\nLittleFS (GIFs .bin)"]
        WIFI["WiFi integrado\nNTP / HTTP"]
    end

    ESP -->|"I2C 400kHz"| OLED
    ESP -->|"I2S 16kHz"| MIC
    ESP -->|"GPIO digital"| TOUCH
    ESP -->|"PWM ledcWrite"| BUZZ
    ESP <-->|"SPI interno"| FLASH
    ESP <-->|"802.11"| WIFI
```

---

## Diagrama de software — flujo principal del firmware

```mermaid
flowchart TD
    START([Encendido / reset]) --> SETUP[setup\nInicializa OLED · I2S · LittleFS · WiFi]
    SETUP --> LOOP[loop]

    LOOP --> TOUCH_READ[procesTouch\nLee GPIO 2 una vez]
    TOUCH_READ --> EVENTS{Eventos\npublicados}

    EVENTS -->|ev_shortTap| TAP_EVENT[Tap corto]
    EVENTS -->|ev_longHold| HOLD_EVENT[Hold largo]
    EVENTS -->|ev_tapsReady| TAPS_EVENT[Ventana de taps]
    EVENTS -->|ev_isHolding| HOLDING[Manteniendo]

    HOLD_EVENT --> MODE_CHANGE[handleModeChange\nAvanza al siguiente modo]
    MODE_CHANGE --> LOOP

    LOOP --> MIC_READ[leerAmplitud\nI2S DMA]
    MIC_READ --> NOISE{Ruido\n> umbral?}
    NOISE -->|Sí| STARTLED[isStartled = true\nMood SURPRISED]
    NOISE -->|No| MODE_RENDER

    STARTLED --> MODE_RENDER

    MODE_RENDER{appMode} -->|0| MASCOTA[Modo 0\nMascota]
    MODE_RENDER -->|1| GIFS[Modo 1\nGIFs LittleFS]
    MODE_RENDER -->|2| RELOJ[Modo 2\nReloj NTP]
    MODE_RENDER -->|3| POMODORO[Modo 3\nPomodoro]
    MODE_RENDER -->|4| MIC_TEST[Modo 4\nTest micrófono]

    MASCOTA --> EYE_RENDER[Motor de ojos\nspring-damper]
    EYE_RENDER --> OLED_OUT[Pantalla OLED]
    GIFS --> OLED_OUT
    RELOJ --> OLED_OUT
    POMODORO --> OLED_OUT
    MIC_TEST --> OLED_OUT

    OLED_OUT --> LOOP
```

---

## Diagrama de estados — sistema de moods

```mermaid
stateDiagram-v2
    [*] --> NORMAL

    NORMAL --> HAPPY : tap aleatorio
    NORMAL --> SURPRISED : sonido fuerte
    NORMAL --> SLEEPY : tap aleatorio
    NORMAL --> ANGRY : tap aleatorio
    NORMAL --> SAD : tap aleatorio
    NORMAL --> EXCITED : tap aleatorio
    NORMAL --> LOVE : tap aleatorio
    NORMAL --> SUSPICIOUS : tap aleatorio
    NORMAL --> DIZZY : tap aleatorio

    SURPRISED --> NORMAL : 2 segundos

    HAPPY --> NORMAL : siguiente tap
    SLEEPY --> NORMAL : siguiente tap
    ANGRY --> NORMAL : siguiente tap
    SAD --> NORMAL : siguiente tap
    EXCITED --> NORMAL : siguiente tap
    LOVE --> NORMAL : siguiente tap
    SUSPICIOUS --> NORMAL : siguiente tap
    DIZZY --> NORMAL : siguiente tap
```

---

## Diagrama de estados — modos de operación

```mermaid
stateDiagram-v2
    [*] --> Mascota

    Mascota --> GIFs : hold largo (≥800ms)
    GIFs --> Reloj : hold largo
    Reloj --> Pomodoro : hold largo
    Pomodoro --> MicTest : hold largo
    MicTest --> Mascota : hold largo

    note right of Mascota
        1 tap → mood aleatorio
        3+ taps → Tetris 🎵
        sonido fuerte → susto
    end note

    note right of GIFs
        Reproduce .bin en secuencia
        20 FPS desde LittleFS
    end note

    note right of Reloj
        Requiere WiFi
        NTP pool.ntp.org
        UTC-5 Ecuador
    end note

    note right of Pomodoro
        1 tap → inicio/pausa
        3+ taps → reiniciar
        25 min trabajo + 5 min descanso
    end note

    note right of MicTest
        1 tap → reinicia pico
        Barra de volumen en tiempo real
    end note
```

---

## Diagrama de comunicación hardware

```mermaid
graph LR
    subgraph ESP32_C6["ESP32-C6 Supermini"]
        I2C_CTRL["I2C Controller\n(400kHz)"]
        I2S_RX["I2S Receiver\n(16kHz, 16-bit)"]
        GPIO_CTRL["GPIO Controller"]
        LEDC["LEDC PWM\n(tone generator)"]
        FS["LittleFS\n(flash interna)"]
        NTP["WiFi Stack\n(NTP client)"]
    end

    I2C_CTRL -->|"SDA GPIO6\nSCL GPIO7"| OLED_HW["OLED SH1106"]
    I2S_RX -->|"SCK GPIO4\nWS GPIO5\nSD GPIO3"| MIC_HW["INMP441"]
    GPIO_CTRL -->|"GPIO2 INPUT"| TOUCH_HW["TTP223"]
    LEDC -->|"GPIO19 OUTPUT"| BUZZ_HW["Buzzer pasivo"]
    FS <-->|"SPI flash"| FRAMES["Frames .bin\n(128×64 px, 1bpp)"]
    NTP <-->|"802.11 WiFi"| INTERNET["Internet\npool.ntp.org"]
```

---

## Módulos del firmware y sus responsabilidades

```mermaid
graph TD
    subgraph Input["Entrada"]
        PT[procesTouch\nGPIO2 → eventos]
        LA[leerAmplitud\nI2S → amplitud]
    end

    subgraph Logic["Lógica"]
        MC[handleModeChange\ngestiona transiciones]
        MS[Sistema de moods\ncurrentMood]
        EYE[Motor de ojos\nspring-damper physics]
    end

    subgraph Output["Salida"]
        ADAFRUIT[Adafruit SH110X\nfillRoundRect · drawLine]
        U8G2[U8g2\nbitmaps de GIF]
        TONE[tone / ledcWriteTone\nbuzzer]
    end

    PT --> MC
    PT --> MS
    LA --> MS
    MS --> EYE
    EYE --> ADAFRUIT
    ADAFRUIT --> OLED_SCR[(Pantalla OLED)]
    U8G2 --> OLED_SCR
    MC --> TONE
    MS --> TONE
```
