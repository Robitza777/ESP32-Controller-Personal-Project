#pragma once

// Profile button gestures. Input task only.
//
//   short press              next profile (after Config::PROFILE_DOUBLE_PRESS_MS, to tell it from a double press)
//   double press             back to the previously active profile
//   hold + A / B / X / Y     jump to profile 1 / 2 / 3 / 4
//   hold Config::PROFILE_HOLD_MS with nothing else pressed   open the config menu
//
// Buttons pressed while Profile is held are kept from the host until they are released.

#include "ControllerTypes.h"

namespace ProfileButton {

enum class Gesture : uint8_t { None, Next, Previous, JumpTo, OpenMenu };

struct Event {
  Gesture gesture = Gesture::None;
  uint8_t profileIndex = 0;  // JumpTo only
};

void begin();

// Call once per input cycle. Returns at most one gesture per call. Sets `suppressedButtons` to the buttons the host
// must not see this cycle (always includes Profile itself).
Event update(const InputState& input, uint32_t nowMs, uint32_t& suppressedButtons);

// After the config menu closes: forgets any gesture in progress; a Profile press still held ends without a gesture.
void reset();

}  // namespace ProfileButton
