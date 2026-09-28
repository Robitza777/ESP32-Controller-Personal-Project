# System Architecture

## Overview

The ESP32 Controller firmware is structured as a modular embedded system where hardware input, profile logic, persistent configuration, BLE communication, power monitoring, and user feedback are separated into independent components.

The architecture is designed around a central control flow, split across the ESP32's two CPU cores:

```
CORE 1 — input task, every 1 ms
-------------------------------
Hardware Inputs
      |
      v
InputReader
      |
      v
ProfileButton / Profiles
      |
      +----------------------------+
      |                            |
      v                            | status snapshot
BleOutput                          | (profile, layer, last activity)
      |                            |
      v                            |
Bluetooth LE HID                   |
                                   |
CORE 0 — UI task, every 20 ms      |
-----------------------------      |
main.cpp (UI loop) <---------------+
      |
      +----> PowerManager
      |
      +----> ConfigMenu ----> Storage ----> ESP32 NVS
      |
      +----> Display -------> SSD1306 OLED
      |
      +----> BatteryMonitor <---- MAX17048 <---- Battery
```

The main firmware entry point (`main.cpp`) acts as the orchestrator, initializing modules and coordinating communication between them.

---

# Task Layout

The firmware runs two FreeRTOS tasks, one per CPU core. Responsiveness comes first: nothing slow is allowed on the path from a button to the Bluetooth report.

| Task | Core | Priority | Period | Work |
|---|---|---|---|---|
| **input** | 1 | high (10) | 1 ms, fixed (`vTaskDelayUntil`) | `InputReader` → `ProfileButton` / `Profiles` → `BleOutput` |
| **UI** | 0 | low (1) | 20 ms | `Display`, `BatteryMonitor`, `PowerManager`, `ConfigMenu`, `Storage` writes |

* The NimBLE Bluetooth host also runs on core 0, above the UI task, so slow UI work never delays Bluetooth either.
* A full OLED frame takes about 23 ms of I²C traffic, and a flash write can stall both cores; both happen only in the UI task, and the display redraws only when something shown has changed.
* The UI task is the **only** user of the I²C bus, so the bus needs no locking.
* The input path never blocks: no `delay()`, no I²C, no flash writes, no serial output.
* The input task publishes a small status snapshot (active profile, layer and toggle state, time of the last activity) every cycle; the UI task reads it every cycle. Both copy a few bytes under a spinlock, so neither side waits noticeably.

Measured input cycle: about 170 µs with nothing to send, about 0.5 ms when a report goes out, rarely up to about 1 ms.

---

# Firmware Layers

## Application Layer

### main.cpp

`main.cpp` is responsible for:

* Loading the saved profiles and settings, initializing all firmware modules, and starting the two tasks.
* Running the input loop and the UI loop.
* Coordinating input processing, profile handling, BLE output, and display updates.
* Handing the profile list over to the configuration menu and back (see below).
* Running the power-off sequence.

Individual modules should not directly depend on each other where possible. Communication should happen through clearly defined interfaces: modules exchange plain data structures (`InputState`, `OutputReport`, `BatteryStatus`, `Profile`...) and `main.cpp` passes them along.

---

# Input Layer

## InputReader

The `InputReader` module handles all physical controller inputs.

Responsibilities:

* Reading digital button states (both GPIO registers in one go).
* Reading analog joystick values (ADC1 through the IDF low-level HAL, oversampled).
* Applying joystick calibration, circular mapping, and deadzone processing.
* Eager debouncing, and the ESP32 GPIO36/39 errata workaround.
* Converting raw hardware values into normalized controller data.

Input flow:

```
Buttons
Joysticks
   |
   v
InputReader
   |
   v
InputState (18 buttons as a bitmask, 4 axes in -32767..32767, +X right, +Y up)
```

The module abstracts the physical GPIO layout from the rest of the firmware.

---

# Profile System

## Profiles

The profile system controls how controller inputs are interpreted.

Built-in presets (read-only templates in flash):

* GAMEPAD
* KB+MOUSE
* MEDIA
* HYBRID

The user's profile list (up to 16) starts as one profile per preset and is then edited in the configuration menu. The profile layer receives normalized controller input and decides how it should be translated, through the active profile's layer (base or RTZ).

Example:

```
Joystick Movement
        |
        v
     Profiles
        |
        +------> XInput Axis
        |
        +------> Mouse Movement / Scroll
        |
        +------> Keyboard Keys (4-way mode)
```

The result is one `OutputReport` per input cycle: the complete state the host should see (Xbox controller, keyboard, consumer keys, system keys, mouse).

---

# Profile Button State Machine

## ProfileButton

The profile button provides access to profile switching and configuration functions without requiring external software.

State flow:

```
Normal Operation

      |
      +---- Short press ----------------> Next profile
      |
      +---- Double press ---------------> Previous profile
      |
      +---- Hold + A / B / X / Y -------> Profile 1 / 2 / 3 / 4
      |
      | Hold 3 s alone
      v

Configuration Menu

      |
      | Hold 3 s again → Save / Discard / Keep editing
      v

Normal Operation
```

Buttons pressed while the profile button is held are kept from the host until they are released.

---

# Configuration Menu

## ConfigMenu, MenuView, ActionCatalog

The configuration menu runs in the UI task:

