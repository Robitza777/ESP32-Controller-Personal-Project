# ESP32-Controller-Personal-Project

A custom Bluetooth Low Energy gamepad controller built from scratch, combining hardware design, firmware development, and a fully 3D-printed enclosure. The goal of this project is a wireless gamepad that works **natively** in modern games (e.g. Fortnite) via true XInput support, with a flexible multi-profile system for gaming, desktop, and media use cases.

## Overview

This is not a modded off-the-shelf controller. Every major aspect is custom-designed:

* **Hardware** – ESP32-based electronics with hall-effect joysticks, dedicated battery management and fuel gauge, OLED status display, and a custom power system.
* **Firmware** – Native XInput over Bluetooth LE (not generic HID/DirectInput), featuring multiple operating profiles, persistent remapping, battery monitoring, and power management.
* **Enclosure** – Fully custom 3D-printed shell, modeled in Blender and optimized around the project's hardware layout.

## Features

### Native XInput over Bluetooth LE

Unlike generic HID/DirectInput gamepads, this controller uses the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library to present itself as a native XInput device. This matters because many modern games, **Fortnite** being a notable example, do not recognize DirectInput controllers at all and previously required workarounds such as **x360ce**. With native XInput support, no additional software is needed.

### Four Operating Profiles

The controller supports four distinct operating profiles, switchable at any time via the dedicated profile button.

| Profile              | Description                                                                                                                                                              |
| -------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **GAMEPAD**          | Native XInput controller. L4/R4 act as configurable aliases to existing XInput buttons.                                                                                  |
| **KEYBOARD + MOUSE** | All buttons map to keyboard keys. The left stick acts as WASD (threshold-based), the right stick controls relative mouse movement, and R2 acts as the left mouse button. |
| **HYBRID**           | Standard XInput controller while L4/R4 simultaneously send configurable keyboard keystrokes alongside the gamepad report.                                                |
| **DESKTOP / MEDIA**  | Cursor movement, scrolling, volume control, media playback, Alt+Tab, and other desktop shortcuts.                                                                        |

### Profile Button & Live Remapping

A dedicated profile button (GPIO36) controls a small state machine:

* **Hold ≥0.5 seconds** → Enter configuration mode.
* **D-pad Up / Down / Left / Right** → Select one of the four operating profiles.
* **Press L4 or R4** (while in configuration mode) → Enter remap mode for that paddle.
* **Press the Options button** → Reset Bluetooth pairing information.
* **Short press the profile button** → Cancel configuration mode.
* **No activity for 3–5 seconds** → Exit configuration mode automatically.

### L4 / R4 Paddle Remapping

Since XInput has no native paddle buttons, L4 and R4 function as configurable aliases to existing XInput buttons.

Once remap mode is entered, the **next physical button pressed** is automatically captured and stored as the new alias. No menus or navigation are required, and all mappings are saved in the ESP32's Non-Volatile Storage (NVS), remaining available after power loss.

### OLED Status Display

A 0.96" SSD1306 OLED provides quick access to important controller information, including:

* Active profile
* Battery percentage
* Charging status
* Bluetooth connection status

### Battery Monitoring

A dedicated MAX17048 fuel gauge, connected directly across the battery terminals, provides accurate state-of-charge readings rather than relying solely on battery voltage.

### Power Management

The controller is designed around **ESP32 Light Sleep**, maximizing battery life while avoiding the automatic shutdown behavior exhibited by FM5324UI/IP5306-compatible power management modules under extremely low current draw.

### Hall-Effect Joysticks

Hall-effect joystick modules eliminate potentiometer wear and significantly reduce stick drift compared to traditional analog sticks.

## Hardware Platform

| Component                   | Purpose                              |
| --------------------------- | ------------------------------------ |
| ESP32 WROOM DevKit (38-pin) | Main microcontroller                 |
| FM5324UI (IP5306 clone)     | Battery charging & power management  |
| MAX17048                    | Battery fuel gauge (state-of-charge) |
| SSD1306 0.96" OLED (I²C)    | Status display                       |
| Ginfull R-5B / L-5C         | Hall-effect joystick modules         |
| Custom 3D-printed enclosure | Full physical housing                |

Full schematics, pinouts, power design, bill of materials, and datasheets are available in [`hardware/`](hardware/).

## Firmware

Built with [PlatformIO](https://platformio.org/) targeting the ESP32 and powered by the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library, allowing simultaneous native XInput and keyboard/mouse HID functionality over Bluetooth Low Energy.

The firmware follows a modular architecture responsible for:

* Input processing
* Profile management
* Bluetooth communication
* OLED display control
* Battery monitoring
* Power management
* Persistent storage (NVS)

See [`firmware/`](firmware/) for the source code and [`docs/Software.md`](docs/Software.md) for implementation details.

## Enclosure

The controller's shell is fully custom-designed and 3D-printed. Its overall shape and ergonomic layout are inspired by the **Alpakka 1 controller** by [Input Labs](https://github.com/inputlabs/cad), while the internal layout, mounting system, and component placement are unique to this project.

Blender source files, STEP exports, and printable STL models are available in [`enclosure/`](enclosure/).

## Repository Structure

```text
ESP32-Controller-Personal-Project/
├── firmware/      — PlatformIO firmware
├── hardware/      — Schematics, pinouts, power design, BOM, datasheets
├── enclosure/     — Blender source, STEP and STL exports
├── docs/          — Technical documentation
├── images/        — Renders, electronics, prototypes and assembly
├── videos/        — Demonstrations
└── tools/         — Calibration and helper scripts
```

## Documentation

* [Build Guide](docs/BuildGuide.md)
* [Hardware](docs/Hardware.md)
* [Software](docs/Software.md)
* [Architecture](docs/Architecture.md)
* [Development Log](docs/DevelopmentLog.md)
* [Future Plans](docs/FuturePlans.md)

## Project Status

This project is under active development.

See:

* [`CHANGELOG.md`](CHANGELOG.md) for released milestones.
* [`docs/DevelopmentLog.md`](docs/DevelopmentLog.md) for ongoing development progress.

## Contributing

Contributions, suggestions, and bug reports are welcome.

Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening issues or pull requests.

## License

See [`LICENSE`](LICENSE) for licensing information.

## Acknowledgements

* Enclosure design inspired by the [Alpakka 1 controller](https://github.com/inputlabs/cad) by **Input Labs**.
* Built on top of the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library by **Mystfit**.
