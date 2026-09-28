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
- Printed a test part to see the screen tolerances and placement on the suspended internal mounting platform.
- Adjusted the stl file `black_front` again after the test.
- Sanded the ESP32 mounting supports to gain more clearance.
- Fixed The screen hole going through the top cover (made me actually happy for some reason).

## 2026-07-16 — Schematic and Pinout Update

- Created the complete electrical schematic for the controller hardware.
- Added the finalized schematic to the GitHub repository documentation.
- Experimentally verified the I²C pull-up resistor configuration of the MAX17048 and SSD1306 modules, confirming that external pull-up resistors are unnecessary.
- Updated `Pinout.md` with a clearer description of the physical layout and positioning of the buttons and joysticks.
  
## 2026-07-17 — Power Management Findings and Documentation Updates

- Experimentally determined that the FM5324UI automatically disables its 5V output after approximately 30 seconds when the load current is below its minimum threshold.
- Identified this behavior as a potential blocker for implementing the planned light/deep sleep functionality, since the reduced power consumption during sleep may trigger an unintended power shutdown (further investigation needed).
- Expanded the project documentation by adding `Bluetooth.md`, `Features.md`, `FuturePlans.md`, `Hardware.md`, and `Software.md` to the `docs/` directory.
- Added `CONTRIBUTING.md` to the repository, providing guidelines for future contributions and development workflow.

## 2026-07-18 — Repository Documentation Restructuring

- Restructured the repository documentation to reduce duplication and improve maintainability.
- Merged `Features.md` into `README.md`, consolidating the project overview and feature descriptions into a single entry point.
- Merged `Bluetooth.md` into `Software.md`, keeping communication architecture and BLE implementation details together with the firmware documentation.
- Merged `Battery.md` into `Hardware.md`, consolidating power architecture and battery management information with the hardware documentation.
- Simplified the `docs/` directory structure to make project information easier to navigate and reduce maintenance overhead.
- Added `Architecture.md` to the `docs/` directory to document the overall system design, module relationships, communication flow, and firmware architecture.
- Added `Pinout.png` to the `hardware/pinout/` directory to provide a visual reference for the physical board orientation and component placement.

## 2026-09-05 — Assembly and Enclosure Adjustments

- Started soldering the components into their final positions on the circuit boards.
- Cut the ESP32 pins to allow the module to fit underneath the secondary circuit board.
- Identified an issue with the placement of the L1 and R1 switches and planned to reposition them.
- Modified `black_front` to improve the enclosure fit; further testing is required.
- Modified `green_thumbstick_x2` to provide better clearance around the electrical components.

## 2026-09-07 — Enclosure and Component Placement Adjustments

- Fixed the placement of the L1 and R1 switches.
- Fixed an issue with the R1 button clearance in `black_front`.
- Adjusted the `green_abxy_x4`, `green_dpad_x4`, `green_home`, and `green_on_off` buttons to fit the new switch placement.
- Moved the joystick holes to align with their positions on the circuit board.
- Lowered the on/off button position to improve its fit within the enclosure.
- Tested a new method for preventing overhang edges from sagging, aiming to achieve a cleaner perimeter around the screen opening.

## 2026-09-18 — Hardware Wiring and Pin Assignment Testing

- Started making the electrical connections between the hardware components.
- Swapped the GPIO assignments of the L3 and R3 buttons, resulting in L3 → GPIO25 and R3 → GPIO26.
- Tested the analog joystick axes and several buttons using dedicated test firmware to verify their functionality.
- Discovered that connecting a button to GPIO3 interferes with the Serial Monitor. Planned to move the B button to GPIO39 and add an external 10 kΩ pull-up resistor to 3.3 V.
- Considered changing the GPIO assignments of additional buttons to improve cable routing and reduce wiring complexity.
- Any further pin assignment changes will be reflected in `Pinout.md`, `Pinout.png`, and the electrical schematic.

## 2026-09-25 — Final Hardware Wiring and Pinout Updates

- Finished the electrical connections between the hardware components.
- Adjusted the wiring of several pins to improve cable routing and overall wire management (added a 10 kΩ pull-up resistor from 3.3v to the GPIO39).
- Experimentally determined that the button connected to GPIO2 was being read as permanently pressed. Added a 1 kΩ pull-up resistor to 3.3 V to resolve the issue.
- Updated `Pinout.md`, `Pinout.png`, and the electrical schematic to reflect all wiring and pin assignment changes.
- Added the first media files documenting the electrical component placement, wiring, and functionality of the test firmware.

## 2026-09-26 — Firmware Rewrite and Bluetooth Composite Test