* `ConfigMenu` holds the menu logic: pages, cursor, the edits.
* `MenuView` is a text-only description of one screen (title, rows, values, cursor). `ConfigMenu` fills it, `Display` draws it: the menu logic never deals with fonts or pixels.
* `ActionCatalog` holds the named, grouped action lists the menu offers (None, RTZ layer, Gamepad, Keyboard, Mouse, Media & system).

While the menu is open, the input task sends the host a neutral report and forwards the buttons and the left stick to the menu. The profile list is handed over between the two tasks with a small state machine, so that only one task works on it at a time:

```
Closed
   |
   | Profile held 3 s: the input task copies the profiles into the menu's workspace
   v
Open  ---- the UI task edits the workspace; the host receives a neutral report
   |
   +---- Save ------> UI task writes the changes to NVS
   |                        |
   |                        v
   |                  Saved: the input task takes the new list into Profiles
   |
   +---- Discard ---> Discarded: the input task keeps its profiles
   |
   v
Closed
```

The workspace also carries the display and power settings; display changes are previewed live while the menu is open.

---

# Storage Layer

## Storage

The storage module provides persistent configuration using ESP32 NVS (Non-Volatile Storage).

Stored data includes:

* The profile list (up to 16 profiles, with a format version).
* Active profile selection.
* Display settings (brightness, screen timeout, name popup).
* Automatic power-off times.

Storage flow:

```
Configuration Menu (Save)
Profile switch (after 3 s)
        |
        v
Storage Module (UI task only)
        |
        v
ESP32 NVS
```

Settings remain available after power loss or restart. The Bluetooth pairings are stored separately by NimBLE.

---

# Communication Layer

## BleOutput

The `BleOutput` module acts as an abstraction layer over the BLE HID implementation.

Responsibilities:

* Sending XInput reports.
* Sending keyboard, consumer (media) and system key reports.
* Sending mouse reports.
* Reporting the battery level to the host.
* Erasing the pairings, and closing the connection before a power-off.

Flow:

```
OutputReport
        |
        v
   BleOutput (a report per device, only when it changed)
        |
        v
ESP32-BLE-CompositeHID
        |
        v
NimBLE ----> Bluetooth LE Device (Xbox controller + keyboard + mouse)
```

The system supports simultaneous controller and HID functionality through the ESP32-BLE-CompositeHID library.

---

# Display Layer

## Display

The OLED display provides user feedback.

Displayed information includes:

* Active profile (name and number), with a full-screen popup after a switch.
* Battery status (percentage, bars, charging).
* Bluetooth connection status.
* RTZ layer and Toggle indicators.
* The configuration menu.
* Full-screen alerts (power-off countdown, battery warnings).

Flow:

```
Status snapshot + BatteryStatus    MenuView    Power alert
             |                        |             |
             v                        v             v
                         Display Module
                               |
                               v
                          SSD1306 OLED
```

The display communicates through the shared I²C bus. It is mounted rotated 180°, redraws only when something shown has changed, and switches off after the configured time without activity.

---

# Battery Monitoring Layer

## BatteryMonitor

The battery monitoring system uses the MAX17048 fuel gauge.

Responsibilities:

* Reading battery state-of-charge every 5 seconds.
* Detecting charging from the gauge's charge rate.
* Providing battery information to the display, the power manager, and the host.
* Handling battery monitoring independently from the charging/power management circuit.

Flow:

```
Battery
   |
   v
MAX17048
   |
   v
BatteryMonitor
   |
   +----> Display
   |
   +----> PowerManager
   |
   +----> BleOutput (Battery Service)
```

---

# Hardware Communication

## I²C Bus

The controller uses a shared I²C bus at 400 kHz:

```
ESP32
 |
 +---- SDA GPIO21
 |
 +---- SCL GPIO22
        |
        +---- SSD1306 OLED (0x3C)
        |
        +---- MAX17048 (0x36)
```

Both devices operate on different I²C addresses and can coexist on the same bus. Only the UI task uses the bus.

---

# Power Management

## PowerManager

The power management module decides when to warn the user and when to power off:

* Automatic power-off after the configured time without activity, with a 10-second countdown.
* Battery critical warning at 5%.
* Automatic power-off at 2% to protect the battery.

Power-off sequence (`main.cpp`):

```
PowerManager: power off
        |
        v
Input task stops sending
        |
        v
OLED off, Bluetooth connection closed
        |
        v
ESP32 deep sleep (no wake-up source)
        |
        v
Load below the power module's threshold ----> 5 V output switched off (~30 s later)
        |
        v
Controller off until the on/off switch is toggled
```

The firmware can switch the controller off, but not on. Light sleep is not used while the controller is on, because the lower current draw would make the power module cut the power in the middle of use.

---

# Design Principles

The firmware architecture follows these principles:

* **Responsiveness first** — nothing on the path from a button to the Bluetooth report may block or wait for slow work.
* **Modularity** — each hardware/software function has its own module.
* **Separation of concerns** — input processing, output generation, storage, and display logic remain independent.
* **Hardware abstraction** — physical GPIO assignments should not leak into higher-level logic; every pin, address, and timing lives in `Config.h`.
* **Graceful degradation** — the controller keeps working if the display or the fuel gauge is missing.
* **Maintainability** — new profiles, mappings, and features should be added without rewriting core functionality.
* **Expandability** — the architecture supports future features such as a companion configuration tool, macros, and OTA updates.
