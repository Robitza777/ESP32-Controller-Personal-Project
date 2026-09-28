# Vendored ESP32-BLE-CompositeHID

Source: https://github.com/Mystfit/ESP32-BLE-CompositeHID at commit `032b0645474584d3dc8178ecebdcbbbcdbe1130b`
(MIT license, see `license.txt`). `examples/`, `scripts/` and `notes/` were left out.

The next upstream commit (`06d93ea`, DualSense Edge) uses IDF 5-only APIs and doesn't build on Arduino core 2.0.x.

## Local changes

Every change is marked with a `Local change` comment.

| File | Change | Why |
|---|---|---|
| `KeyboardDescriptors.h` | Keyboard key array accepts usages 0x00–0xE7 (was 0x00–0x65) | F13–F24, international and language keys |
| `KeyboardDescriptors.h` | Consumer control is an array of `MEDIA_KEY_SLOTS` 16-bit usages 0x000–0x3FF (was a fixed 24-key bitmask) | any media / launch / browser / brightness key; 25 bytes instead of 78 |
| `KeyboardDescriptors.h` | New System Control collection, report ID `0x44` (power down, sleep, wake up) | system keys |
| `KeyboardHIDCodes.h` | Removed the `KEY_MEDIA_*` bitmask constants, added `MEDIA_KEY_SLOTS` | the bitmask no longer exists |
| `KeyboardConfiguration.*` | `setUseSystemKeys()` | enables the System Control collection |
| `KeyboardDevice.*` | `setMediaKeys()` / `setSystemKeys()` + `sendSystemKeyReport()` replace `mediaKeyPress()` / `mediaKeyRelease()`; media and system characteristics are only created when enabled | matches the new descriptors |
| `BleCompositeHID.cpp` | Scan response enabled before `setName()` | names longer than ~7 characters don't fit the advertising packet |
| `BleConnectionStatus.cpp` | Connected only once the link is encrypted (NimBLE also reports failed encryption as "authentication complete") | the host's HID driver can't use an unencrypted link |
| `BleConnectionStatus.cpp`, `BleCompositeHID.cpp`, `DiagLog.h` | Logs stored bonds at boot, connect / encryption / disconnect (with reason) | diagnosing hosts that can't reconnect after a restart |
| `library.json` | Removed the example list, version `0.3.1-robitza.1` | examples aren't vendored |

The combined HID report map must stay ≤ 512 bytes (GATT attribute limit); the library only checks each device on its
own. `BleOutput::begin()` logs the total.
