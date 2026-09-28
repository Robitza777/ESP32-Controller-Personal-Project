#pragma once

// MAX17048 fuel gauge: state of charge and whether the battery is charging. UI task only (I2C).
// It sits across the battery terminals and keeps counting while the controller is switched off, so the first
// reading after power-on is already accurate. If it doesn't answer, status().present stays false.

#include "ControllerTypes.h"

namespace BatteryMonitor {

void begin();

// Reads the gauge every Config::BATTERY_POLL_MS; cheap to call every UI cycle. True when a new reading came in.
bool update(uint32_t nowMs);

BatteryStatus status();

}  // namespace BatteryMonitor
