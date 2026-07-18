# Bluetooth / BLE Architecture

## Why Native XInput Matters

Many modern games do not recognize generic HID/DirectInput gamepads at all. **Fortnite** is a well-known example — it requires XInput-compatible input and will not detect a DirectInput controller natively. Previously, DirectInput controllers needed a workaround like **x360ce** to translate their input into an XInput-compatible signal for such games.

This project avoids that entirely by presenting the controller as a **native XInput device** over Bluetooth LE.

## Library

BLE communication and HID/XInput report generation is handled by [**ESP32-BLE-CompositeHID**](https://github.com/Mystfit/ESP32-BLE-CompositeHID) (Mystfit fork). This library supports composite HID descriptors, allowing the device to expose:

- A native XInput gamepad interface, **and**
- A generic keyboard/mouse HID interface

simultaneously, which is what makes the KEYBOARD+MOUSE, HYBRID, and DESKTOP/MEDIA profiles possible without re-pairing or switching BLE modes.

## Migration from ESP32-BLE-Gamepad

The original firmware (`[0.1.0]`) was based on [**ESP32-BLE-Gamepad**](https://github.com/lemmingDev/ESP32-BLE-Gamepad) by lemmingDev, which only exposes a generic HID/DirectInput gamepad. This was a foundational limitation, since it could not be recognized natively by XInput-only games — hence the architectural migration to ESP32-BLE-CompositeHID.

## Pairing & Reset

BLE pairing can be toggled/reset via the **Options button**, accessible through the profile button's configuration state (see [Software.md](Software.md) for the full state machine).

## Report Handling by Profile

| Profile | BLE Report Type(s) Sent |
|---|---|
| GAMEPAD | XInput gamepad report only |
| KEYBOARD + MOUSE | Keyboard + mouse HID reports only |
| HYBRID | XInput gamepad report + keyboard HID report (for L4/R4) |
| DESKTOP/MEDIA | Keyboard/media-key HID reports |

## Known Limitations / Open Items

BLE connection stability, reconnection behavior, and latency have not yet been fully characterized on real hardware. Any findings or issues should be tracked as appropriate.
