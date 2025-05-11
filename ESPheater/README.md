# ESP32 Heater Control with BLE

This project implements a temperature control system using an ESP32 microcontroller, featuring Bluetooth Low Energy (BLE) communication, temperature sensing, and RGB LED status indication.

## Overview

The system monitors temperature using a Dallas DS18B20 sensor, controls a heater to maintain a target temperature, and communicates the system state via BLE to connected devices. An RGB LED indicates the system's current state through different colors.

## Hardware Requirements

- ESP32 Development Board
- DS18B20 Temperature Sensor
- Heater (controlled via a relay or transistor)
- RGB LED (common cathode/anode, depending on setup)
- Push-button for reset
- Resistors and appropriate wiring

## Pin Configuration

- **Temperature Sensor (DS18B20)**: Connected to GPIO 15
- **Heater Control**: Connected to GPIO 13
- **Reset Button**: Connected to GPIO 4 (with internal pull-up)
- **RGB LED**:
  - Red: GPIO 25
  - Green: GPIO 26
  - Blue: GPIO 27

## Software Requirements

- Arduino IDE with ESP32 board support
- Required libraries:
  - `OneWire`
  - `DallasTemperature`
  - `esp32-hal-ledc` (included with ESP32 Arduino core)
  - `ESP32 BLE Arduino` (for BLE functionality)

## Installation

1. **Install Arduino IDE** and add ESP32 board support.
2. **Install Libraries**:
   - Install `OneWire` and `DallasTemperature` via the Arduino Library Manager.
   - Ensure the ESP32 Arduino core is installed (includes BLE and LEDC libraries).
3. **Upload Code**:
   - Connect the ESP32 to your computer.
   - Open the provided `.ino` file in the Arduino IDE.
   - Select the appropriate ESP32 board and port.
   - Upload the code.

## Configuration

The system is configured with the following parameters (defined in the code):

- **Target Temperature**: 98°C
- **Overheat Threshold**: 101°C
- **Stabilizing Margin**: ±2°C
- **BLE Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **BLE Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`

Modify these values in the code if needed.

## System States

The system operates in the following states, indicated by the RGB LED:

- **IDLE** (Blue): System is below target temperature, heater off.
- **HEATING** (Red): Heater is on, warming up to target temperature.
- **STABILIZING** (Yellow): Temperature is near target, fine-tuning.
- **TARGET_REACHED** (Green): Temperature is at target (±0.9°C).
- **OVERHEAT** (White): Temperature exceeds 101°C, heater off until reset.

## BLE Communication

The ESP32 acts as a BLE server, advertising the service UUID. When a client connects, the system notifies the current state (e.g., "HEATING", "IDLE") every 500ms. Use a BLE client app (e.g., nRF Connect) to connect and monitor the characteristic.

## Operation

1. Power on the ESP32.
2. The system initializes and starts advertising via BLE.
3. The temperature sensor reads the current temperature every 50ms.
4. The heater control task manages the heater based on the current state.
5. The logger task updates the Serial monitor, RGB LED, and BLE characteristic.
6. If the system overheats, it enters the OVERHEAT state and requires a reset (press the reset button connected to GPIO 4).

## Tasks

The system uses FreeRTOS tasks pinned to specific cores for efficiency:

- **readTemperatureTask**: Reads temperature from DS18B20 (Core 1).
- **heaterControlTask**: Manages heater state transitions (Core 1).
- **loggerTask**: Logs state to Serial, updates RGB LED, and notifies via BLE (Core 0).

## Notes

- Ensure proper heat dissipation for the heater to prevent overheating.
- The reset button is required to exit the OVERHEAT state.
- The RGB LED uses PWM (LEDC) with 5kHz frequency and 10-bit resolution.
- The system is designed for real-time operation with minimal delays.

## License

This project is licensed under the MIT License.