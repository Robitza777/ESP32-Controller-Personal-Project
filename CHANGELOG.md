# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

---

## [1.0.0] - 2026-09-28

Complete rewrite of the firmware as a modular PlatformIO project. Tested on the real hardware with Windows 11.

### Added
- Migration to **ESP32-BLE-CompositeHID** (Mystfit fork) for native XInput support combined with simultaneous keyboard/mouse HID. The controller appears as an Xbox Series X controller named **Robitza's ESP32 Gamepad**.
- Keyboard (full key range, including F13–F24 and the numpad), 5-button mouse with vertical and horizontal scrolling, media / launch / browser keys, and system keys (power down, sleep, wake up), all on the same Bluetooth connection.
- Battery level reported to the host (shown in the Windows Bluetooth settings).
- **Profile system** with up to 16 profiles, starting from four built-in presets:
  - **GAMEPAD** — pure XInput; L4 = View, R4 = Guide.
  - **KB+MOUSE** — FPS layout: left stick as WASD, right stick as mouse, R2 / L2 as left / right click, the other buttons as keys.
  - **MEDIA** — mouse, scrolling, volume and track control, play/pause, Alt+Tab, Win+D, copy and paste.
  - **HYBRID** — GAMEPAD, plus a PC layer while R4 is held (mouse, scroll, clicks, Enter / Esc, media keys).
- Up to 3 simultaneous actions per button; button modes Normal, Toggle, and Turbo (1–20 Hz).
- **RTZ layer** ("Robitza-Shift"): a second set of mappings in every profile, activated by a button while held or latched in Toggle mode.
- Stick modes: Xbox left, Xbox right, mouse (with a response curve), scroll, 4-way (one action per direction), or none; per-stick deadzone and speed.
- Profile button gestures (GPIO36): short press = next profile, double press = previous profile, hold + A/B/X/Y = jump to profile 1–4, hold 3 s = configuration menu.
- **On-controller configuration menu** on the OLED: edit any button (chosen by pressing it) or stick on both layers, deadzones, turbo speed, rename; create profiles from a preset or a copy; reorder and delete profiles; controller settings. Changes are saved only on explicit Save.
- OLED status display (SSD1306, I2C): Bluetooth state, battery percentage with 4 bars and a charging indicator, active profile name and number, RTZ / TOGGLE tags, and a full-screen popup after a profile switch.
- Display settings: brightness (10 levels), screen timeout (10 s to 5 min, or never), profile name popup on/off.
- Dedicated battery fuel gauge (MAX17048) for accurate state-of-charge, independent of the power management chip.
- **Automatic power-off** after a configurable time without input (separately for connected and offline; 1–60 minutes or never, default 5 minutes), with a 10-second on-screen countdown. Uses ESP32 deep sleep, after which the power module switches the 5 V output off.
- Battery protection: full-screen warning at 5%, automatic power-off at 2%.
- Stick calibration: per-direction scaling around the center measured at power-on, circular output (0% circularity error), radial deadzone; the centers can be measured again from the menu.
- Persistent storage in NVS: profiles, the last active profile (restored at power-on), display and power settings.
- Forget Bluetooth pairings from the menu.
- Release and debug builds; the debug build adds a serial log (pairing, connection, battery, power, saves).
- `tools/InputTester.html`: a browser page showing held keys, mouse buttons, scrolling, and gamepad buttons and sticks, to test every output.
- Full modular firmware structure (separate modules for input, profiles, profile button, configuration menu, BLE output, display, battery, power, and storage).

### Changed
- Project migrated from Arduino IDE to **PlatformIO** for proper modular folder structure and version control.
- Input scanning runs at a fixed 1 kHz in its own task on a dedicated CPU core; display, battery, and storage work runs on the other core.
- Reports are sent only when the state changes, with a requested 7.5 ms Bluetooth connection interval.
- OLED library changed from Adafruit SSD1306/GFX to U8g2.
- Custom partition table with a 128 KB settings store (NVS).
- The profile button design changed from "hold 0.5 s to arm, D-pad to select, press L4/R4 to remap" to direct gestures plus a full configuration menu, where every button can be remapped.
- Power management uses deep sleep for automatic power-off instead of the planned light sleep, which would make the power module cut the power during use.
- Legacy sketches (`Code.ino`, `Code(Experimental).ino`, `Test Code/`) removed; they remain available in the Git history.

### Fixed
- Joystick axis mapping corrected to signed range (-32767 to 32767) instead of unsigned (0 to 32767).
- Deadzone logic now actually applied (previously declared but unused).
- Removed blocking `delay()` calls from the main loop.
- Added button debounce (eager: no added latency).
- Corrected pin assignments to match physical hardware (previously inconsistent in the "stable" firmware variant).
- Safer battery sensor initialization to prevent crashes when the sensor is absent; the firmware also keeps running without the display.
- Correct fuel gauge scaling: the driver is set to the MAX17048 (the library defaults to the MAX17043).
- ESP32 errata on GPIO36/39 (false presses when the ADC powers up) handled in firmware.

### Known Issues
- Right after erasing the Bluetooth pairings, the first pairing sometimes doesn't survive a restart; one or two re-pairs fix it.
- The *Editing* and *Brightness* consumer keys have not been verified on Windows yet.

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
