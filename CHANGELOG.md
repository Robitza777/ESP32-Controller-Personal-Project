# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Migration to **ESP32-BLE-CompositeHID** (Mystfit fork) for native XInput support combined with simultaneous keyboard/mouse HID.
- **4-profile input system**, managed via a dedicated profile button on GPIO36:
  - **GAMEPAD** — pure XInput, with L4/R4 as configurable aliases to other XInput buttons.
  - **KEYBOARD + MOUSE** — all buttons as keypresses; left stick as WASD; right stick as relative mouse movement; R2 as left click.
  - **HYBRID** — standard XInput plus L4/R4 mirrored as parallel keyboard input.
  - **DESKTOP/MEDIA** — volume, play/pause, scroll, Alt+Tab, cursor control.
- Profile button state machine: hold to arm, D-pad to select profile, L4/R4 to enter remap mode, Options to reset BLE pairing, timeout/re-press to cancel.
- Live L4/R4 alias remapping — next button pressed after entering remap mode is captured and saved as the new alias, persisted in NVS.
- OLED status display (SSD1306, I2C) showing battery level and active profile.
- Dedicated battery fuel gauge (MAX17048) for accurate state-of-charge, independent of the power management chip.
- Light sleep power management to extend battery life without triggering the power module's auto-shutoff.
- Full modular firmware restructuring (separate modules for input, profiles, BLE output, display, battery, power, and storage).

### Changed
- Project migrated from Arduino IDE to **PlatformIO** for proper modular folder structure and version control.

### Fixed
- Joystick axis mapping corrected to signed range (-32767 to 32767) instead of unsigned (0 to 32767).
- Deadzone logic now actually applied (previously declared but unused).
- Removed blocking `delay()` calls from the main loop.
- Added button debounce.
- Corrected pin assignments to match physical hardware (previously inconsistent in the "stable" firmware variant).
- Safer battery sensor initialization to prevent crashes when the sensor is absent.

---

## [0.1.0] - 2026-07-06

First functional version of the controller, based on the [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library by lemmingDev.

### Added
- Initial BLE gamepad functionality using ESP32-BLE-Gamepad.
- Basic joystick and button input reading.
- Basic battery voltage reading.

### Known Issues
- Uses ESP32-BLE-Gamepad (DirectInput/generic HID), not XInput — controller is not natively recognized by modern games (e.g. Fortnite) without third-party workarounds.
- Joystick axis mapping outputs unsigned range (0 to 32767) instead of signed (-32767 to 32767).
- Deadzone variable declared but never applied.
- Blocking `delay()` calls used in the main loop.
- No button debounce implemented.
- No fallback handling when the battery sensor is not detected.