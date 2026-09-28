# Software / Firmware Architecture

## Toolchain

Firmware is developed using **PlatformIO** in VS Code — not the classic Arduino IDE. PlatformIO's real folder structure (`src/`, `include/`, `lib/`) maps naturally onto this project's modular design and works cleanly with Git version control, unlike the Arduino IDE's tab-based, folder-less sketches.

| Item | Version / Setting |
|---|---|
| Platform | `espressif32 @ 7.0.1` (Arduino core 2.0.17, ESP-IDF 4.4) |
| Board | `esp32dev` (ESP32-WROOM-32 DevKit) |
| Language | C++17 |
| Partition table | [`partitions.csv`](../firmware/partitions.csv): app 3 MB, NVS 128 KB |
| Libraries | Pinned to exact versions in [`platformio.ini`](../firmware/platformio.ini) |

The project has two builds of the same firmware:

* **`release`** (default) — what stays on the controller. No serial output.
* **`debug`** — adds `CONTROLLER_DEBUG`, which enables a timestamped serial log: stored Bluetooth pairings, connection and encryption results, disconnect reasons, battery readings, power decisions, and saves.

Build and flash instructions are in [`firmware/README.md`](../firmware/README.md).

## Core Library

The firmware is built on [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) (Mystfit fork), which enables native XInput support combined with simultaneous generic HID (keyboard/mouse) output over Bluetooth LE.

