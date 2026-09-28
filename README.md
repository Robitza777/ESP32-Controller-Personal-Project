# ESP32-Controller-Personal-Project

A custom Bluetooth Low Energy gamepad controller built from scratch, combining hardware design, firmware development, and a fully 3D-printed enclosure. The goal of this project is a wireless gamepad that works **natively** in modern games (e.g. Fortnite) via true XInput support, with a flexible multi-profile system for gaming, desktop, and media use cases.

## Overview

This is not a modded off-the-shelf controller. Every major aspect is custom-designed:

* **Hardware** – ESP32-based electronics with hall-effect joysticks, dedicated battery management and fuel gauge, OLED status display, and a custom power system.
* **Firmware** – Native XInput over Bluetooth LE (not generic HID/DirectInput) together with keyboard, mouse, and media keys, featuring up to 16 profiles that are fully editable on the controller itself, battery monitoring, and automatic power-off.
* **Enclosure** – Fully custom 3D-printed shell, modeled in Blender and optimized around the project's hardware layout.

## Features

### Native XInput over Bluetooth LE

Unlike generic HID/DirectInput gamepads, this controller uses the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library to present itself as a native XInput device. This matters because many modern games, **Fortnite** being a notable example, do not recognize DirectInput controllers at all and previously required workarounds such as **x360ce**. With native XInput support, no additional software is needed.

The same Bluetooth connection also carries a keyboard, a 5-button mouse with vertical and horizontal scrolling, media keys, and system keys (sleep, power down). Any button can send any of them at any time, without re-pairing or switching modes. In Windows, the controller appears as **Robitza's ESP32 Gamepad**.

### Low Latency

Responsiveness is the firmware's top priority:

* All inputs are scanned at a fixed **1 kHz** on a dedicated CPU core.
* A button press registers on the very first sample (eager debounce, no added delay).
* The controller requests the shortest Bluetooth LE connection interval (**7.5 ms**) and sends a report only when something changed.
* Display, battery, and storage work runs on the other CPU core and never delays input.

### Profiles and Presets

The controller holds up to **16 profiles**, switchable at any time via the dedicated profile button. On first boot, it starts with four built-in presets:

| Profile      | Description                                                                                                                                                                  |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **GAMEPAD**  | Native XInput controller. L4 acts as View, R4 as the Xbox Guide button.                                                                                                      |
| **KB+MOUSE** | PC games without controller support (FPS layout). The left stick acts as WASD, the right stick controls the mouse, R2 / L2 are the left / right mouse buttons.               |
| **MEDIA**    | Desktop and media control: cursor, scrolling, volume, track control, play/pause, Alt+Tab, Win+D, copy and paste.                                                             |
| **HYBRID**   | Standard XInput controller, plus a second layer while R4 is held: mouse, scrolling, clicks, Enter / Esc, and media keys, to operate the PC without leaving the game profile. |

Every profile can be edited, and new ones created, directly on the controller:

* Each button can send up to **three actions at once** (e.g. Ctrl+C, LB+RB, or an Xbox button plus a key), chosen from all Xbox buttons, the full keyboard (including F13–F24 and the numpad), mouse buttons and scrolling, and media, launch, browser, and system keys.
* Button modes: **Normal**, **Toggle** (press on / press off), and **Turbo** (1–20 presses per second).
* **RTZ layer** ("Robitza-Shift"): a button can switch the whole controller to a second set of mappings while held, or lock it in Toggle mode, similar to Razer HyperShift.
* Each stick can act as the left or right Xbox stick, a mouse, a scroll wheel, or four separate directions (WASD, arrow keys, D-pad...), with its own deadzone and speed.
* Any button or stick can be disabled.

### Profile Button

A dedicated profile button (GPIO36) switches profiles without any menu:

* **Short press** → Next profile.
* **Double press** → Back to the previous profile.
* **Hold + A / B / X / Y** → Jump to profile 1 / 2 / 3 / 4.
* **Hold for 3 seconds** → Open the configuration menu.

Buttons pressed while the profile button is held are never sent to the host.

### On-Controller Configuration Menu

All settings are changed directly on the controller's OLED screen, with no app, website, or reflashing required:

* **D-pad or left stick** to navigate, **A** to select, **B** to go back, **L1 / R1** to jump through long lists.
* To edit a button, simply **press it**.
* Create profiles from a preset or a copy of another profile, rename, reorder, and delete them.
* Controller settings: display brightness and timeout, automatic power-off times, stick recalibration, and Bluetooth pairing reset.
* Changes are kept only when leaving the menu with **Save**. While the menu is open, the host receives no input.

All profiles and settings are stored in the ESP32's Non-Volatile Storage (NVS), remaining available after power loss. The complete menu is described in the [User Guide](docs/UserGuide.md).

### OLED Status Display

A 0.96" SSD1306 OLED provides quick access to important controller information, including:

* Active profile name and number
* Battery percentage and a 4-bar indicator
* Charging status
* Bluetooth connection status
* RTZ layer and Toggle indicators

