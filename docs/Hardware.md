# Hardware Overview

## Core Components

| Component | Role |
|---|---|
| ESP32 WROOM DevKit (38-pin, CP2102) | Main microcontroller, handles input reading, BLE communication, and firmware logic |
| Ginfull R-5B / L-5C hall-effect joysticks | Analog stick input, chosen for durability and drift resistance over potentiometer sticks |
| MAX17048 | Battery fuel gauge (I²C `0x36`) — provides accurate state-of-charge readings |
| IP5306 / FM5324UI | Battery charging and 5 V power delivery |
| SSD1306 OLED 128x64 (I²C `0x3C`) | Status display and configuration menu, mounted rotated 180° |
| 1S Li-Po battery, 3.7 V 2500 mAh | Power source |
| Custom protoboard | Houses all electronics |

## Power Architecture

- The **IP5306 / FM5324UI** module charges the 1S Li-Po battery and delivers 5 V to the ESP32 DevKit's `5V` pin.
- A physical **on/off switch** in the 5 V line, between the power module and the ESP32, disconnects the system power completely. The module's `K` (KEY) pin is **not used**: it was tested and did not provide the expected power control behavior in this hardware implementation.
- The power module switches its 5 V output off by itself when the load current stays below its minimum (about 45–50 mA) for about 30 seconds. While the controller is on, the ESP32 with its Bluetooth radio draws enough to stay above that threshold, so the firmware doesn't use light sleep or CPU down-clocking. The firmware uses this behavior for its **automatic power-off**: the ESP32 enters deep sleep, the module cuts the 5 V output about 30 seconds later, and the on/off switch (off, then on) starts the controller again. The firmware can switch the controller off, but not on.
- The **MAX17048** fuel gauge is wired directly across the battery terminals, in parallel with the charging circuit. This placement means it reads the battery's true state without interfering with charging, and it keeps tracking the battery while the controller is switched off. Its `INT` pin is not connected.

<!-- Full details in [PowerDesign.md](../hardware/power/PowerDesign.md) and [BatteryCalibration.md](../hardware/power/BatteryCalibration.md). -->

## GPIO Considerations

GPIO36 (used for the profile button) and GPIO39 (D-pad Down) are **input-only pins with no internal pull-up**, requiring an external 10kΩ pull-up resistor to 3.3V each. This is a hard hardware constraint of the ESP32 and must be accounted for in the wiring.

- **GPIO2** (X button) is tied to the DevKit's on-board LED and needs an external 1 kΩ pull-up resistor to 3.3 V to be read reliably.
- **Boot strapping pins** — GPIO0 (A), GPIO2 (X), GPIO5 (D-pad Up), GPIO12 (R4) and GPIO15 (R2) affect the ESP32 boot process: their buttons must not be held while the controller is switched on, reset, or flashed.
- **ESP32 errata** — GPIO36 and GPIO39 can briefly read LOW when the ADC powers up. The firmware works around it (the ADC stays powered and these two pins need two LOW readings in a row).
- **Joysticks** use ADC1 (GPIO32–35), because ADC2 can't be used while Bluetooth is active. The Ginfull sticks' raw output covers the full 0–4095 range and saturates at both ends before full travel; the firmware calibrates and scales it.
- **L2 / R2** are digital buttons, not analog triggers.

Full pin assignments are documented in [`Pinout.md`](../hardware/pinout/Pinout.md).

## Assembly Notes

The ESP32 DevKit sits in female headers (socketed, not soldered). A board that isn't pushed fully into its headers can lose contact on its `5V` pin under vibration, which shows up as random power-offs and can also disturb the stick readings (a stick that no longer rests at zero). If random resets or power-offs appear, check the seating of the board first.

## Enclosure

All electronics — ESP32, protoboard, joysticks, power management, fuel gauge, and OLED are held together by the **back shell** (`green_back.stl`) and the **front shell** (`black_front.stl`), which are purely mechanical, providing only cutout windows for buttons, sticks, and the display; no components attach to them directly.

The OLED display, profile button and on/off switch are mounted on a suspended support platform.

The overall shell shape and ergonomics are inspired by the **Alpakka 1 controller by Input Labs**, adapted to this project's specific internal component layout.

See [`enclosure/`](../enclosure/) for 3D source files and exports, and [`hardware/schematics/`](../hardware/schematics/) for circuit schematics.

## Datasheets

Component datasheets are available in [`hardware/datasheets/`](../hardware/datasheets/):
- ESP32
- MAX17048
- IP5306 / FM5324UI
- SSD1306
