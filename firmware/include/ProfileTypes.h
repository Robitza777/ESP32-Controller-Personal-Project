#pragma once

// What a profile is: plain data shared by Profiles (runs it), Storage (keeps it) and the config menu (edits it).

#include "ControllerTypes.h"

// Stored with saved profiles. Bump it whenever the layout of Profile (or anything inside it) changes, and add a
// migration in Storage so the user's profiles survive the update.
constexpr uint16_t PROFILE_FORMAT_VERSION = 1;

constexpr uint8_t MAX_PROFILES = 16;
constexpr uint8_t PROFILE_NAME_MAX = 10;
constexpr uint8_t ACTIONS_PER_BUTTON = 3;
constexpr uint8_t LAYER_COUNT = 2;  // base layer + one layer active while a Layer button is held

// The Profile button is reserved for switching profiles and the menu, so it can't be mapped.
static_assert(static_cast<uint8_t>(Button::Profile) == BUTTON_COUNT - 1, "Profile must be the last Button");
constexpr uint8_t MAPPABLE_BUTTON_COUNT = BUTTON_COUNT - 1;

enum class ActionType : uint8_t {
  None,
  Xbox,      // code: XboxButton
  Dpad,      // code: DpadDirection
  Trigger,   // code: TriggerSide; fully pressed
  Key,       // code: HID keyboard usage 0x04..0xE7 (0xE0..0xE7 are Ctrl, Shift, Alt, Win)
  Consumer,  // code: HID consumer usage (media, launch, browser, editing, brightness)
  System,    // code: SystemKey
  Mouse,     // code: MouseButton
  Scroll,    // code: StickDirection (declared below); one step on press, then repeats while held
  Layer,     // while held, the second layer is active; a button with this action does nothing else
};

enum class TriggerSide : uint8_t { Left, Right };

struct Action {
  ActionType type = ActionType::None;
  uint16_t code = 0;
};

enum class ButtonMode : uint8_t {
  Normal,  // the actions follow the button
  Toggle,  // each press switches the actions on or off (on a Layer button: locks the layer)
  Turbo,   // while held, the actions pulse at Profile::turboHz
};

struct ButtonMapping {
  Action actions[ACTIONS_PER_BUTTON];  // all fire together, e.g. Ctrl + C
  ButtonMode mode = ButtonMode::Normal;
};

enum class StickMode : uint8_t {
  None,
  XboxLeft,
  XboxRight,
  Mouse,       // speed follows deflection through the mouse curve, scaled by sensitivity
  Scroll,      // vertical and horizontal wheel
  Directions,  // one action per direction (WASD, arrows, D-pad...); diagonals press two
};

enum class StickDirection : uint8_t { Up, Down, Left, Right, Count };
constexpr uint8_t STICK_DIRECTION_COUNT = static_cast<uint8_t>(StickDirection::Count);

struct StickMapping {
  StickMode mode = StickMode::None;
  uint8_t sensitivity = 5;                     // Mouse / Scroll speed, 1..10
  Action directions[STICK_DIRECTION_COUNT];  // Directions mode only
};

struct ProfileLayer {
  ButtonMapping buttons[MAPPABLE_BUTTON_COUNT];  // indexed by Button
  StickMapping sticks[STICK_COUNT];              // indexed by Stick
};

struct Profile {
  char name[PROFILE_NAME_MAX + 1] = {};
  uint8_t deadzonePercent[STICK_COUNT] = {};  // per physical stick, shared by both layers
  uint8_t turboHz = 10;
  ProfileLayer layers[LAYER_COUNT];
};