This replaces the earlier [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library (lemmingDev), which only supported DirectInput/generic HID gamepad output and required third-party workarounds like **x360ce** for compatibility with XInput-only games.

The library is **vendored** in [`firmware/lib/ESP32-BLE-CompositeHID/`](../firmware/lib/ESP32-BLE-CompositeHID/) at commit `032b064`, with local changes. The next upstream commit uses ESP-IDF 5-only functions and no longer builds on Arduino core 2.0.x. Every local change is marked `Local change` in the code and listed in the library's `VENDORED.md`:

* Keyboard: the full key range 0x00–0xE7 (F13–F24, international and language keys), instead of 0x00–0x65.
* Consumer control: an array of two 16-bit usages (any media, launch, browser, editing, or brightness key), instead of a fixed bitmask of 24 keys. It is also smaller: 25 bytes of report map instead of 78.
* A new System Control collection (power down, sleep, wake up).
* The device name is sent in the scan response, because it doesn't fit in the 31-byte advertising packet.
* The link counts as connected only once it is encrypted.
* Diagnostic log lines (debug build only).

Other libraries: [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) 2.5.1 (Bluetooth LE stack), [U8g2](https://github.com/olikraus/u8g2) 2.36.18 (OLED), and the [SparkFun MAX1704x Fuel Gauge](https://github.com/sparkfun/SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library) library 1.0.4.

---

## Bluetooth / BLE Architecture

### Why Native XInput Matters

Many modern games do not recognize generic HID/DirectInput gamepads at all. **Fortnite** is a well-known example — it requires XInput-compatible input and will not detect a DirectInput controller natively. Previously, DirectInput controllers needed a workaround like **x360ce** to translate their input into an XInput-compatible signal.

This project avoids that entirely by presenting the controller as a **native XInput device** over Bluetooth LE.

### Library

BLE communication and HID/XInput report generation is handled by [**ESP32-BLE-CompositeHID**](https://github.com/Mystfit/ESP32-BLE-CompositeHID) (Mystfit fork). This library supports composite HID descriptors, allowing the device to expose:

* A native XInput gamepad interface, **and**
* A generic keyboard/mouse HID interface

simultaneously, which is what makes the **KB+MOUSE**, **HYBRID**, and **MEDIA** profiles possible without re-pairing or switching BLE modes.

The controller identifies itself as an **Xbox Series X controller** (Microsoft's vendor and product IDs), so Windows binds its own XInput driver (`xinputhid`) exactly as it does for a real Xbox controller. The Xbox device must be the first device in the report map for this to work.

| Identity | Value |
|---|---|
| Device name | Robitza's ESP32 Gamepad |
| Manufacturer | Robitza Electrical Engineering |
| Appearance | Gamepad |
| Services | HID, Battery (level reported to the host), Device Information |

⚠️ The combined HID report map of all devices must stay within **512 bytes** (the size limit of one GATT attribute). The library only checks each device separately; above the limit, Windows shows a driver error (Code 10). The Xbox **One S** configuration plus keyboard, media keys and mouse needs about 546 bytes and fails. The Xbox **Series** configuration used here needs **474 bytes**. The debug build logs the total at boot.

### Migration from ESP32-BLE-Gamepad

The original firmware (`[0.1.0]`) was based on [**ESP32-BLE-Gamepad**](https://github.com/lemmingDev/ESP32-BLE-Gamepad) by lemmingDev, which only exposed a generic HID/DirectInput gamepad. This was a foundational limitation, since it could not be recognized natively by XInput-only games — hence the architectural migration to [**ESP32-BLE-CompositeHID**](https://github.com/Mystfit/ESP32-BLE-CompositeHID).

### Pairing & Reset

The controller pairs with **bonding** (the keys are stored on both sides), so the host reconnects by itself after every power-on. NimBLE keeps up to 3 bonds, stored in NVS separately from the profiles; the room for stored subscriptions was raised from 8 to 24 (`CONFIG_BT_NIMBLE_MAX_CCCDS`), because each host subscribes to 7 characteristics.

All pairings are erased with **Controller → Forget Bluetooth** in the configuration menu. The current connection is dropped at once and the controller advertises for a new pairing; the host must remove the old device and pair again.

### Report Handling

Profiles are no longer tied to a fixed report type: any button can send any action, so every profile can use every HID device at once.

| HID device | Report ID(s) | Carries |
|---|---|---|
| Xbox Series controller | 0x01–0x04 | Buttons, D-pad, two sticks (±32767), two triggers |
| Mouse | 0x20 | 5 buttons, movement, vertical and horizontal scroll |
| Keyboard | 0x40 | Modifiers + 6 keys, usages 0x00–0xE7 |
| Consumer control | 0x43 | 2 simultaneous media / launch / browser keys |
| System control | 0x44 | Power down, sleep, wake up |

* `send()` takes the complete desired state every input cycle. A device's report goes out **only when its part changed**, at most once every 3.75 ms per device (two per 7.5 ms connection event), so the newest state always wins and the link is never flooded.
* Mouse movement is accumulated as fractions of a count and sent as whole counts, so slow stick movements still move the cursor smoothly.
* Axis conventions are converted at the output: internally +Y means up; the Xbox and mouse reports use +Y = down.
* The controller requests a **7.5 ms** connection interval with no slave latency; Windows accepts it.
* While the configuration menu is open, the host receives a neutral report (nothing pressed, sticks centered).

### Known Limitations / Open Items

* Tested with **Windows 11** only.
* Right after **Forget Bluetooth** (or an Erase Flash), the first pairing sometimes doesn't survive a restart; one or two re-pairs fix it for good. The cause is unknown. The debug build logs the stored bonds at boot and the encryption result of every connection, to investigate if it comes back.
* Connecting costs one input cycle of about 22 ms (the pairing is written to flash). It happens only at connection time, outside gameplay.
* L2 / R2 are digital buttons, so the XInput triggers are either released or fully pressed.
* The Xbox Share button can be mapped, but games don't see it.
* The *Editing* (application commands such as Copy, Paste, Undo) and *Brightness* consumer keys have not been verified on Windows yet.

---

## Module Structure

Each firmware module follows a consistent `.h` / `.cpp` pairing convention:

* `include/ModuleName.h` — declarations and function signatures, plus a comment describing the module's behavior.
* `src/ModuleName.cpp` — full implementation. Private state lives in an anonymous namespace.

The one exception is `main.cpp`, which has no `.h` counterpart, since it acts as the central orchestrator tying all other modules together. Modules exchange plain data structures (`InputState`, `OutputReport`, `BatteryStatus`...) defined in `ControllerTypes.h` and `ProfileTypes.h`, and don't call each other directly.

Every pin, I²C address, timing, and threshold lives in [`Config.h`](../firmware/include/Config.h).

### Modules

| Module | Task | Responsibility |
|---|---|---|
| **`InputReader`** | input | Reads all buttons and sticks; debounce, stick calibration, circular mapping, deadzone. |
| **`ProfileButton`** | input | Profile button gestures (next, previous, jump, open menu). |
| **`Profiles`** | input | The four presets, the profile list, and the engine that turns inputs into outputs through the active profile. |
| **`BleOutput`** | input | Thin wrapper over ESP32-BLE-CompositeHID: sends XInput/HID reports, battery level, pairing reset. |
| **`ConfigMenu`** | UI | The configuration menu's logic; produces a text-only `MenuView` for the display. |
| **`ActionCatalog`** | UI | Named, grouped tables of every action the menu offers. |
| **`Display`** | UI | SSD1306 OLED: status screen, profile popup, menu, alerts, screen timeout. |
| **`BatteryMonitor`** | UI | MAX17048 fuel gauge: charge level and charging state. |
| **`PowerManager`** | UI | Decides when to show power alerts and when to power off. |
| **`Storage`** | UI | Profiles and settings in NVS. |

The two tasks are described in [`Architecture.md`](Architecture.md).

---

## Input Processing

`InputReader` runs every **1 ms** in the input task. A full scan takes about 160–180 µs.

* **Sticks** are on ADC1 (ADC2 can't be used together with Bluetooth). The Arduino `analogRead()` costs about 117 µs per read and the IDF driver about 48 µs, almost all of it driver overhead, so the firmware configures ADC1 once and then converts through the IDF low-level HAL (a few µs per conversion). Each axis is oversampled 4 times per scan.
* **Calibration:** at power-on, each stick's center is averaged over 64 samples. A center too far from mid-scale means the stick was touched, and the nominal center is used instead. Each direction (left, right, up, down) is scaled by its own span, so both ends reach full deflection even when the center isn't exactly in the middle. The centers can be measured again from the menu (**Recalibrate sticks**).
* **Circular mapping:** each axis saturates on its own, so diagonals would reach the corners of a square; the output is clamped to a circle. Measured circularity error in a gamepad tester: **0.0%** (15–19% with plain per-axis mapping).
* **Deadzone:** radial, per stick and per profile (default 5%, up to 50%). Movement is rescaled so the output starts from 0 right at the deadzone edge instead of jumping.
* **Buttons** are read in one go from the two GPIO input registers, after the stick conversions so they are as fresh as possible. **Eager debounce:** a press counts on the first sample, then that pin is ignored for 5 ms — no added latency.
* **ESP32 errata (GPIO36 / GPIO39):** these pins can glitch LOW for about 80 ns when the ADC powers up. The firmware keeps the ADC powered permanently, samples the buttons only after all conversions have finished, and requires two consecutive LOW samples on these two pins.

---

## Profile Engine

A profile holds:

* A name (up to 10 characters).
* Two layers — the base layer and the **RTZ** layer — each with a mapping for all 17 mappable buttons and both sticks.
* Per button: up to **3 actions**, fired together, and a mode (**Normal**, **Toggle**, **Turbo**).
* Per stick: a mode (**None**, **Xbox left**, **Xbox right**, **Mouse**, **Scroll**, **4-way**), a speed (1–10), and 4 direction actions for 4-way mode.
* A deadzone per stick and a turbo speed (1–20 Hz).

An action is one of: Xbox button, D-pad direction, trigger, keyboard key, consumer key, system key, mouse button, scroll step, or **RTZ layer**. The Profile button itself can't be mapped.

`Profiles::apply()` is called once per input cycle and turns the `InputState` into an `OutputReport`:

* **Layers:** a button whose action is *RTZ layer* activates the second layer while held (Toggle mode latches it). Each press keeps the layer it started on, so a layer change can't leave a key stuck.
* **Mouse:** speed follows the stick deflection through a curve (deflection²), precise near the center and fast at full deflection; sensitivity 5 is 1500 counts/s at full deflection.
* **Scroll:** 4 steps/s per sensitivity step at full deflection. A button mapped to a scroll direction scrolls once, then repeats after 300 ms while held.
* **4-way:** a direction presses at 50% deflection and releases below 35%, so it can't chatter at the threshold; diagonals press two directions.
* **Profile switch:** toggles, turbo, and layers are reset, and buttons still held from the previous profile stay silent until released, so a switch never fires the new profile's actions by itself.

---

## Profile State Machine

Handled by `ProfileButton.cpp`, driven by the profile button on GPIO36:

* Short press → next profile (after 300 ms, to tell it apart from a double press).
* Double press → back to the previously active profile.
* Hold + A / B / X / Y → jump to profile 1 / 2 / 3 / 4.
* Hold for 3 s with nothing else pressed → open the configuration menu.

Buttons pressed while the profile button is held are kept from the host until they are released.

The **configuration menu** (`ConfigMenu`) replaces the earlier L4/R4 press-to-bind design: every button and stick of every profile can be changed on the controller, and the button to edit is chosen by pressing it. It runs in the UI task and edits a copy of the profile list and of the controller settings; the copy replaces the real one only when the user chooses **Save** on the way out. The menu is described in detail in the [User Guide](UserGuide.md).

---

## Persistence

All profiles and settings are stored using the ESP32's **NVS (Non-Volatile Storage) / Preferences API**, so settings survive power cycles. Everything lives in the NVS namespace `controller`:

| Key | Contents |
|---|---|
| `p0` … `p15` | One profile per slot |
| `count` | Number of saved profiles |
| `format` | Layout version of the saved profiles (`PROFILE_FORMAT_VERSION`) |
| `active` | The last active profile |
| `display` | Brightness, screen timeout, name popup |
| `power` | Automatic power-off times |

* **When it writes:** profiles and settings only when the user saves in the menu (only the changed slots, then the count); the active profile once it has stayed selected for 3 seconds. Never from the input task: a flash write briefly stalls both CPU cores.
* **Validation:** every stored value is checked when loaded; a missing or damaged value falls back to the defaults, or to the four presets for the profile list.
* **Format changes:** saved profiles carry a format version. A firmware with a different profile layout ignores them and the presets come back, unless a migration for that version is added to `Storage`.
* The Bluetooth pairings are stored by NimBLE in its own NVS namespace, which is why **Reset all profiles** keeps them.

---

## Power Management

`PowerManager` runs in the UI task. It only decides; `main.cpp` shows the alerts and runs the shutdown.

* **Automatic power-off:** after the configured time without activity (separate settings for connected and not connected; 1–60 minutes or never, default 5 minutes), with a 10-second full-screen countdown that any activity cancels. Activity means any button, or a stick past 25% deflection; at rest a stick can jitter just past its deadzone, which must not keep the controller on.
* **Battery critical:** at 5% or less (not charging), a full-screen warning for 5 seconds, repeated every minute.
* **Battery empty:** at 2% or less in 3 readings in a row (not charging), a short message, then power-off to protect the Li-Po cell.

The firmware can't disconnect the battery, and it can't switch the controller back on. It powers off by entering **deep sleep** with no wake-up source: the input task stops sending, the OLED is switched off, the Bluetooth connection is closed (so the host sees the controller leave at once), and the ESP32 sleeps. The load then falls far below the power module's minimum, and the module switches its 5 V output off about 30 seconds later. The on/off switch restarts the controller.

Light sleep and CPU down-clocking are deliberately not used while the controller is on: with less current draw, the power module would switch the controller off in the middle of use.

---

## Legacy Bugs Fixed (from the [0.1.0] firmware)

The original lemmingDev-based firmware had several issues, all fixed by the rewrite:

* Incorrect joystick axis mapping (0 → 32767 instead of the correct signed range, -32767 → 32767) — the axes are now signed and calibrated per direction.
* Deadzone value declared but never actually applied — the radial deadzone is applied to every stick.
* Blocking `delay()` calls in the main loop — no blocking calls; the input task runs on a fixed 1 ms schedule.
* No button debounce — eager debounce on every button.
* Insufficient error handling for a missing/disconnected battery sensor — the firmware keeps running without the fuel gauge or the display.
