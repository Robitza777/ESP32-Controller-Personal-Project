#include "BatteryMonitor.h"

#include <Arduino.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <Wire.h>

#include "Config.h"
#include "Log.h"

namespace BatteryMonitor {
namespace {

// The library defaults to the MAX17043, which scales voltage and change rate differently.
SFE_MAX1704X gauge(MAX1704X_MAX17048);

BatteryStatus current;
bool polledOnce = false;
uint32_t lastPollMs = 0;

}  // namespace

void begin() {
  current.present = gauge.begin(Wire);
  LOG("BatteryMonitor: MAX17048 %s", current.present ? "found" : "NOT found");
}

bool update(uint32_t nowMs) {
  if (polledOnce && nowMs - lastPollMs < Config::BATTERY_POLL_MS) return false;
  polledOnce = true;
  lastPollMs = nowMs;

  current.present = gauge.isConnected();
  if (!current.present) return false;

  // The gauge can report a little over 100 % right after a full charge.
  current.percent = static_cast<uint8_t>(constrain(lroundf(gauge.getSOC()), 0L, 100L));
  const float ratePercentPerHour = gauge.getChangeRate();
  current.charging = ratePercentPerHour > Config::BATTERY_CHARGING_RATE;
  LOG("BatteryMonitor: %u%%, %.2f V, %+.1f %%/h%s", current.percent, gauge.getVoltage(), ratePercentPerHour,
      current.charging ? " (charging)" : "");
  return true;
}

BatteryStatus status() { return current; }

}  // namespace BatteryMonitor
