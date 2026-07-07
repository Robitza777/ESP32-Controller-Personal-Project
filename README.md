# ESP32-Controller-Personal-Project

A custom Bluetooth Low Energy gamepad controller built from scratch, combining hardware design, firmware development, and a fully 3D-printed enclosure. The goal of this project is a wireless gamepad that works **natively** in modern games (e.g. Fortnite) via true XInput support, with a flexible multi-profile system for gaming, desktop, and media use cases.

## Overview

This is not a modded off-the-shelf controller — every layer is custom:

- **Hardware**: ESP32-based board, hall-effect joysticks, dedicated battery management and fuel gauge, OLED status display.
- **Firmware**: Native XInput over Bluetooth LE (not generic HID/DirectInput), with a 4-profile input system and persistent remapping.
- **Enclosure**: Fully 3D-printed/designed shell, modeled in Blender and exported for printing.

## Features

- **Native XInput over BLE** — recognized directly by modern games without third-party wrapper software.
- **4 input profiles**, switchable via a dedicated profile button:
  - **GAMEPAD** — pure XInput, with configurable L4/R4 button aliases.
  - **KEYBOARD + MOUSE** — left stick as WASD, right stick as mouse movement, R2 as left click.
  - **HYBRID** — standard XInput plus L4/R4 mirrored as keyboard input.
  - **DESKTOP/MEDIA** — volume, play/pause, scroll, Alt+Tab, cursor control.
- **Live button remapping** — enter remap mode and the next button pressed is captured and saved as an alias, persisted in NVS (no menus required).
- **OLED status display** — battery level and active profile at a glance.
- **Battery monitoring** — dedicated fuel gauge for accurate state-of-charge, independent from the power management chip.
- **Power-efficient design** — light sleep support to extend battery life without tripping the power module's auto-shutoff.

## Hardware Platform

| Component | Purpose |
|---|---|
| ESP32 WROOM DevKit (38-pin) | Main microcontroller |
| FM5324UI (IP5306 clone) | Battery charging & power management |
| MAX17048 | Battery fuel gauge (state-of-charge) |
| SSD1306 0.96" OLED (I2C) | Status display |
| Ginfull R-5B / L-5C | Hall-effect joystick modules |
| Custom 3D-printed enclosure | Full physical housing |

Full schematics, pinouts, and datasheets are available in [`hardware/`](hardware/).

## Firmware

Built with [PlatformIO](https://platformio.org/) targeting the ESP32, using the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library for simultaneous native XInput and keyboard/mouse HID support over Bluetooth LE.

See [`firmware/`](firmware/) for source code and [`docs/Software.md`](docs/Software.md) for architecture details.

## Enclosure

The controller's shell is fully custom-designed and 3D-printed. Its overall shape and ergonomic layout are **inspired by the Alpakka 1 controller by Input Labs**, adapted here for this project's own hardware layout and internal component placement.

3D models (Blender source + STL/STEP exports) are available in [`enclosure/`](enclosure/).

## Repository Structure

```
ESP32-BLE-Gamepad/
├── firmware/     — PlatformIO project (src, include, lib, tests)
├── hardware/     — Schematics, pinouts, power design, BOM, datasheets
├── enclosure/    — Blender source, STL/STEP exports for 3D printing
├── docs/         — Build guide, feature docs, development log, known issues
├── images/       — Renders, prototype photos, assembly photos
├── videos/       — Demo footage
└── tools/        — Calibration and helper scripts
```

## Documentation

- [Build Guide](docs/BuildGuide.md)
- [Features](docs/Features.md)
- [Hardware](docs/Hardware.md)
- [Software](docs/Software.md)
- [Battery](docs/Battery.md)
- [Bluetooth](docs/Bluetooth.md)
- [Known Issues](docs/KnownIssues.md)
- [Future Plans](docs/FuturePlans.md)
- [Open Questions](docs/OpenQuestions.md)
- [Development Log](docs/DevelopmentLog.md)

## Project Status

This project is under active development. See [`CHANGELOG.md`](CHANGELOG.md) for released versions and [`docs/DevelopmentLog.md`](docs/DevelopmentLog.md) for ongoing progress.

## Contributing

Contributions are welcome — see [`CONTRIBUTING.md`](CONTRIBUTING.md) for guidelines on submitting issues, pull requests, and hardware/enclosure changes.

## License

See [`LICENSE`](LICENSE) for details.

## Acknowledgements

- Enclosure design inspired by the [Alpakka 1 controller](https://github.com/inputlabs/cad) by **Input Labs**.
- Built on top of the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library by Mystfit.