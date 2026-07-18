# Hardware Overview

## Core Components

| Component | Role |
|---|---|
| ESP32 WROOM DevKit (38-pin, CP2102) | Main microcontroller, handles input reading, BLE communication, and firmware logic |
| Ginfull R-5B / R-5C hall-effect joysticks | Analog stick input, chosen for durability and drift resistance over potentiometer sticks |
| MAX17048 | Battery fuel gauge — provides accurate state-of-charge readings |
| FM5324UI (IP5306 clone) | Battery charging and power management |
| SSD1306 OLED (I2C) | Status display (battery %, active profile) |
| Custom protoboard | Houses all electronics |

## Power Architecture

- The **FM5324UI** handles battery charging and overall power delivery. Its `K` pin connects through the physical on/off button directly to GND — no external resistor is needed, since the chip has an internal pull-up.
- The **MAX17048** fuel gauge is wired directly across the battery terminals, in parallel with the charging circuit. This placement means it reads the battery's true state without interfering with charging.

<!-- Full details in [PowerDesign.md](../hardware/power/PowerDesign.md) and [BatteryCalibration.md](../hardware/power/BatteryCalibration.md). -->

## GPIO Considerations

GPIO36 (used for the profile button) is an **input-only pin with no internal pull-up**, requiring an external 10kΩ pull-up resistor to 3.3V. This is a hard hardware constraint of the ESP32 and must be accounted for in the wiring.

Full pin assignments are documented in [`Pinout.md`](../hardware/pinout/Pinout.md).

## Enclosure

All electronics — ESP32, protoboard, joysticks, power management, fuel gauge, and OLED are held together by the **back shell** (`green_back.stl`) and the **front shell** (`black_front_05.stl`), which are purely mechanical, providing only cutout windows for buttons, sticks, and the display; no components attach to them directly.

The OLED display, profile button and on/off switch are mounted on a suspended support platform.

The overall shell shape and ergonomics are inspired by the **Alpakka 1 controller by Input Labs**, adapted to this project's specific internal component layout.

See [`enclosure/`](../enclosure/) for 3D source files and exports, and [`hardware/schematics/`](../hardware/schematics/) for circuit schematics.

## Datasheets

Component datasheets are available in [`hardware/datasheets/`](../hardware/datasheets/):
- ESP32
- MAX17048
- IP5306 / FM5324UI
