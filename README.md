# STM32 Environmental Data Logger

An embedded data logger built on the STM32 Nucleo G070RB. Reads temperature, humidity, and distance data in real time, stores up to 64 readings in a ring buffer, and provides a full user interface via a 16x2 LCD and IR remote control. Proximity alerts are triggered via a buzzer when distance falls below a configurable threshold.

\---

## Demo

[!\[STM32 DataLogger Demo](https://img.youtube.com/vi/VKbnwb2SH60/maxresdefault.jpg)](https://www.youtube.com/watch?v=VKbnwb2SH60)

> \\\*Click the thumbnail to watch the full demo on YouTube\\\*

The video covers all three operating modes — LIVE sensor monitoring, PLAYBACK scrolling through stored readings, and ALERT CONFIG with live threshold adjustment and LED alert triggering.

\---

## Features

* **Live Sensor Monitoring** — Temperature and humidity via DHT11 (every 2s), distance via HC-SR04 (every 500ms), displayed on 16x2 LCD in real time
* **Ring Buffer Storage** — Circular buffer stores up to 64 `DataPoint\\\_t` readings with overwrite-on-full behaviour
* **IR Remote Control** — Full system control via VS1838B IR receiver with NEC protocol decoding
* **3 Operating Modes** — LIVE, PLAYBACK, and ALERT CONFIG switchable via IR remote
* **Playback Mode** — Scroll through all 64 stored readings using CH+/CH- with wraparound navigation
* **Configurable Proximity Alert** — Distance threshold adjustable from 2cm to 400cm via IR remote, buzzer triggers when breached
* **I2C LCD Driver** — Custom bare-metal HD44780 driver over PCF8574 I2C backpack, written from scratch
* **Non-blocking Architecture** — All timing via `HAL\\\_GetTick()`, no `HAL\\\_Delay` in main loop
* **UART Streaming** — Human-readable sensor data streamed at 115200 baud for PC monitoring

\---

## Hardware

|Component|Part|Purpose|
|-|-|-|
|MCU Board|STM32 Nucleo G070RB|Main microcontroller|
|Temperature/Humidity Sensor|DHT11|Reads ambient temp and humidity|
|Ultrasonic Distance Sensor|HC-SR04|Measures distance (2cm - 400cm)|
|IR Receiver|VS1838B|Receives NEC IR remote signals|
|IR Remote|Generic 21-button NEC remote|User input|
|LCD Display|1602B with PCF8574 I2C backpack|System display|
|Buzzer|Active buzzer (5V)|Proximity alert|
|NPN Transistor|2N2222A|Buzzer driver circuit|
|Resistor|1kΩ|Base resistor for transistor|
|Resistors|10kΩ × 2|Voltage divider for HC-SR04 ECHO pin (5V → 3.3V)|

\---

## Wiring

### Pin Assignments

|STM32 Pin|Label|Connected To|
|-|-|-|
|PA1|DHT11\_DATA|DHT11 DATA pin|
|PA2|USART2\_TX|ST-Link (UART monitor)|
|PA3|USART2\_RX|ST-Link (UART monitor)|
|PA4|HCSR04\_TRIG|HC-SR04 TRIG|
|PA7|HCSR04\_ECHO|HC-SR04 ECHO (via 10kΩ voltage divider)|
|PA8|IR\_INPUT|VS1838B OUT|
|PB0|BUZZER|2N2222A Base (via 1kΩ resistor)|
|PB8|I2C1\_SCL|LCD PCF8574 SCL|
|PB9|I2C1\_SDA|LCD PCF8574 SDA|

### HC-SR04 Voltage Divider

The HC-SR04 ECHO pin outputs 5V which exceeds the STM32G070 GPIO maximum of 3.3V. A voltage divider is used to level shift:

```
HC-SR04 ECHO ── 10kΩ ── PA7
                    │
                  10kΩ
                    │
                   GND
```

### Buzzer Transistor Driver

The active buzzer draws more current than the STM32 GPIO can safely supply. A 2N2222A NPN transistor is used as a switch:

```
3.3V (PB0) ── 1kΩ ── Base (B)
                     2N2222A
                     Collector (C) ── Buzzer negative
                     Emitter (E)   ── GND

Buzzer positive ── 5V
```

\---

## Software Architecture

### System Modes

The system operates as a state machine with three modes, switchable via IR remote:

```
┌─────────────┐    Button 1 / Button 0     ┌─────────────┐
│             │ ◄────────────────────────── │             │
│  MODE\\\_LIVE  │                             │ MODE\\\_PLAYBACK│
│             │ ──────── Button 2 ────────► │             │
└──────┬──────┘                             └─────────────┘
       │                                          ▲
       │ Button 3                                 │ Play/Pause (confirm)
       ▼                                          │
┌─────────────────┐                               │
│ MODE\\\_ALERT\\\_     │ ────────────────────────────► │
│ CONFIG          │
└─────────────────┘
```

### Timer Assignments

|Timer|Purpose|Prescaler|Resolution|
|-|-|-|-|
|TIM14|µs delay + DHT11 timing|63|1µs|
|TIM16|HC-SR04 echo measurement|63|1µs|
|TIM17|IR remote pulse measurement|63|1µs|

### Ring Buffer

A circular buffer stores up to 64 `DataPoint\\\_t` structs. When full, the oldest entry is overwritten. Random access by index is supported for playback mode.

```c
typedef struct {
    uint8\\\_t  temperature;   // °C
    uint8\\\_t  humidity;      // %
    uint16\\\_t distance;      // cm
    uint32\\\_t timestamp;     // ms since boot
} DataPoint\\\_t;
```

### IR Remote Button Mapping

|Button|Code|Action|
|-|-|-|
|1|0x0C|Switch to LIVE mode|
|2|0x18|Switch to PLAYBACK mode|
|3|0x5E|Switch to ALERT CONFIG mode|
|0|0x16|Return to LIVE mode|
|CH+|0x47|Next entry in playback|
|CH-|0x45|Previous entry in playback|
|Vol+|0x15|Increase distance threshold|
|Vol-|0x07|Decrease distance threshold|
|Play/Pause|0x43|Confirm threshold and return to LIVE|

### Non-blocking Timing

No `HAL\\\_Delay` is used in the main loop. All periodic tasks are scheduled using `HAL\\\_GetTick()` timestamps:

```c
if(now - last\\\_dht11\\\_tick >= 2000){   // DHT11 every 2000ms
    last\\\_dht11\\\_tick = now;
    // read sensor
}

if(now - last\\\_hcsr04\\\_tick >= 500){   // HC-SR04 every 500ms
    last\\\_hcsr04\\\_tick = now;
    // read sensor
}
```

\---

## Project Structure

```
DataLogger/
├── Core/
    ├── Inc/
    │   ├── DHT11.h          # DHT11 driver header
    │   ├── HCSR04.h         # HC-SR04 driver header
    │   ├── ring\\\_buffer.h    # Ring buffer header
    │   ├── lcd.h            # LCD I2C driver header
    │   ├── ir\\\_remote.h      # IR remote driver header
    │   └── utils.h          # Microsecond delay utility
    └── Src/
        ├── DHT11.c          # Bit-bang DHT11 driver
        ├── HCSR04.c         # HC-SR04 ultrasonic driver
        ├── ring\\\_buffer.c    # Circular buffer implementation
        ├── lcd.c            # HD44780 over PCF8574 I2C driver
        ├── ir\\\_remote.c      # NEC IR protocol decoder (EXTI + timer)
        ├── utils.c          # Timer-based microsecond delay
        └── main.c           # System state machine and main loop

    
```

\---

## Build and Flash

1. Clone the repository
2. Open STM32CubeIDE and select **File → Open Projects from File System**
3. Select the cloned folder
4. Build with **Project → Build All**
5. Connect the Nucleo board via USB
6. Flash with **Run → Run**

**Toolchain:** STM32CubeIDE 2.0.0
**HAL Version:** STM32G0xx HAL  
**System Clock:** 64MHz (16MHz HSI × PLL)  
**UART Monitor:** 115200 baud, 8N1 (use PuTTY, or Arduino Serial Monitor)

\---

## Key Technical Notes

* **STM32G0 EXTI** uses `HAL\\\_GPIO\\\_EXTI\\\_Falling\\\_Callback` not `HAL\\\_GPIO\\\_EXTI\\\_Callback` — different from F4/F1 families. This was a major pain point while debugging. The interrupt fired but the callback function failed to run. The relevant registers were seen to be configured correctly as well
* **VS1838B** combines the 9ms LOW and 4.5ms HIGH IR leader into a single \~13500µs pulse — the driver detects this directly in the IDLE state
* **HC-SR04 ECHO** outputs 5V and must be level-shifted before connecting to the STM32 GPIO

\---

## Planned Improvements

* \[ ] FreeRTOS migration — sensor, display, IR, and alert as independent tasks with queue-based communication
* \[ ] UART CSV streaming — structured output for PC-side data logging
* \[ ] Python visualizer — real-time matplotlib dashboard over serial

\---

## Author

**Parth Parolekar**  
GitHub: [github.com/ParthParolekar](https://github.com/ParthParolekar)

