# Software / Firmware Architecture

## Toolchain

Firmware is developed using **PlatformIO** in VS Code — not the classic Arduino IDE. PlatformIO's real folder structure (`src/`, `include/`, `lib/`) maps naturally onto this project's modular design and works cleanly with Git version control, unlike the Arduino IDE's tab-based, folder-less sketches.

## Core Library

The firmware is built on [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) (Mystfit fork), which enables native XInput support combined with simultaneous generic HID (keyboard/mouse) output over Bluetooth LE.

This replaces the earlier [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library (lemmingDev), which only supported DirectInput/generic HID gamepad output and required third-party workarounds like **x360ce** for compatibility with XInput-only games.

---

## Bluetooth / BLE Architecture

### Why Native XInput Matters

Many modern games do not recognize generic HID/DirectInput gamepads at all. **Fortnite** is a well-known example — it requires XInput-compatible input and will not detect a DirectInput controller natively. Previously, DirectInput controllers needed a workaround like **x360ce** to translate their input into an XInput-compatible signal.

This project avoids that entirely by presenting the controller as a **native XInput device** over Bluetooth LE.

### Library

BLE communication and HID/XInput report generation is handled by [**ESP32-BLE-CompositeHID**](https://github.com/Mystfit/ESP32-BLE-CompositeHID) (Mystfit fork). This library supports composite HID descriptors, allowing the device to expose:

* A native XInput gamepad interface, **and**
* A generic keyboard/mouse HID interface

simultaneously, which is what makes the **KEYBOARD + MOUSE**, **HYBRID**, and **DESKTOP / MEDIA** profiles possible without re-pairing or switching BLE modes.

### Migration from ESP32-BLE-Gamepad

The original firmware (`[0.1.0]`) was based on [**ESP32-BLE-Gamepad**](https://github.com/lemmingDev/ESP32-BLE-Gamepad) by lemmingDev, which only exposed a generic HID/DirectInput gamepad. This was a foundational limitation, since it could not be recognized natively by XInput-only games — hence the architectural migration to [**ESP32-BLE-CompositeHID**](https://github.com/Mystfit/ESP32-BLE-CompositeHID).

### Pairing & Reset

BLE pairing can be toggled/reset via the **Options button**, accessible through the profile button's configuration state (see **Profile State Machine** below).

### Report Handling by Profile

| Profile              | BLE Report Type(s) Sent                                 |
| -------------------- | ------------------------------------------------------- |
| **GAMEPAD**          | XInput gamepad report only                              |
| **KEYBOARD + MOUSE** | Keyboard + mouse HID reports only                       |
| **HYBRID**           | XInput gamepad report + keyboard HID report (for L4/R4) |
| **DESKTOP / MEDIA**  | Keyboard/media-key HID reports                          |

### Known Limitations / Open Items

BLE connection stability, reconnection behavior, and latency have not yet been fully characterized on real hardware. Any findings or issues should be tracked as appropriate.

---

## Module Structure

Each firmware module follows a consistent `.h` / `.cpp` pairing convention:

* `include/ModuleName.h` — declarations and function signatures only.
* `src/ModuleName.cpp` — full implementation.

The one exception is `main.cpp`, which has no `.h` counterpart, since it acts as the central orchestrator tying all other modules together.

### Planned Modules (implementation sequence)

1. **`Config.cpp`** — shared constants, pin definitions, and configuration values.
2. **`Storage.cpp`** — NVS-based persistence for profiles and button remaps.
3. **`InputReader.cpp`** — reads joysticks and buttons; fixes legacy bugs (see below).
4. **`BleOutput.cpp`** — thin wrapper over ESP32-BLE-CompositeHID for sending XInput/HID reports.
5. **`Profiles.cpp`** + **`ProfileButton.cpp`** — profile state machine and switching logic.
6. **`BatteryMonitor.cpp`** + **`Display.cpp`** — battery reading and OLED status rendering.

<!-- 7. **`PowerManager.cpp`** — light sleep management. -->

---

## Profile State Machine

Handled by `ProfileButton.cpp`, driven by the profile button on GPIO36:

* Hold ≥0.5s alone → enter configuration/armed state.
* D-pad Up/Down/Left/Right → select profile.
* L4 or R4 press (while armed) → enter remap mode for that paddle.
* Options button → toggle BLE pairing/reset.
* Short re-press of profile button → cancel explicitly.
* Auto-timeout (3–5s) → exit configuration state if no input.

L4/R4 remapping works via direct press-to-bind: after entering remap mode, the next physical button pressed is captured and saved as the new alias in NVS — no menu navigation involved.

---

## Persistence

All profile selections and button remaps are stored using the ESP32's **NVS (Non-Volatile Storage) / Preferences API**, so settings survive power cycles.

---

## Known Bugs Being Fixed (from the legacy [0.1.0] firmware)

The original lemmingDev-based firmware had several issues being addressed in the `InputReader.cpp` rewrite:

* Incorrect joystick axis mapping (0 → 32767 instead of the correct signed range, -32767 → 32767).
* Deadzone value declared but never actually applied.
* Blocking `delay()` calls in the main loop.
* No button debounce.
* Insufficient error handling for a missing/disconnected battery sensor.

<!-- See [KnownIssues.md](KnownIssues.md) for the current status of these fixes. -->

<!-- ## Power Management

The `PowerManager.cpp` module uses **light sleep**, deliberately avoiding deep sleep to prevent wake-trigger complications with this hardware setup. See [Hardware.md](Hardware.md) for details on the project's power architecture and battery system.
-->