- Added `Test_Code_Compressed.mp4` to media to hopefully allow for web viewing of the video.
- Fixed `Pinout.md` Digital Inputs Table.
- Started the 1.0.0 firmware from scratch as a modular PlatformIO project (one `.h`/`.cpp` module per function, every pin and timing in `Config.h`), with responsiveness as the top priority.
- Built a standalone test firmware for **ESP32-BLE-CompositeHID**: an Xbox controller, a keyboard, media keys, and a mouse on one Bluetooth connection. Windows bound its native Xbox driver (`xinputhid`), and all four devices worked together.
- Experimentally determined that Windows rejects the device ("driver error", Code 10) when the combined HID report map is larger than 512 bytes. The Xbox One S configuration with keyboard, media keys, and mouse needs about 546 bytes; switched to the Xbox **Series** configuration (about 495 bytes).
- Verified that the host reconnects without re-pairing after a power cycle, and that Windows accepts the shortest connection interval (7.5 ms).
- Determined the stick directions: the raw X value falls when a stick is pushed right, the raw Y value rises when it is pushed up.

## 2026-09-27 — Core Firmware Modules

- Defined the task layout: a 1 kHz input task on core 1 (buttons → profiles → Bluetooth) and a slower UI task on core 0 (display, battery, storage), so slow work never delays input.
- Implemented `InputReader`:
  - All buttons are read in one go from the GPIO registers, with eager debounce.
  - `analogRead()` was measured at about 117 µs per read; the sticks now go through the ESP-IDF low-level ADC functions (a full scan takes about 160–180 µs).
  - Per-direction stick calibration and circular mapping brought the gamepad tester's circularity error from 15–19% down to **0.0%**.
  - Added a workaround for the ESP32 errata on GPIO36/39 (false LOW readings when the ADC powers up).
- Vendored ESP32-BLE-CompositeHID into `firmware/lib/` (the next upstream version needs ESP-IDF 5) and extended it: the full keyboard range including F13–F24, any media/consumer key, system keys (power, sleep, wake), and the device name in the scan response. Implemented `BleOutput`; all keys, mouse buttons 1–5, and both scroll directions verified on Windows.
- Implemented `Profiles` and `ProfileButton`: the four presets (GAMEPAD, KB+MOUSE, MEDIA, HYBRID with an R4 "PC layer"), up to 3 actions per button, Toggle/Turbo/Layer modes, stick modes, and the new profile button gestures (short press, double press, hold + A/B/X/Y). Verified on hardware, including WASD diagonals in a real game.
- Added `tools/InputTester.html`, a browser page that shows held keys, mouse buttons, scrolling, and gamepad buttons and sticks.
- Found that random power-offs and a right stick not resting at zero were caused by the ESP32 DevKit not being fully seated in its female headers (lost contact on the `5V` pin). Reseated the board; both problems disappeared.
- Implemented `Display` (switched from Adafruit SSD1306/GFX to U8g2, rotated 180°) and `BatteryMonitor` (MAX17048, battery level also reported to Windows). All on-device text is in English; profile names use one fixed font size.
- Implemented `Storage`: the controller starts on the last active profile. Enlarged the NVS settings store to 128 KB with a custom partition table. A first attempt that moved the app to 0x20000 boot-looped, because PlatformIO always flashes the app at 0x10000; the app stays at 0x10000 and NVS moved after it.
- Investigated the gamepad no longer working after a restart (Windows sometimes showing a phone or computer icon): the Bluetooth link came back, but the HID devices didn't. Added connection diagnostics, made room for more stored subscriptions, set the Bluetooth appearance to "gamepad", and count the controller as connected only once the link is encrypted. After one more re-pair, the controller has reconnected after every restart.
- Started the on-controller configuration menu (round 1): editing buttons, sticks, deadzones, and turbo speed, with Save / Discard / Keep editing when leaving.

## 2026-09-28 — Configuration Menu, Power Management and Firmware 1.0.0

- Finished the configuration menu (round 2): renaming (10 characters), new profiles from a preset or a copy, reordering and deleting profiles, and the Controller page (Forget Bluetooth, Reset all profiles, About). All verified on hardware, including after power cycles.
- Observed that right after **Forget Bluetooth**, the first pairing sometimes still doesn't survive a restart; one or two re-pairs fix it for good. The cause is not found yet; the debug build logs the stored pairings to investigate it.
- Added display settings: brightness (10 levels), screen timeout, and the profile name popup, previewed live in the menu.
- Found that the 10-second screen timeout never fired: at rest, the sticks jitter slightly past their deadzone. Sticks now count as activity only past 25% deflection.
- Added automatic power-off (separate times while connected and offline, default 5 minutes) with a 10-second full-screen countdown, plus a battery critical warning at 5% and automatic power-off at 2%. Experimentally verified that after the ESP32 enters deep sleep, the power module cuts the 5 V output completely, and that the on/off switch starts the controller again.
- Added **Recalibrate sticks** to the menu.
- Created the release and debug builds and tagged the firmware as **1.0.0**.
- Rewrote the repository documentation for the new firmware and added the User Guide.
