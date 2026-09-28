#pragma once

// Samples every physical input into an InputState. Input task only (not thread-safe).
//
// Sticks: ADC1 is converted through the IDF low-level HAL (the drivers cost ~50-120 us per read), oversampled,
// scaled per direction around the center measured at power-on, clamped to a circle, then passed through the
// radial deadzone.
// Buttons (sampled after the sticks, so they are freshest): both GPIO input registers are read in one go, then
// eagerly debounced (a press counts on the first sample, then the pin is ignored for Config::DEBOUNCE_LOCKOUT_US).
// GPIO36/39 need two LOW samples in a row.
// The output is linear; response curves belong to the outputs that need them (e.g. the mouse).

#include "ControllerTypes.h"

namespace InputReader {

// Configures pins and ADC, then measures each stick's resting center. The sticks must be untouched at power-on;
// a center that is implausibly far from mid-scale is replaced by the nominal one.
void begin();

// Samples everything once. Call once per input cycle; nowUs is micros() taken at the start of the cycle.
const InputState& read(uint32_t nowUs);

// Radial deadzone in percent of full deflection (default Config::STICK_DEADZONE_PERCENT). Movement is rescaled
// so the output starts from 0 right at the deadzone edge instead of jumping.
void setDeadzone(Stick stick, uint8_t percent);

// Measures the stick centers again, as at power-on (the sticks must be released). Blocks for ~12 ms, so only
// while the config menu is open (the host gets a neutral report then).
void recalibrate();

}  // namespace InputReader
