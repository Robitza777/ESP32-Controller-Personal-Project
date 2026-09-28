#pragma once

// Settings kept across power-offs, in the NVS partition (see partitions.csv): the user's profile list, the last
// active profile, the display settings and the auto power-off times.
//
// A flash write stalls both cores while it runs (usually under 1 ms, rarely tens of ms when NVS compacts a page), so
// writes happen only from the UI task: on an explicit save in the menu, or a few seconds after a profile switch.
// Never from the input task. Loading happens once, in setup(), before the tasks start.
//
// Saved profiles carry PROFILE_FORMAT_VERSION. A firmware with a different profile layout ignores them and the
// presets come back, until a migration for that version is written here.

#include "ProfileTypes.h"

namespace Storage {

void begin();

// Reads the saved list into `list`; returns how many profiles were read. 0 = nothing saved yet, or saved in a format
// this firmware can't read.
uint8_t loadProfiles(Profile* list, uint8_t capacity);

// How many profiles flash holds (0 while the presets are in use). A save must also write every slot at or beyond
// this, or the stored list would have holes.
uint8_t savedProfileCount();

// Writes one profile to its slot. saveProfileCount() makes the list (and the format version) official, so after
// adding, deleting or reordering: save the affected slots first, then the count.
bool saveProfile(uint8_t index, const Profile& profile);
bool saveProfileCount(uint8_t count);

uint8_t loadActiveIndex();  // 0 when nothing saved
bool saveActiveIndex(uint8_t index);

// Controller-wide display settings; the defaults when nothing valid is stored.
DisplaySettings loadDisplaySettings();
bool saveDisplaySettings(const DisplaySettings& settings);

// Auto power-off times; the defaults when nothing valid is stored.
PowerSettings loadPowerSettings();
bool savePowerSettings(const PowerSettings& settings);

// Factory reset of the profiles: the presets come back at the next boot. BLE pairing is kept.
void eraseProfiles();

}  // namespace Storage
