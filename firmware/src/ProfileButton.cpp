#include "ProfileButton.h"

#include "Config.h"

namespace ProfileButton {
namespace {

constexpr uint32_t PROFILE_BIT = buttonBit(Button::Profile);

// Held together with Profile, these jump straight to profile 1..4.
constexpr Button JUMP_BUTTONS[] = {Button::A, Button::B, Button::X, Button::Y};

bool wasHeld = false;
uint32_t pressStartMs = 0;
bool usedAsModifier = false;  // another button went down during this hold
bool menuOpened = false;
bool secondPress = false;     // this press started within the double-press window of a short press
bool shortPending = false;    // a short press waiting to see whether a second one follows
uint32_t shortReleaseMs = 0;
uint32_t suppressed = 0;      // pressed during a hold; kept from the host until released
uint32_t previousButtons = 0;

}  // namespace

void begin() {}

Event update(const InputState& input, uint32_t nowMs, uint32_t& suppressedButtons) {
  Event event;
  const uint32_t buttons = input.buttons;
  const bool held = buttons & PROFILE_BIT;
  const uint32_t newlyPressed = buttons & ~previousButtons & ~PROFILE_BIT;

  suppressed &= buttons;  // released buttons reach the host normally again

  if (held && !wasHeld) {
    pressStartMs = nowMs;
    usedAsModifier = false;
    menuOpened = false;
    secondPress = shortPending && nowMs - shortReleaseMs <= Config::PROFILE_DOUBLE_PRESS_MS;
    shortPending = false;
  }

  if (held) {
    if (newlyPressed != 0) {
      suppressed |= newlyPressed;
      usedAsModifier = true;
      for (uint8_t i = 0; i < sizeof(JUMP_BUTTONS) / sizeof(JUMP_BUTTONS[0]); ++i) {
        if (newlyPressed & buttonBit(JUMP_BUTTONS[i])) {
          event = {Gesture::JumpTo, i};
          break;
        }
      }
    }
    if (!usedAsModifier && !menuOpened && nowMs - pressStartMs >= Config::PROFILE_HOLD_MS) {
      menuOpened = true;
      event = {Gesture::OpenMenu, 0};
    }
  } else if (wasHeld && !usedAsModifier && !menuOpened) {
    if (secondPress) {
      event = {Gesture::Previous, 0};
    } else {
      shortPending = true;
      shortReleaseMs = nowMs;
    }
  }

  if (shortPending && !held && nowMs - shortReleaseMs > Config::PROFILE_DOUBLE_PRESS_MS) {
    shortPending = false;
    event = {Gesture::Next, 0};
  }

  wasHeld = held;
  previousButtons = buttons;
  suppressedButtons = suppressed | PROFILE_BIT;
  return event;
}

void reset() {
  // As if the current press (if any) had already produced its gesture: its release does nothing, and no button
  // counts as newly pressed on the next update.
  wasHeld = true;
  usedAsModifier = true;
  menuOpened = true;
  secondPress = false;
  shortPending = false;
  suppressed = 0;
  previousButtons = UINT32_MAX;
}

}  // namespace ProfileButton
