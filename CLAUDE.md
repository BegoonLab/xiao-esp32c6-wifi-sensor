# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a power-efficient IoT environmental sensor built on the XIAO ESP32C6 microcontroller. It measures temperature, humidity, pressure, and air quality (VOC/NOx) using BME280/BME680 and SGP41 sensors. The firmware supports two connectivity modes: Wi-Fi with MQTT and ZigBee.

## Build Commands

This project uses ESP-IDF v5.5.1 with CMake. You must have ESP-IDF installed locally.

```bash
# Clone with submodules
git clone https://github.com/BegoonLab/xiao-esp32c6-wifi-sensor
cd xiao-esp32c6-wifi-sensor
git submodule update --init --recursive

# Configure the project
idf.py menuconfig

# Build
idf.py build

# Flash to device (replace <PORT> with actual port, e.g., /dev/ttyUSB0)
idf.py -p <PORT> flash

# Build, flash, and monitor in one command
idf.py -p <PORT> build flash monitor
```

## Linting

Uses Trunk for linting and formatting:

```bash
trunk check        # Run all linters
trunk fmt          # Auto-format code
trunk check --all  # Check all files, not just changed
```

Key linters: clang-format, clang-tidy, cmake-format (C/C++), black, ruff (Python), shellcheck (shell scripts).

Ignored paths: `managed_components/`, `build/`, `cmake-*/`, `vendor/`

## Architecture

### Connectivity Modes (Mutually Exclusive)

The firmware supports two connectivity modes, selected at compile time via `idf.py menuconfig` under "XIAO Sensor Configuration" → "Select Sensor Connection Type":

1. **Wi-Fi/MQTT** (`CONFIG_SENSOR_CONNECTION_WIFI`): Wakes from deep sleep, connects to WiFi, publishes sensor data to MQTT broker, returns to deep sleep
2. **ZigBee** (`CONFIG_SENSOR_CONNECTION_ZIGBEE`): Long-running ZigBee end device (note: SGP41 not supported in ZigBee mode due to protocol limitations)

### Main Source Files (`main/`)

| File | Purpose |
|------|---------|
| `main.c` | Application entry point, orchestrates initialization and main loop based on connectivity mode |
| `sensor_data.h/c` | Unified sensor data structure with mutex-protected access |
| `sensor_bme.h/c` | BME280/BME680 I2C driver for temperature/humidity/pressure |
| `sensor_sgp.h/c` | SGP41 VOC/NOx sensor driver |
| `sensor_adc.h/c` | Battery voltage monitoring via ADC |
| `sensor_mqtt.h/c` | MQTT client for WiFi mode |
| `sensor_wifi.h/c` | WiFi connection management |
| `sensor_zb.h/c` | ZigBee protocol implementation |
| `sensor_sleep.h/c` | Deep sleep power management |
| `sensor_nvs.h/c` | Non-volatile storage for persistent data |

### Initialization Flow

```
app_main() → init_nvs() → init_gpio() → init_led() → init_i2c()
    ↓
[Based on CONFIG_SENSOR_CONNECTION_*]
    ├── WiFi: init_mqtt → read_sensors → mqtt_publish → go_sleep (repeats)
    └── ZigBee: init_zb → start_zb (long-running)
```

### Vendor Libraries

Located in `vendor/`: Bosch Sensortec BME280/BME68x APIs, Sensirion SGP41 driver and gas index algorithm.

## Key Configuration Paths

- `idf.py menuconfig` → "XIAO Sensor Configuration": WiFi credentials, MQTT settings, sensor type, connectivity mode, sleep duration
- `idf.py menuconfig` → "Component config" → "Log output": Enable logging for debugging (default: disabled)
