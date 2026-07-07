# Development Log

A chronological log of design decisions, findings, and progress on the ESP32 BLE Gamepad project.

---

## 2026-07-06 — Initial Firmware Review and Redesign Decisions

- Reviewed existing firmware based on ESP32-BLE-Gamepad (lemmingDev), v0.5.4.
- Identified that the library implements DirectInput / generic HID, not XInput — root cause of games (e.g. Fortnite) not recognizing the controller natively.
- Identified additional bugs:
  - Joystick axis mapping outputs unsigned range (0 → 32767) instead of signed (-32767 → 32767).
  - Deadzone variable declared but never applied.
  - Blocking `delay()` calls present in the main loop.
  - No button debounce.
  - Unsafe battery sensor initialization (no fallback if sensor absent).
  - Pin assignments in the "stable" firmware variant inconsistent with actual physical hardware (Experimental variant + `Pins.txt` confirmed as the correct reference).
- Tagged as `[0.1.0]` in CHANGELOG — first known-working baseline, with all issues above logged as Known Issues.
- Decided to migrate to **ESP32-BLE-CompositeHID** (Mystfit fork) to get native XInput plus simultaneous keyboard/mouse HID support.
- Designed the **4-profile system**: GAMEPAD, KEYBOARD+MOUSE, HYBRID, DESKTOP/MEDIA.
- Designed the profile button state machine (GPIO36): hold ≥0.5s to arm, D-pad to select profile, L4/R4 to enter remap mode, Options to reset BLE pairing, timeout/re-press to cancel.
- Defined L4/R4 alias remapping behavior: after entering remap mode, the next physical button pressed is automatically captured and saved as the new alias in NVS — no menu navigation required.
- Confirmed GPIO36 (input-only, no internal pull-up) requires an external 10 kΩ pull-up to 3.3V for the profile button.
- Reserved GPIO39 and GPIO1 for future use.
- Defined power wiring: FM5324UI battery on BAT+/BAT−, 5V out to DevKit V5 pin, KEY pin routed through a physical on/off button to GND.
- Defined MAX17048 wiring: connected in parallel directly across battery terminals, sharing GND with the rest of the system.
- Confirmed OLED and MAX17048 share the I2C bus (SDA = GPIO21, SCL = GPIO22) with no address conflict.
- Decided on **light sleep** (not deep sleep) for power management, to avoid triggering the FM5324UI's auto-shutoff below ~50 mA draw.
- Set 4.15V as a placeholder threshold for charging detection, pending empirical calibration.
- Identified the need for a separate suspended mounting platform (above the ESP32 module) to hold the OLED and the two square buttons (on/off + profile) — to be designed.
- Noted that joystick pinouts differ between the R-5B and L-5C modules and require verification (ConsoleMods Wiki or multimeter) before final wiring.

## 2026-07-07 — Toolchain Migration and OLED Testing

- Migrated firmware project from Arduino IDE to **PlatformIO**, run via the PlatformIO IDE extension in VS Code.
- Reasoning: real folder structure (`src/`, `include/`, `lib/`) matches the desired modular code organization and works properly with Git, unlike Arduino IDE's tab-based sketches.
- Defined `.h`/`.cpp` convention: each module has a matching pair (`include/ModuleName.h` for declarations, `src/ModuleName.cpp` for implementation), except `main.cpp`, which has no header pair.
- Built a standalone PlatformIO test project (`OLED_Test_PlatformIO/`) to validate the SSD1306 128x64 display at I2C address 0x3C.
- Used `esp32dev` board target with `Adafruit GFX Library` and `Adafruit SSD1306` via `lib_deps`.
- Test sketch includes a visual heartbeat (counter + animated bar) and explicit error handling if the display doesn't respond at the expected address.
- Hit a blocker: `command 'platformio-ide.build' not found` error in VS Code after installing the PlatformIO IDE extension — known issue, typically caused by an incomplete background Core install, needing a VS Code restart, or Python interpreter conflicts (common on Windows).

## 2026-07-08 — GitHub Repository Organization and Small Progress

- Defined final repository structure: `firmware/`, `hardware/`, `enclosure/`, `docs/`, `images/`, `videos/`, `tools/`, plus top-level `README.md`, `LICENSE`, `.gitignore`, `CHANGELOG.md`, `CONTRIBUTING.md`.
- Decided all GitHub-facing content (docs, code comments, commit messages) is written in English.
- Generated `README.md` — project overview, features, hardware table, repository structure, documentation index, and acknowledgements (crediting the **Alpakka 1 controller by Input Labs** as the enclosure's design inspiration).
- Generated `CHANGELOG.md` in Keep a Changelog format, covering `[Unreleased]` (redesign work) and `[0.1.0]` (initial working baseline with Known Issues).
- Modified the front enclosure design to integrate the OLED display while maintaining clearance for internal components.
- Experimentally determined the joystick pinouts by disassembling a damaged PS4 controller and tracing the motherboard connections, providing a verified reference for wiring.
- Continued development of the suspended internal mounting platform for the OLED display, refining its positioning and structural support.
- Experimentally verified that the FM5324UI module's KEY pin does not provide the expected power control behavior in this hardware implementation.
- Updated the power system design to replace the KEY-pin-controlled power function with a dedicated inline physical on/off switch that completely disconnects system power.
