#pragma once

// Every pin, address, timing and threshold lives here. Pin source of truth: Pinout.md.

#include <Arduino.h>
#include "ControllerTypes.h"

namespace Config {

// ---------------------------------------------------------------------------------------------
// Buttons (all active LOW)

struct ButtonPin {
  uint8_t gpio;
  bool externalPullup;  // input-only GPIO36/39 have no internal pull-up; GPIO2 has an external 1k
  bool glitchProne;     // ESP32 errata: GPIO36/39 can read LOW for ~80 ns when the ADC powers up
};

// Indexed by Button.
constexpr ButtonPin BUTTON_PINS[BUTTON_COUNT] = {
  {0, false, false},   // A          strapping pin: don't hold at boot/flash
  {17, false, false},  // B
  {2, true, false},    // X          strapping pin, on-board LED
  {4, false, false},   // Y
  {5, false, false},   // D-Up       strapping pin
  {39, true, true},    // D-Down     input-only
  {23, false, false},  // D-Left
  {13, false, false},  // D-Right
  {18, false, false},  // L1
  {19, false, false},  // L2         digital trigger
  {25, false, false},  // L3
  {14, false, false},  // L4         back paddle
  {16, false, false},  // R1
  {15, false, false},  // R2         digital trigger, strapping pin
  {26, false, false},  // R3
  {12, false, false},  // R4         back paddle, strapping pin (must not be HIGH at boot)
  {27, false, false},  // Options
  {36, true, true},    // Profile    input-only
};

// ---------------------------------------------------------------------------------------------
// Sticks (ADC1, safe to use with BLE)

struct AxisPin {
  uint8_t gpio;
  bool invert;  // raw X falls when pushed right, raw Y rises when pushed up; output is +X right, +Y up
};

// Indexed by Axis.
constexpr AxisPin AXIS_PINS[AXIS_COUNT] = {
  {34, true},   // Left X   ADC1_CH6
  {35, false},  // Left Y   ADC1_CH7
  {32, true},   // Right X  ADC1_CH4
  {33, false},  // Right Y  ADC1_CH5
};

constexpr int ADC_MAX = 4095;  // 12-bit, 11 dB attenuation; the sticks saturate both ends before full travel
constexpr int ADC_MID = 2048;
constexpr uint8_t STICK_OVERSAMPLING = 4;        // ADC reads averaged per axis per scan
constexpr uint16_t STICK_CENTER_SAMPLES = 64;    // reads averaged for the power-on center
constexpr int STICK_CENTER_MAX_OFFSET = 400;     // raw counts; a center farther from ADC_MID means a touched stick
constexpr uint8_t STICK_DEADZONE_PERCENT = 5;
constexpr uint8_t STICK_DEADZONE_MAX_PERCENT = 50;

// ---------------------------------------------------------------------------------------------
// I2C bus, shared by the OLED and the fuel gauge. Only the UI task may use it.

constexpr uint8_t I2C_SDA = 21;
constexpr uint8_t I2C_SCL = 22;
constexpr uint32_t I2C_FREQUENCY_HZ = 400000;
constexpr uint8_t OLED_ADDRESS = 0x3C;
constexpr uint8_t FUEL_GAUGE_ADDRESS = 0x36;

// ---------------------------------------------------------------------------------------------
// Input timing

constexpr uint32_t INPUT_SCAN_PERIOD_MS = 1;
constexpr uint32_t DEBOUNCE_LOCKOUT_US = 5000;   // eager debounce: press counts at once, then the pin is ignored
constexpr uint32_t MIN_REPORT_INTERVAL_US = 3750;  // at most ~2 reports per 7.5 ms connection event
constexpr uint32_t PROFILE_HOLD_MS = 3000;       // hold Profile this long to enter config mode
static_assert(DEBOUNCE_LOCKOUT_US >= MIN_REPORT_INTERVAL_US,
              "every press and release must outlast the report rate limit, or a quick tap could be lost");

// ---------------------------------------------------------------------------------------------
// Profiles

constexpr uint32_t PROFILE_DOUBLE_PRESS_MS = 300;  // a second Profile press within this goes back a profile
constexpr uint32_t ACTIVE_PROFILE_SAVE_DELAY_MS = 3000;  // remember the active profile once it stays this long
constexpr float STICK_DIRECTION_PRESS = 0.50f;     // Directions mode: fraction of full deflection that presses
constexpr float STICK_DIRECTION_RELEASE = 0.35f;   // ...and releases (hysteresis, no chatter at the threshold)
constexpr float MOUSE_COUNTS_PER_S_PER_SENSITIVITY = 300.0f;  // at full deflection; sensitivity 5 = 1500 counts/s
constexpr float MOUSE_CURVE_EXPONENT = 2.0f;       // speed = deflection^exponent: precise near the center
constexpr float SCROLL_STEPS_PER_S_PER_SENSITIVITY = 4.0f;    // at full deflection; sensitivity 5 = 20 steps/s
constexpr uint8_t SENSITIVITY_MIN = 1;
constexpr uint8_t SENSITIVITY_MAX = 10;
constexpr uint32_t SCROLL_REPEAT_DELAY_MS = 300;   // a Scroll action held this long starts repeating...
constexpr uint32_t SCROLL_REPEAT_INTERVAL_MS = 60; // ...one step every this often
constexpr uint8_t TURBO_DEFAULT_HZ = 10;
constexpr uint8_t TURBO_MIN_HZ = 1;
constexpr uint8_t TURBO_MAX_HZ = 20;
static_assert(1000000 / (2 * TURBO_MAX_HZ) >= MIN_REPORT_INTERVAL_US, "each turbo pulse must reach the host");

// ---------------------------------------------------------------------------------------------
// Display and battery (UI task)

constexpr uint32_t PROFILE_POPUP_MS = 1000;       // full-screen profile name after a switch
// A stick counts as activity (screen timeout and wake-up, auto power-off) only past this deflection. At rest a
// stick can jitter a count or two past its deadzone, which is invisible in use but would keep the screen on.
constexpr uint8_t ACTIVITY_STICK_PERCENT = 25;
constexpr uint32_t BATTERY_POLL_MS = 5000;
constexpr float BATTERY_CHARGING_RATE = 0.5f;     // %/h; the gauge's charge rate above this means charging
constexpr uint8_t BATTERY_BAR_THRESHOLDS[] = {10, 25, 50, 75};  // percent at which bar 1, 2, 3, 4 lights up

// Display settings (Controller page of the menu). Brightness levels map to the SSD1306 contrast byte; the panel
// brightens fastest at the low end, so the steps are closer together there.
constexpr uint8_t DISPLAY_CONTRAST_LEVELS[] = {1, 4, 8, 16, 32, 64, 96, 144, 200, 255};
constexpr uint8_t DISPLAY_BRIGHTNESS_LEVELS = sizeof(DISPLAY_CONTRAST_LEVELS);
constexpr uint16_t DISPLAY_SLEEP_OPTIONS_S[] = {10, 30, 60, 120, 300, 0};  // 0 = never
constexpr uint8_t DISPLAY_SLEEP_OPTION_COUNT = sizeof(DISPLAY_SLEEP_OPTIONS_S) / sizeof(DISPLAY_SLEEP_OPTIONS_S[0]);
static_assert(DISPLAY_CONTRAST_LEVELS[DisplaySettings{}.brightness - 1] == 16,
              "default brightness = contrast 16, the low level the user picked (the screen is only glanced at)");
static_assert(DISPLAY_SLEEP_OPTIONS_S[DisplaySettings{}.screenOff] == 30, "default: screen off after 30 s");

// ---------------------------------------------------------------------------------------------
// Power (PowerManager, UI task)

constexpr uint8_t AUTO_OFF_OPTIONS_MIN[] = {1, 2, 3, 5, 10, 15, 30, 60, 0};  // 0 = never
constexpr uint8_t AUTO_OFF_OPTION_COUNT = sizeof(AUTO_OFF_OPTIONS_MIN);
static_assert(AUTO_OFF_OPTIONS_MIN[PowerSettings{}.offWhenConnected] == 5 &&
                  AUTO_OFF_OPTIONS_MIN[PowerSettings{}.offWhenOffline] == 5,
              "default: off after 5 minutes without activity, connected or not (user, 2026-09-28)");
constexpr uint8_t AUTO_OFF_WARNING_S = 10;          // full-screen countdown before an auto power-off
constexpr uint8_t BATTERY_CRITICAL_PERCENT = 5;     // full-screen warning at or below this
constexpr uint32_t BATTERY_ALERT_SHOW_MS = 5000;    // ...shown this long
constexpr uint32_t BATTERY_ALERT_REPEAT_MS = 60000; // ...this often
constexpr uint8_t BATTERY_EMPTY_PERCENT = 2;        // power off at or below this, to protect the Li-Po
constexpr uint8_t BATTERY_EMPTY_READINGS = 3;       // ...seen this many readings in a row (BATTERY_POLL_MS apart)
constexpr uint32_t BATTERY_EMPTY_SHOW_MS = 3000;    // "battery empty" on screen before powering off
constexpr uint32_t POWER_OFF_DISCONNECT_MS = 300;   // time for the disconnect to reach the host before deep sleep

// ---------------------------------------------------------------------------------------------
// Config menu (UI task)

constexpr const char* FIRMWARE_VERSION = "1.0.0";  // shown under Controller > About
constexpr uint32_t MENU_REPEAT_DELAY_MS = 400;     // a held direction starts repeating after this...
constexpr uint32_t MENU_REPEAT_INTERVAL_MS = 80;   // ...then moves once per interval, like a held keyboard key
constexpr float MENU_STICK_PRESS = 0.50f;          // left stick deflection that counts as a direction
constexpr uint32_t MENU_STICK_FASTEST_MS = 40;     // repeat interval at full deflection (slower near the threshold)

// ---------------------------------------------------------------------------------------------
// BLE

constexpr const char* BLE_DEVICE_NAME = "Robitza's ESP32 Gamepad";  // too long to advertise; sent in the scan response
constexpr const char* BLE_MANUFACTURER = "Robitza Electrical Engineering";
constexpr uint16_t BLE_CONNECTION_INTERVAL = 6;  // x 1.25 ms = 7.5 ms, the BLE minimum (Windows accepts it)

// ---------------------------------------------------------------------------------------------
// Tasks. The NimBLE host runs on core 0 at a higher priority than the UI task.

constexpr BaseType_t INPUT_TASK_CORE = 1;
constexpr UBaseType_t INPUT_TASK_PRIORITY = 10;
constexpr uint32_t INPUT_TASK_STACK_BYTES = 8192;

constexpr BaseType_t UI_TASK_CORE = 0;
constexpr UBaseType_t UI_TASK_PRIORITY = 1;
constexpr uint32_t UI_TASK_STACK_BYTES = 8192;
constexpr uint32_t UI_TASK_PERIOD_MS = 20;

}  // namespace Config
