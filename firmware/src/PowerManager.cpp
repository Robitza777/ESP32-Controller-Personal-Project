#include "PowerManager.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

namespace PowerManager {
namespace {

// Battery empty: counted readings, then a short notice, then off.
uint8_t emptyReadings = 0;
bool emptyShutdown = false;
uint32_t emptySinceMs = 0;

// Battery critical: the alert comes back every BATTERY_ALERT_REPEAT_MS while the level stays low.
constexpr uint8_t CRITICAL_CLEAR_PERCENT = Config::BATTERY_CRITICAL_PERCENT + 2;  // no flicker around the threshold
bool critical = false;
uint32_t nextCriticalAlertMs = 0;
uint32_t criticalAlertUntilMs = 0;

bool countdownLogged = false;

uint32_t autoOffMs(const PowerSettings& settings, bool connected) {
  const uint8_t index = connected ? settings.offWhenConnected : settings.offWhenOffline;
  if (index >= Config::AUTO_OFF_OPTION_COUNT) return 0;
  return Config::AUTO_OFF_OPTIONS_MIN[index] * 60000u;  // 0 = never
}

bool reached(uint32_t nowMs, uint32_t timeMs) { return static_cast<int32_t>(nowMs - timeMs) >= 0; }

}  // namespace

void begin() {}

Decision update(uint32_t nowMs, uint32_t lastActivityMs, bool connected, const BatteryStatus& battery,
                bool newBatteryReading, const PowerSettings& settings) {
  Decision decision;
  decision.batteryPercent = battery.percent;
  const bool onBattery = battery.present && !battery.charging;

  // Battery empty. Once started it can't be cancelled: the cell is being protected.
  if (newBatteryReading && !emptyShutdown) {
    emptyReadings = onBattery && battery.percent <= Config::BATTERY_EMPTY_PERCENT ? emptyReadings + 1 : 0;
    if (emptyReadings >= Config::BATTERY_EMPTY_READINGS) {
      emptyShutdown = true;
      emptySinceMs = nowMs;
      LOG("power: battery empty (%u%%), powering off", battery.percent);
    }
  }
  if (emptyShutdown) {
    decision.alert = Alert::BatteryEmpty;
    decision.powerOff = reached(nowMs, emptySinceMs + Config::BATTERY_EMPTY_SHOW_MS);
    return decision;
  }

  // Auto power-off. Signed: lastActivityMs comes from the input task and can be a little newer than nowMs.
  const uint32_t timeoutMs = autoOffMs(settings, connected);
  const int32_t idleMs = static_cast<int32_t>(nowMs - lastActivityMs);
  if (timeoutMs != 0 && idleMs >= static_cast<int32_t>(timeoutMs)) {
    LOG("power: %lu min without activity (%s), powering off", static_cast<unsigned long>(timeoutMs / 60000),
        connected ? "connected" : "offline");
    decision.powerOff = true;
    return decision;
  }
  const int32_t leftMs = static_cast<int32_t>(timeoutMs) - idleMs;
  if (timeoutMs != 0 && leftMs <= static_cast<int32_t>(Config::AUTO_OFF_WARNING_S * 1000u)) {
    if (!countdownLogged) LOG("power: auto-off countdown");
    countdownLogged = true;
    decision.alert = Alert::AutoOffCountdown;
    decision.secondsLeft = static_cast<uint8_t>((leftMs + 999) / 1000);
    return decision;
  }
  if (countdownLogged) LOG("power: auto-off cancelled");
  countdownLogged = false;

  // Battery critical: a few seconds, repeated, while running on battery.
  if (onBattery && battery.percent <= Config::BATTERY_CRITICAL_PERCENT && !critical) {
    critical = true;
    nextCriticalAlertMs = nowMs;
    LOG("power: battery critical (%u%%)", battery.percent);
  } else if (!onBattery || battery.percent >= CRITICAL_CLEAR_PERCENT) {
    critical = false;
  }
  if (critical) {
    if (reached(nowMs, nextCriticalAlertMs)) {
      criticalAlertUntilMs = nowMs + Config::BATTERY_ALERT_SHOW_MS;
      nextCriticalAlertMs = nowMs + Config::BATTERY_ALERT_REPEAT_MS;
    }
    if (!reached(nowMs, criticalAlertUntilMs)) decision.alert = Alert::BatteryCritical;
  }
  return decision;
}

}  // namespace PowerManager
