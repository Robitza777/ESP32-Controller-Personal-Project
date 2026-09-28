# Firmware

PlatformIO project for the controller's ESP32, firmware version **1.0.0**. How the firmware works is described in [`docs/Software.md`](../docs/Software.md) and [`docs/Architecture.md`](../docs/Architecture.md); how to use the controller in [`docs/UserGuide.md`](../docs/UserGuide.md).

## Requirements

* [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) extension (or the PlatformIO Core CLI).
* A USB cable to the ESP32 DevKit (CP2102 USB-serial chip; Windows may need the Silicon Labs CP210x driver).

Everything else (the ESP32 toolchain, Arduino core, and libraries) is downloaded by PlatformIO on the first build, at the exact versions pinned in [`platformio.ini`](platformio.ini).

## Build and Flash

Open **this folder** (`firmware/`, the one containing `platformio.ini`) in VS Code, not the repository root.

| Build | What it is | Command |
|---|---|---|
| `release` | The default. What stays on the controller: no serial output. | `pio run -t upload` |
| `debug` | The same firmware, plus a serial log: Bluetooth pairing and connection, battery readings, power decisions, saves. | `pio run -e debug -t upload` |

In VS Code, the same actions are under **PlatformIO → Project Tasks → release / debug → Upload**. To read the debug log, open the serial monitor (`pio device monitor`, 115200 baud) after uploading.

Uploading only replaces the program: saved profiles, settings, and Bluetooth pairings are kept.

**Don't hold any button while the board resets or is being flashed.** A, X, D-pad Up, R2, and R4 are on ESP32 boot pins (see [`Pinout.md`](../hardware/pinout/Pinout.md)).

### First Flash

The project uses its own partition table ([`partitions.csv`](partitions.csv)) with a larger settings store (NVS, 128 KB). If the board was previously flashed with a different partition table, erase it once with **PlatformIO → Project Tasks → release → Platform → Erase Flash** before uploading. Erasing also removes the Bluetooth pairing, so the controller must be removed and paired again in Windows.

## Folder Structure

```text
firmware/
├── platformio.ini     — Board, libraries (pinned versions), release and debug builds
├── partitions.csv     — Flash layout: app 3 MB, NVS 128 KB
├── include/           — Module headers: public interface and design notes
│   ├── Config.h       — Every pin, address, timing and threshold
│   └── ...
├── src/               — Module implementations; main.cpp starts the tasks and connects the modules
└── lib/
    └── ESP32-BLE-CompositeHID/   — Vendored library with local changes (see its VENDORED.md)
```

| Module | Files | Responsibility |
|---|---|---|
| main | `main.cpp` | Starts the input and UI tasks, passes data between modules, runs the menu hand-off and the power-off sequence |
| InputReader | `InputReader.h/.cpp` | Buttons and sticks: debounce, stick calibration, circular mapping, deadzone |
| ProfileButton | `ProfileButton.h/.cpp` | Profile button gestures |
| Profiles | `Profiles.h/.cpp`, `ProfileTypes.h` | Presets, profile list, and the engine that turns inputs into outputs |
| ConfigMenu | `ConfigMenu.h/.cpp`, `MenuView.h` | The on-controller configuration menu |
| ActionCatalog | `ActionCatalog.h/.cpp`, `HidUsages.h` | Named, grouped action lists offered by the menu |
| BleOutput | `BleOutput.h/.cpp` | Bluetooth LE HID device: Xbox controller, keyboard, media keys, mouse |
| Display | `Display.h/.cpp` | OLED: status screen, menu, alerts |
| BatteryMonitor | `BatteryMonitor.h/.cpp` | MAX17048 fuel gauge |
| PowerManager | `PowerManager.h/.cpp` | Automatic power-off and battery protection |
| Storage | `Storage.h/.cpp` | Profiles and settings in NVS |
| (shared) | `ControllerTypes.h`, `Log.h` | Data types passed between modules, debug logging |

## Libraries

| Library | Version | Used for |
|---|---|---|
| [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID) | commit `032b064`, vendored in `lib/` | Xbox controller + keyboard + mouse over one BLE connection |
| [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) | 2.5.1 | Bluetooth LE stack |
| [Callback](https://github.com/tomstewart89/Callback) | 1.1 | Required by ESP32-BLE-CompositeHID |
| [U8g2](https://github.com/olikraus/u8g2) | 2.36.18 | SSD1306 OLED |
| [SparkFun MAX1704x Fuel Gauge](https://github.com/sparkfun/SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library) | 1.0.4 | MAX17048 fuel gauge |

Platform: `espressif32 @ 7.0.1` (Arduino core 2.0.17, ESP-IDF 4.4), C++17.
