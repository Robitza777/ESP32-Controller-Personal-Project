#pragma once

// Plain data shared between modules. Modules depend on these types, not on each other.

#include <stdint.h>

// Physical buttons. The order is the index into Config::BUTTON_PINS and the bit position in InputState::buttons.
enum class Button : uint8_t {
  A, B, X, Y,
  DpadUp, DpadDown, DpadLeft, DpadRight,
  L1, L2, L3, L4,
  R1, R2, R3, R4,
  Options, Profile,
  Count
};
constexpr uint8_t BUTTON_COUNT = static_cast<uint8_t>(Button::Count);

constexpr uint32_t buttonBit(Button button) { return 1u << static_cast<uint8_t>(button); }

enum class Axis : uint8_t { LeftX, LeftY, RightX, RightY, Count };
constexpr uint8_t AXIS_COUNT = static_cast<uint8_t>(Axis::Count);

enum class Stick : uint8_t { Left, Right, Count };
constexpr uint8_t STICK_COUNT = static_cast<uint8_t>(Stick::Count);

constexpr int16_t AXIS_MAX = 32767;  // full deflection; the range is symmetric (-AXIS_MAX..AXIS_MAX)

// Every physical input after debouncing and stick processing.
// Axes are -AXIS_MAX..AXIS_MAX with +X = right and +Y = up; outputs that use another convention convert at their end.
struct InputState {
  uint32_t buttons = 0;  // bit n set = Button n pressed
  int16_t axes[AXIS_COUNT] = {};

  bool isPressed(Button button) const { return buttons & buttonBit(button); }
  int16_t axis(Axis a) const { return axes[static_cast<uint8_t>(a)]; }
};

// ---------------------------------------------------------------------------------------------
// Outputs: everything the host should see after the active profile has been applied.
// Same axis convention as InputState (+X right, +Y up); BleOutput converts to what each HID device expects.

// Xbox Series controller buttons, as XInput games see them.
enum class XboxButton : uint8_t { A, B, X, Y, LB, RB, View, Menu, Guide, LS, RS, Share, Count };

enum class DpadDirection : uint8_t { Up, Down, Left, Right, Count };

constexpr uint16_t TRIGGER_MAX = 1023;

struct GamepadReport {
  uint16_t buttons = 0;  // bit n = XboxButton n
  uint8_t dpad = 0;      // bit n = DpadDirection n
  int16_t leftX = 0, leftY = 0, rightX = 0, rightY = 0;
  uint16_t leftTrigger = 0, rightTrigger = 0;  // 0..TRIGGER_MAX
};

constexpr uint8_t MAX_KEYS = 6;  // simultaneous non-modifier keys in a HID keyboard report

struct KeyboardReport {
  uint8_t modifiers = 0;      // HID modifier bits (Ctrl, Shift, Alt, GUI; left and right)
  uint8_t keys[MAX_KEYS] = {};  // HID usage IDs, 0 = empty slot
};

constexpr uint8_t MAX_CONSUMER_KEYS = 2;  // media/launch/browser keys held at the same time

enum class SystemKey : uint8_t { PowerDown, Sleep, WakeUp, Count };

enum class MouseButton : uint8_t { Left, Right, Middle, Back, Forward, Count };

// Mouse motion is a rate, so profiles express it as the fraction of counts to move during this input cycle;
// BleOutput accumulates the fractions and sends whole counts.
struct MouseReport {
  uint8_t buttons = 0;  // bit n = MouseButton n
  float moveX = 0.0f;   // counts this cycle, +X right
  float moveY = 0.0f;   // counts this cycle, +Y up
  float wheel = 0.0f;   // steps this cycle, + = scroll up
  float pan = 0.0f;     // steps this cycle, + = scroll right
};

struct OutputReport {
  GamepadReport gamepad;
  KeyboardReport keyboard;
  uint16_t consumer[MAX_CONSUMER_KEYS] = {};  // HID consumer-page usages (media, launch, browser...), 0 = none
  uint8_t system = 0;                         // bit n = SystemKey n
  MouseReport mouse;
};

// ---------------------------------------------------------------------------------------------
// Battery

struct BatteryStatus {
  bool present = false;   // the fuel gauge answered
  uint8_t percent = 0;    // 0-100
  bool charging = false;  // charge rate above Config::BATTERY_CHARGING_RATE
};

// ---------------------------------------------------------------------------------------------
// Controller-wide display settings (not per profile): Controller page of the config menu, kept in flash.
// The defaults are what the controller used before these were settings (checked in Config.h).

struct DisplaySettings {
  uint8_t brightness = 4;  // level 1..Config::DISPLAY_BRIGHTNESS_LEVELS
  uint8_t screenOff = 1;   // index into Config::DISPLAY_SLEEP_OPTIONS_S
  bool namePopup = true;   // the new profile's name full screen after a switch
};

// Controller-wide auto power-off (PowerManager): minutes without activity, as indexes into
// Config::AUTO_OFF_OPTIONS_MIN. Defaults checked in Config.h.
struct PowerSettings {
  uint8_t offWhenConnected = 3;  // while a host is connected
  uint8_t offWhenOffline = 3;    // while nothing is connected
};