After a profile switch, the new name is shown full screen for a second. The screen switches off after a configurable time without input (30 seconds by default), and any input wakes it up.

### Battery Monitoring

A dedicated MAX17048 fuel gauge, connected directly across the battery terminals, provides accurate state-of-charge readings rather than relying solely on battery voltage.

The battery level is also reported to Windows and shown next to the device in the Bluetooth settings. At 5%, the controller shows a full-screen warning; at 2%, it switches itself off to protect the Li-Po cell.

### Power Management

The controller switches itself off after a configurable time without input (5 minutes by default, set separately for connected and not connected), after a 10-second on-screen countdown that any input cancels.

Rather than fighting the automatic shutdown behavior of the IP5306 / FM5324UI power module, which cuts its output when the load current stays too low, the firmware relies on it: the ESP32 enters deep sleep, the power module switches its 5 V output off about 30 seconds later, and the controller draws no power until the on/off switch is toggled.

### Hall-Effect Joysticks

Hall-effect joystick modules eliminate potentiometer wear and significantly reduce stick drift compared to traditional analog sticks.

The firmware measures each stick's center at power-on, scales every direction separately, and maps the sticks to a true circle (0% circularity error in a gamepad tester), with a per-profile radial deadzone. The centers can be measured again from the menu at any time.

## Hardware Platform

| Component                   | Purpose                              |
| --------------------------- | ------------------------------------ |
| ESP32 WROOM DevKit (38-pin) | Main microcontroller                 |
| IP5306 / FM5324UI           | Battery charging & power management  |
| MAX17048                    | Battery fuel gauge (state-of-charge) |
| SSD1306 0.96" OLED (I²C)    | Status display                       |
| Ginfull R-5B / L-5C         | Hall-effect joystick modules         |
| Custom 3D-printed enclosure | Full physical housing                |

Full schematics, pinouts, power design, bill of materials, and datasheets are available in [`hardware/`](hardware/).

## Firmware

Built with [PlatformIO](https://platformio.org/) targeting the ESP32 and powered by the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library, allowing simultaneous native XInput and keyboard/mouse HID functionality over Bluetooth Low Energy.

The firmware (version **1.0.0**) follows a modular architecture responsible for:

* Input processing
* Profile management
* On-controller configuration menu
* Bluetooth communication
* OLED display control
* Battery monitoring
* Power management
* Persistent storage (NVS)

Two builds are available: **release** (the default, flashed on the controller) and **debug** (the same firmware plus a detailed serial log for troubleshooting).

See [`firmware/`](firmware/) for the source code and build instructions, [`docs/Software.md`](docs/Software.md) for implementation details, and [`docs/Architecture.md`](docs/Architecture.md) for how the modules work together.

## Enclosure

The controller's shell is fully custom-designed and 3D-printed. Its overall shape and ergonomic layout are inspired by the **Alpakka 1 controller** by [Input Labs](https://github.com/inputlabs/cad), while the internal layout, mounting system, and component placement are unique to this project.

Blender source files, STEP exports, and printable STL models are available in [`enclosure/`](enclosure/).

## Repository Structure

```text
ESP32-Controller-Personal-Project/
├── firmware/      — PlatformIO firmware
├── hardware/      — Schematics, pinouts, power design, BOM, datasheets
├── enclosure/     — Blender source, STEP and STL exports
├── docs/          — User guide and technical documentation
├── media/         — Photos and videos of the enclosure, firmware and hardware
└── tools/         — Input tester
```

## Documentation

* [User Guide](docs/UserGuide.md)
* [Build Guide](docs/BuildGuide.md)
* [Hardware](docs/Hardware.md)
* [Software](docs/Software.md)
* [Architecture](docs/Architecture.md)
* [Development Log](docs/DevelopmentLog.md)
* [Future Plans](docs/FuturePlans.md)

## Project Status

Firmware **1.0.0** is complete and tested on the real hardware: all inputs, the four presets, the configuration menu, Bluetooth reconnection, battery monitoring, and automatic power-off. The project remains under active development.

See:

* [`CHANGELOG.md`](CHANGELOG.md) for released milestones.
* [`docs/DevelopmentLog.md`](docs/DevelopmentLog.md) for ongoing development progress.
* [`docs/FuturePlans.md`](docs/FuturePlans.md) for ideas being considered.

## Contributing

Contributions, suggestions, and bug reports are welcome.

Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before opening issues or pull requests.

## License

See [`LICENSE`](LICENSE) for licensing information.

## Acknowledgements

* Enclosure design inspired by the [Alpakka 1 controller](https://github.com/inputlabs/cad) by **Input Labs**.
* Built on top of the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library by **Mystfit**, included in [`firmware/lib/`](firmware/lib/) with local changes.
* Bluetooth LE stack: [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) by **h2zero**.
* OLED graphics: [U8g2](https://github.com/olikraus/u8g2) by **olikraus**.
* Fuel gauge driver: [SparkFun MAX1704x Fuel Gauge Arduino Library](https://github.com/sparkfun/SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library) by **SparkFun Electronics**.
