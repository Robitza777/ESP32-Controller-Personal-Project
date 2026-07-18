# Features

## Native XInput over Bluetooth LE

Unlike generic HID/DirectInput gamepads, this controller uses the [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) library to present itself as a native XInput device. This matters because many modern games — Fortnite being a notable example — do not recognize DirectInput controllers at all, and previously required workarounds like x360ce to function. With native XInput support, no such workaround is needed.

## Four Input Profiles

The controller supports four distinct profiles, switchable at any time via the profile button:

| Profile | Description |
|---|---|
| **GAMEPAD** | Native XInput. L4/R4 act as remappable aliases to other XInput buttons. |
| **KEYBOARD + MOUSE** | All buttons map to keyboard keys. Left stick acts as WASD (threshold-based). Right stick acts as relative mouse movement. R2 acts as left click. |
| **HYBRID** | Same as GAMEPAD, but L4/R4 simultaneously send keyboard keystrokes alongside the XInput report — useful for games that mix controller and keyboard shortcuts. |
| **DESKTOP/MEDIA** | Volume control, play/pause, scroll, Alt+Tab, and cursor control for general desktop use. |

## Profile Button & Remapping

A dedicated profile button (wired to GPIO36) drives a small state machine:

- **Hold ≥0.5s** — enters configuration/armed state.
- **D-pad Up/Down/Left/Right** (while armed) — selects a profile.
- **L4 or R4 press** (while armed) — enters remap mode for that paddle.
- **Options button** — toggles BLE pairing/reset.
- **Short re-press of profile button** — cancels explicitly.
- **Auto-timeout (3–5s)** — exits configuration state automatically if there's no input.

### L4/R4 Remapping

XInput has no native paddle slots — L4 and R4 are implemented as configurable aliases to existing XInput buttons. Once in remap mode, the **next physical button pressed** is automatically captured and saved as the new alias — no menu or list navigation required. Mappings persist across reboots via NVS storage.

## OLED Status Display

A small SSD1306 OLED display shows the current battery level and active profile at a glance, so you always know your controller's status without needing to check a paired device.

## Battery Monitoring

A dedicated MAX17048 fuel gauge — wired directly across the battery terminals — provides accurate state-of-charge readings, rather than relying on raw voltage alone.

<!-- ## Power Management

The controller uses **light sleep** (not deep sleep) for power savings between inputs, chosen specifically to avoid the wake-trigger complications that come with deep sleep on this hardware. See [Battery.md](Battery.md) for more on power design and expected battery life. -->
