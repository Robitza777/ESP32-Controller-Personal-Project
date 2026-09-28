#pragma once

// The profile list the user switches through, the built-in presets, and the engine that turns inputs into outputs
// through the active profile. Input task only; the config menu edits a copy, which main hands back through
// replaceAll().

#include "ControllerTypes.h"
#include "ProfileTypes.h"

namespace Profiles {

// Read-only templates in flash. A new profile starts as a copy of one of them.
enum class Preset : uint8_t { Gamepad, KeyboardMouse, Media, Hybrid, Count };

// Built in begin() and never changed afterwards, so the config menu may read them from the UI task.
const Profile& preset(Preset preset);

// Loads the saved list through `load` (it fills the array and returns how many it read). With nothing saved, the
// list gets one profile per preset. `savedActive` is the profile to start on; out of range falls back to the first.
using LoadProfiles = uint8_t (*)(Profile* list, uint8_t capacity);
void begin(LoadProfiles load, uint8_t savedActive);

uint8_t count();
const Profile& get(uint8_t index);
uint8_t activeIndex();
const Profile& active();

// Switching resets toggles, turbo and layers, and remembers the profile left for selectPrevious().
void select(uint8_t index);
void selectNext();      // wraps around
void selectPrevious();  // back to the profile active before the last switch; repeating it toggles between the two

// For the status screen: the second layer is active / some Toggle-mode button is latched on.
bool layerActive();
bool toggleActive();

// After the config menu closes, while the input task is the only user again: take the saved list, or keep the
// current one. Both reset toggles, turbo and layers, and keep buttons still held from the host until released.
void replaceAll(const Profile* list, uint8_t count, uint8_t active);
void resync();

// Maps one input cycle through the active profile. Keeps toggle, turbo, layer and repeat state between calls, so
// call it exactly once per cycle with that cycle's nowUs and dtS (seconds since the previous cycle).
OutputReport apply(const InputState& input, uint32_t nowUs, float dtS);

}  // namespace Profiles
