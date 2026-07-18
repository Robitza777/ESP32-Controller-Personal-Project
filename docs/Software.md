# Software / Firmware Architecture

## Toolchain

Firmware is developed using **PlatformIO** in VS Code — not the classic Arduino IDE. PlatformIO's real folder structure (`src/`, `include/`, `lib/`) maps naturally onto this project's modular design and works cleanly with Git version control, unlike the Arduino IDE's tab-based, folder-less sketches.

## Core Library

The firmware is built on [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) (Mystfit fork), which enables native XInput support combined with simultaneous generic HID (keyboard/mouse) output over Bluetooth LE.

This replaces the earlier [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library (lemmingDev), which only supported DirectInput/generic HID gamepad output and required third-party workarounds like x360ce for compatibility with XInput-only games.

## Module Structure

Each firmware module follows a consistent `.h` / `.cpp` pairing convention:
- `include/ModuleName.h` — declarations and function signatures only.
- `src/ModuleName.cpp` — full implementation.

The one exception is `main.cpp`, which has no `.h` counterpart, since it acts as the central orchestrator tying all other modules together.

### Planned Modules (implementation sequence)

1. **`Config.cpp`** — shared constants, pin definitions, and configuration values.
2. **`Storage.cpp`** — NVS-based persistence for profiles and button remaps.
3. **`InputReader.cpp`** — reads joysticks and buttons; fixes legacy bugs (see below).
4. **`BleOutput.cpp`** — thin wrapper over ESP32-BLE-CompositeHID for sending XInput/HID reports.
5. **`Profiles.cpp`** + **`ProfileButton.cpp`** — profile state machine and switching logic.
6. **`BatteryMonitor.cpp`** + **`Display.cpp`** — battery reading and OLED status rendering.
<!-- 7. **`PowerManager.cpp`** — light sleep management. -->

## Profile State Machine

Handled by `ProfileButton.cpp`, driven by the profile button on GPIO36:

- Hold ≥0.5s alone → enter configuration/armed state.
- D-pad Up/Down/Left/Right → select profile.
- L4 or R4 press (while armed) → enter remap mode for that paddle.
- Options button → toggle BLE pairing/reset.
- Short re-press of profile button → cancel explicitly.
- Auto-timeout (3–5s) → exit configuration state if no input.

L4/R4 remapping works via direct press-to-bind: after entering remap mode, the next physical button pressed is captured and saved as the new alias in NVS — no menu navigation involved.

## Persistence

All profile selections and button remaps are stored using the ESP32's **NVS (Non-Volatile Storage) / Preferences API**, so settings survive power cycles.

## Known Bugs Being Fixed (from the legacy [0.1.0] firmware)

The original lemmingDev-based firmware had several issues being addressed in the `InputReader.cpp` rewrite:

- Incorrect joystick axis mapping (0 → 32767 instead of the correct signed range, -32767 → 32767).
- Deadzone value declared but never actually applied.
- Blocking `delay()` calls in the main loop.
- No button debounce.
- Insufficient error handling for a missing/disconnected battery sensor.

<!-- See [KnownIssues.md](KnownIssues.md) for the current status of these fixes. -->

<!-- ## Power Management

The `PowerManager.cpp` module uses **light sleep**, deliberately avoiding deep sleep to prevent wake-trigger complications with this hardware setup. See [Battery.md](Battery.md) for details. -->
