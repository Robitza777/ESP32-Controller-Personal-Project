#pragma once

// Automatic power-off and battery warnings. UI task only.
//
// The firmware can't disconnect the battery. It powers off by entering deep sleep: the load drops far below the
// IP5306's ~50 mA threshold and the IP5306 switches its 5 V output off ~32 s later. The power switch (off, then on)
// starts the controller again.
//
//   no activity for the auto-off time (PowerSettings; one time while connected to a host, one while not)
//       -> the last Config::AUTO_OFF_WARNING_S seconds show a full-screen countdown; any activity cancels it -> off
//   battery at or below Config::BATTERY_CRITICAL_PERCENT, not charging
//       -> full-screen "battery critical" for Config::BATTERY_ALERT_SHOW_MS, again every Config::BATTERY_ALERT_REPEAT_MS
//   battery at or below Config::BATTERY_EMPTY_PERCENT, not charging, in Config::BATTERY_EMPTY_READINGS readings
//   in a row (one bad reading must not switch the controller off) -> "battery empty" briefly -> off
//
// Activity = any button, or a stick past Config::ACTIVITY_STICK_PERCENT (the same rule as the screen timeout).
// This module only decides; main shows the alerts and runs the shutdown.

#include "ControllerTypes.h"

namespace PowerManager {

enum class Alert : uint8_t { None, AutoOffCountdown, BatteryCritical, BatteryEmpty };

struct Decision {
  Alert alert = Alert::None;  // shown full screen over everything else, the screen lit even if it was asleep
  uint8_t secondsLeft = 0;    // AutoOffCountdown
  uint8_t batteryPercent = 0; // BatteryCritical / BatteryEmpty
  bool powerOff = false;      // shut down now
};

void begin();

// Call every UI cycle. `battery` is the latest fuel-gauge status; `newBatteryReading` is true in the cycle a new
// reading arrived (the empty-battery check counts readings, not cycles).
Decision update(uint32_t nowMs, uint32_t lastActivityMs, bool connected, const BatteryStatus& battery,
                bool newBatteryReading, const PowerSettings& settings);

}  // namespace PowerManager
