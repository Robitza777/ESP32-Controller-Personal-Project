#include "Storage.h"

#include <Arduino.h>
#include <Preferences.h>

#include "Config.h"
#include "Log.h"

namespace Storage {
namespace {

// One NVS namespace for everything the firmware stores; NimBLE keeps its pairing data in its own.
constexpr const char* NAMESPACE = "controller";
constexpr const char* KEY_FORMAT = "format";
constexpr const char* KEY_COUNT = "count";
constexpr const char* KEY_ACTIVE = "active";
constexpr const char* KEY_DISPLAY = "display";
constexpr const char* KEY_POWER = "power";

Preferences prefs;
bool opened = false;
uint8_t savedCount = 0;

struct ProfileKey {
  char text[4];  // "p0".."p15"
  explicit ProfileKey(uint8_t index) { snprintf(text, sizeof(text), "p%u", index); }
};

}  // namespace

void begin() {
  opened = prefs.begin(NAMESPACE, false);
  if (!opened) LOG("Storage: could not open NVS, nothing will be saved");
}

uint8_t loadProfiles(Profile* list, uint8_t capacity) {
  if (!opened) return 0;
  const uint8_t count = min(prefs.getUChar(KEY_COUNT, 0), capacity);
  if (count == 0) {
    LOG("Storage: no saved profiles, using the presets");
    return 0;
  }
  const uint16_t format = prefs.getUShort(KEY_FORMAT, 0);
  if (format != PROFILE_FORMAT_VERSION) {
    LOG("Storage: saved profiles are format %u, this firmware reads %u -> using the presets", format,
        PROFILE_FORMAT_VERSION);
    return 0;
  }

  for (uint8_t i = 0; i < count; ++i) {
    const ProfileKey key(i);
    if (prefs.getBytesLength(key.text) != sizeof(Profile) ||
        prefs.getBytes(key.text, &list[i], sizeof(Profile)) != sizeof(Profile)) {
      LOG("Storage: profile slot %u is missing or damaged -> using the presets", i);
      return 0;
    }
    list[i].name[PROFILE_NAME_MAX] = '\0';
  }
  LOG("Storage: loaded %u profiles", count);
  savedCount = count;
  return count;
}

bool saveProfile(uint8_t index, const Profile& profile) {
  if (!opened || index >= MAX_PROFILES) return false;
  const ProfileKey key(index);
  return prefs.putBytes(key.text, &profile, sizeof(Profile)) == sizeof(Profile);
}

uint8_t savedProfileCount() { return savedCount; }

bool saveProfileCount(uint8_t count) {
  if (!opened) return false;
  prefs.putUShort(KEY_FORMAT, PROFILE_FORMAT_VERSION);
  const bool ok = prefs.putUChar(KEY_COUNT, count) == 1;
  if (ok) savedCount = count;
  return ok;
}

uint8_t loadActiveIndex() { return opened ? prefs.getUChar(KEY_ACTIVE, 0) : 0; }

bool saveActiveIndex(uint8_t index) {
  if (!opened) return false;
  const bool ok = prefs.putUChar(KEY_ACTIVE, index) == 1;
  LOG("Storage: active profile %u %s", index + 1, ok ? "saved" : "NOT saved");
  return ok;
}

DisplaySettings loadDisplaySettings() {
  DisplaySettings settings;
  if (!opened || prefs.getBytesLength(KEY_DISPLAY) != sizeof(DisplaySettings)) return DisplaySettings{};
  prefs.getBytes(KEY_DISPLAY, &settings, sizeof(settings));
  // A damaged or foreign value must not leave the screen black or the timeout out of range.
  if (settings.brightness < 1 || settings.brightness > Config::DISPLAY_BRIGHTNESS_LEVELS ||
      settings.screenOff >= Config::DISPLAY_SLEEP_OPTION_COUNT) {
    LOG("Storage: display settings out of range, using the defaults");
    return DisplaySettings{};
  }
  return settings;
}

bool saveDisplaySettings(const DisplaySettings& settings) {
  if (!opened) return false;
  return prefs.putBytes(KEY_DISPLAY, &settings, sizeof(settings)) == sizeof(settings);
}

PowerSettings loadPowerSettings() {
  PowerSettings settings;
  if (!opened || prefs.getBytesLength(KEY_POWER) != sizeof(PowerSettings)) return PowerSettings{};
  prefs.getBytes(KEY_POWER, &settings, sizeof(settings));
  if (settings.offWhenConnected >= Config::AUTO_OFF_OPTION_COUNT ||
      settings.offWhenOffline >= Config::AUTO_OFF_OPTION_COUNT) {
    LOG("Storage: power settings out of range, using the defaults");
    return PowerSettings{};
  }
  return settings;
}

bool savePowerSettings(const PowerSettings& settings) {
  if (!opened) return false;
  return prefs.putBytes(KEY_POWER, &settings, sizeof(settings)) == sizeof(settings);
}

void eraseProfiles() {
  if (!opened) return;
  prefs.clear();
  savedCount = 0;
  LOG("Storage: profiles erased, presets at next boot");
}

}  // namespace Storage
