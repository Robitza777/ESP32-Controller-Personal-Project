# GPIO Pinout

This document describes the GPIO assignments used by the ESP32-Controller-Personal-Project.

---

## Digital Inputs

| Function | GPIO | Notes |
|----------|------|-------|
| A Button (Down) | GPIO0 | Boot strapping pin (flashing mode). Do not hold during power-on or firmware flashing. |
| B Button (Right) | GPIO17 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| X Button (Left)| GPIO2 | Strapping pin tied to on-board LED. Requires an external 1 kΩ pull-up resistor to function reliably. Do not hold during power-on or firmware flashing. |
| Y Button (Up) | GPIO4 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| D-Pad Up | GPIO5 | Boot strapping pin (SDIO timing). Avoid holding during reset. |
| D-Pad Down | GPIO39 | Input-only pin (GPI). Lacks internal pull resistors; requires an external 10 kΩ pull-up resistor. |
| D-Pad Left | GPIO23 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| D-Pad Right | GPIO13 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| R1 (Right Shoulder) (Up) | GPIO16 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| R2 (Right Trigger) (Down) | GPIO15 | Boot strapping pin (ROM boot logging). Avoid holding during reset. |
| R3 (Right Joystick Click) | GPIO26 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| R4 (Back Paddle) | GPIO12 | Boot strapping pin (flash VDD voltage). Critical: do not pull HIGH at boot. |
| L1 (Left Shoulder) (Up) | GPIO18 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| L2 (Left Trigger) (Down) | GPIO19 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| L3 (Left Joystick Click) | GPIO25 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| L4 (Back Paddle) | GPIO14 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| Options (Middle) | GPIO27 | Standard digital input. Uses internal `INPUT_PULLUP`. |
| Profile Button | GPIO36 |Input-only pin (GPI). Lacks internal pull resistors; requires an external 10 kΩ pull-up resistor. |

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

## Optional Connections

| Function | Recommendation |
|----------|---------------|
| MAX17048 INT | Leave unconnected (not used in this build). No free GPIO is left for it: GPIO39 is now D-Pad Down, and GPIO1 / GPIO3 are reserved. The firmware reads the fuel gauge every 5 seconds instead. |

---

## Reserved GPIOs

| GPIO | Reason |
|------|--------|
| GPIO1 (TX0) | USB serial (CP2102): firmware upload and the Serial Monitor. Keep free. |
| GPIO3 (RX0) | USB serial (CP2102). A button on this pin interferes with the Serial Monitor. Keep free. |

---

# Boot-Sensitive Pins

The following GPIOs affect the ESP32 boot process and **must not be held LOW during reset or upload**:

- GPIO0
- GPIO2
- GPIO5
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