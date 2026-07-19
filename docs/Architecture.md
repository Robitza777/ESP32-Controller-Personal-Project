# System Architecture

## Overview

The ESP32 Controller firmware is structured as a modular embedded system where hardware input, profile logic, persistent configuration, BLE communication, power monitoring, and user feedback are separated into independent components.

The architecture is designed around a central control flow:

```
Hardware Inputs
      |
      v
InputReader
      |
      v
Profiles / Configuration Logic
      |
      +----------------+
      |                |
      v                v
BleOutput          Display
      |
      v
Bluetooth LE HID

Battery Hardware
      |
      v
BatteryMonitor
      |
      v
Display
```

The main firmware entry point (`main.cpp`) acts as the orchestrator, initializing modules and coordinating communication between them.

---

# Firmware Layers

## Application Layer

### main.cpp

`main.cpp` is responsible for:

* Initializing all firmware modules.
* Managing the main execution loop.
* Coordinating input processing, profile handling, BLE output, and display updates.

Individual modules should not directly depend on each other where possible. Communication should happen through clearly defined interfaces.

---

# Input Layer

## InputReader

The `InputReader` module handles all physical controller inputs.

Responsibilities:

* Reading digital button states.
* Reading analog joystick values.
* Applying joystick calibration and deadzone processing.
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
Controller State
```

The module abstracts the physical GPIO layout from the rest of the firmware.

---

# Profile System

## Profiles

The profile system controls how controller inputs are interpreted.

Supported profiles:

* GAMEPAD
* KEYBOARD + MOUSE
* HYBRID
* DESKTOP/MEDIA

The profile layer receives normalized controller input and decides how it should be translated.

Example:

```
Joystick Movement
        |
        v
     Profiles
        |
        +------> XInput Axis
        |
        +------> Mouse Movement
        |
        +------> Keyboard Input
```

---

# Profile Button State Machine

## ProfileButton

The profile button provides access to configuration functions without requiring external software.

State flow:

```
Normal Operation

      |
      | Hold profile button
      v

Configuration Mode

      |
      +---- D-pad ----> Profile Selection
      |
      +---- L4/R4 ----> Remap Mode
      |
      +---- Options ---> BLE Pairing Reset

      |
      | Timeout / Cancel
      v

Normal Operation
```

Remapping works by capturing the next physical button pressed and storing it as the new alias.

---

# Storage Layer

## Storage

The storage module provides persistent configuration using ESP32 NVS (Non-Volatile Storage).

Stored data includes:

* Active profile selection.
* Button remapping configuration.
* Future user-adjustable settings.

Storage flow:

```
Configuration Input
        |
        v
Storage Module
        |
        v
ESP32 NVS
```

Settings remain available after power loss or restart.

---

# Communication Layer

## BleOutput

The `BleOutput` module acts as an abstraction layer over the BLE HID implementation.

Responsibilities:

* Sending XInput reports.
* Sending keyboard reports.
* Sending mouse reports.

Flow:

```
Controller State
        |
        v
   BleOutput
        |
        v
ESP32-BLE-CompositeHID
        |
        v
Bluetooth LE Device
```

The system supports simultaneous controller and HID functionality through the ESP32-BLE-CompositeHID library.

---

# Display Layer

## Display

The OLED display provides user feedback.

Displayed information includes:

* Active profile.
* Battery status.
* System information.

Flow:

```
System State
      |
      v
 Display Module
      |
      v
 SSD1306 OLED
```

The display communicates through the shared I²C bus.

---

# Battery Monitoring Layer

## BatteryMonitor

The battery monitoring system uses the MAX17048 fuel gauge.

Responsibilities:

* Reading battery state-of-charge.
* Providing battery information to the display system.
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
   v
Display
```

---

# Hardware Communication

## I²C Bus

The controller uses a shared I²C bus:

```
ESP32
 |
 +---- SDA GPIO21
 |
 +---- SCL GPIO22
        |
        +---- SSD1306 OLED
        |
        +---- MAX17048
```

Both devices operate on different I²C addresses and can coexist on the same bus.

---

# Power Management

## PowerManager

The power management module is reserved for future implementation.

The intended purpose is:

* Managing ESP32 sleep states.
* Reducing idle power consumption.
* Extending battery life.

Current hardware limitations involving the FM5324UI power management module require further investigation before implementing automatic sleep behavior.

---

# Design Principles

The firmware architecture follows these principles:

* **Modularity** — each hardware/software function has its own module.
* **Separation of concerns** — input processing, output generation, storage, and display logic remain independent.
* **Hardware abstraction** — physical GPIO assignments should not leak into higher-level logic.
* **Maintainability** — new profiles, mappings, and features should be added without rewriting core functionality.
* **Expandability** — the architecture supports future features such as additional profiles, configuration tools, and OTA updates.
