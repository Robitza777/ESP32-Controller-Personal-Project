#pragma once

// SSD1306 128x64 OLED through U8g2, mounted rotated 180 degrees. UI task only (it owns the I2C bus).
//
// Status screen: BLE state and battery (4 bars + percent) on top, the active profile's name and number in the
// middle, an RTZ / TOGGLE tag while one is on. A profile switch shows the new name full screen for
// Config::PROFILE_POPUP_MS. Brightness, the screen timeout and the popup come from DisplaySettings (defaults until
// applySettings()); after the timeout without input the screen switches off and any input wakes it.
// Redraws only when something shown has changed (a full frame is ~23 ms of I2C).
// If no display answers on the bus, every call does nothing.

#include "ControllerTypes.h"
#include "MenuView.h"
#include "ProfileTypes.h"

namespace Display {

// Everything the screen shows; main fills it from the input task's snapshot plus the battery.
struct State {
  char profileName[PROFILE_NAME_MAX + 1] = {};
  uint8_t profileNumber = 0;    // 1-based position in the list
  uint8_t profileCount = 0;
  uint32_t profileSwitches = 0;  // counts switches; a new value triggers the full-screen popup
  bool layerActive = false;
  bool toggleActive = false;
  bool bleConnected = false;
  BatteryStatus battery;
  uint32_t lastInputMs = 0;      // last activity: a button, or a stick past Config::ACTIVITY_STICK_PERCENT
};

void begin();

// Brightness, screen timeout and the name popup. Takes effect at once, so the menu can preview a change.
void applySettings(const DisplaySettings& settings);

void update(const State& state, uint32_t nowMs);

// The config menu instead of the status screen, until update() is called again. Same sleep rule as the status
// screen.
void showMenu(const MenuView& view, uint32_t lastInputMs, uint32_t nowMs);

// A full-screen alert (power-off countdown, battery warnings) over everything else, lit even if the screen was
// asleep: a short line on top, one big word or number, a short line below.
void showAlert(const char* top, const char* big, const char* bottom);

// Blanks the panel before the controller powers off (it would otherwise keep showing the last frame).
void powerOff();

// True while the screen is switched off. The menu ignores the input that wakes it, so a press in the dark can't
// change a setting nobody saw.
bool asleep();

}  // namespace Display
