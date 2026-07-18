# GPIO Pinout

This document describes the GPIO assignments used by the ESP32-BLE-Gamepad project.

---

## Digital Inputs

| Function | GPIO | Type | Notes |
|----------|------|------|------|
| A Button (Down) | GPIO23 | Digital Input | `INPUT_PULLUP` |
| B Button (Right) | GPIO3 | Digital Input | UART0 RX. Safe for button usage. |
| X Button (Left)| GPIO0 | Digital Input | Boot-sensitive. Do **not** hold during flashing. |
| Y Button (Up) | GPIO19 | Digital Input | `INPUT_PULLUP` |
| R1 (Right Shoulder) (Up) | GPIO16 | Digital Input | `INPUT_PULLUP` |
| R2 (Right Trigger) (Down) | GPIO15 | Digital Input | Boot strapping pin. Do not hold during reset. |
| L1 (Left Shoulder) (Up) | GPIO18 | Digital Input | `INPUT_PULLUP` |
| L2 (Left Trigger) (Down) | GPIO17 | Digital Input | `INPUT_PULLUP` |
| Options (Middle) | GPIO27 | Digital Input | Moved from GPIO12 to avoid boot issues. Used together with the Profile Button to toggle BLE mode. |
| L3 (Left Joystick Click) | GPIO26 | Digital Input | `INPUT_PULLUP` |
| R3 (Right Joystick Click) | GPIO25 | Digital Input | `INPUT_PULLUP` |
| D-Pad Up | GPIO5 | Digital Input | Used for profile selection. Avoid holding during reset. |
| D-Pad Down | GPIO4 | Digital Input | Used for profile selection. |
| D-Pad Left | GPIO2 | Digital Input | Boot-sensitive. Avoid holding during reset. |
| D-Pad Right | GPIO13 | Digital Input | Used for profile selection. |
| L4 (Back Paddle) | GPIO14 | Digital Input | Used to enter L4 remap mode. |
| R4 (Back Paddle) | GPIO12 | Digital Input | Boot-sensitive. Used to enter R4 remap mode. |

---

## Analog Inputs

| Function | GPIO | ADC Channel |
|----------|------|-------------|
| Left Stick X (Black) | GPIO34 | ADC1_CH6 |
| Left Stick Y (Orange) | GPIO35 | ADC1_CH7 |
| Right Stick X (Black) | GPIO32 | ADC1_CH4 |
| Right Stick Y (Orange) | GPIO33 | ADC1_CH5 |

---

## I²C Devices

Both devices share the same I²C bus.

| Device | SDA | SCL | Notes |
|---------|-----|-----|------|
| MAX17048 Fuel Gauge | GPIO21 | GPIO22 | Requires 10 kΩ pull-up resistors to 3.3V if not included in the boards (in my case i didn't need external pull-up) |
| SSD1306 OLED Display | GPIO21 | GPIO22 | Shares the bus with MAX17048. Verify I²C addresses if additional devices are added. |

---

## Special Inputs

| Function | GPIO | Notes |
|----------|------|------|
| Profile Button | GPIO36 | Input only. Requires an external 10 kΩ pull-up resistor to 3.3V. |
| Reserved | GPIO39 | Input only. Available for future expansion. |
| Reserved | GPIO1 | UART0 TX. Can be used as GPIO if Serial debugging is not required. |

---

## Optional Connections

| Function | Recommendation |
|----------|---------------|
| MAX17048 INT | Leave unconnected unless interrupt functionality is required. GPIO39 is the preferred choice if available. |

---

# Boot-Sensitive Pins

The following GPIOs affect the ESP32 boot process and **must not be held LOW during reset or upload**:

- GPIO0
- GPIO2
- GPIO12
- GPIO15

These pins are safe to use for buttons as long as the user is not pressing them while the board is powering up or resetting.

---

# I²C Bus

```
GPIO21 (SDA)
        │
        ├── MAX17048
        └── SSD1306 OLED

GPIO22 (SCL)
        │
        ├── MAX17048
        └── SSD1306 OLED
```

---

# Summary

| Category | Count |
|----------|------:|
| Digital buttons | 18 |
| Analog axes | 4 |
| I²C devices | 2 |
| Reserved GPIOs | 2 |
| Optional interrupt lines | 1 |