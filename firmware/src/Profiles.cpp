#include "Profiles.h"

#include <Arduino.h>

#include <initializer_list>

#include "Config.h"
#include "HidUsages.h"

namespace Profiles {
namespace {

constexpr uint8_t PRESET_COUNT = static_cast<uint8_t>(Preset::Count);

// ---------------------------------------------------------------------------------------------
// Presets

Profile presets[PRESET_COUNT];

template <typename E>
constexpr uint16_t code(E value) {
  return static_cast<uint16_t>(value);
}

Action xbox(XboxButton b) { return {ActionType::Xbox, code(b)}; }
Action dpad(DpadDirection d) { return {ActionType::Dpad, code(d)}; }
Action trigger(TriggerSide side) { return {ActionType::Trigger, code(side)}; }
Action key(uint16_t usage) { return {ActionType::Key, usage}; }
Action consumer(uint16_t usage) { return {ActionType::Consumer, usage}; }
Action mouse(MouseButton b) { return {ActionType::Mouse, code(b)}; }
Action layerKey() { return {ActionType::Layer, 0}; }

void map(ProfileLayer& layer, Button button, std::initializer_list<Action> actions) {
  ButtonMapping& mapping = layer.buttons[static_cast<uint8_t>(button)];
  mapping = ButtonMapping{};
  uint8_t i = 0;
  for (const Action& action : actions) {
    if (i < ACTIONS_PER_BUTTON) mapping.actions[i++] = action;
  }
}

void mapStick(ProfileLayer& layer, Stick stick, StickMode mode) {
  StickMapping& mapping = layer.sticks[static_cast<uint8_t>(stick)];
  mapping = StickMapping{};
  mapping.mode = mode;
}

void mapStickDirections(ProfileLayer& layer, Stick stick, Action up, Action down, Action left, Action right) {
  mapStick(layer, stick, StickMode::Directions);
  Action* directions = layer.sticks[static_cast<uint8_t>(stick)].directions;
  directions[code(StickDirection::Up)] = up;
  directions[code(StickDirection::Down)] = down;
  directions[code(StickDirection::Left)] = left;
  directions[code(StickDirection::Right)] = right;
}

void startProfile(Profile& profile, const char* name) {
  profile = Profile{};
  snprintf(profile.name, sizeof(profile.name), "%s", name);
  for (uint8_t& deadzone : profile.deadzonePercent) deadzone = Config::STICK_DEADZONE_PERCENT;
  profile.turboHz = Config::TURBO_DEFAULT_HZ;
}

void buildGamepadLayer(ProfileLayer& l) {
  map(l, Button::A, {xbox(XboxButton::A)});
  map(l, Button::B, {xbox(XboxButton::B)});
  map(l, Button::X, {xbox(XboxButton::X)});
  map(l, Button::Y, {xbox(XboxButton::Y)});
  map(l, Button::DpadUp, {dpad(DpadDirection::Up)});
  map(l, Button::DpadDown, {dpad(DpadDirection::Down)});
  map(l, Button::DpadLeft, {dpad(DpadDirection::Left)});
  map(l, Button::DpadRight, {dpad(DpadDirection::Right)});
  map(l, Button::L1, {xbox(XboxButton::LB)});
  map(l, Button::R1, {xbox(XboxButton::RB)});
  map(l, Button::L2, {trigger(TriggerSide::Left)});
  map(l, Button::R2, {trigger(TriggerSide::Right)});
  map(l, Button::L3, {xbox(XboxButton::LS)});
  map(l, Button::R3, {xbox(XboxButton::RS)});
  map(l, Button::L4, {xbox(XboxButton::View)});  // the controller has only one middle button (Options)
  map(l, Button::R4, {xbox(XboxButton::Guide)});
  map(l, Button::Options, {xbox(XboxButton::Menu)});
  mapStick(l, Stick::Left, StickMode::XboxLeft);
  mapStick(l, Stick::Right, StickMode::XboxRight);
}

// PC games without controller support, FPS-style.
void buildKeyboardMouseLayer(ProfileLayer& l) {
  map(l, Button::A, {key(HidKey::SPACE)});
  map(l, Button::B, {key(HidKey::LEFT_CTRL)});
  map(l, Button::X, {key(HidKey::R)});
  map(l, Button::Y, {key(HidKey::E)});
  map(l, Button::DpadUp, {key(HidKey::DIGIT_1)});
  map(l, Button::DpadRight, {key(HidKey::DIGIT_2)});
  map(l, Button::DpadDown, {key(HidKey::DIGIT_3)});
  map(l, Button::DpadLeft, {key(HidKey::DIGIT_4)});
  map(l, Button::L1, {key(HidKey::Q)});
  map(l, Button::R1, {key(HidKey::F)});
  map(l, Button::L2, {mouse(MouseButton::Right)});
  map(l, Button::R2, {mouse(MouseButton::Left)});
  map(l, Button::L3, {key(HidKey::LEFT_SHIFT)});
  map(l, Button::R3, {key(HidKey::V)});
  map(l, Button::L4, {key(HidKey::TAB)});
  map(l, Button::R4, {key(HidKey::M)});
  map(l, Button::Options, {key(HidKey::ESCAPE)});
  mapStickDirections(l, Stick::Left, key(HidKey::W), key(HidKey::S), key(HidKey::A), key(HidKey::D));
  mapStick(l, Stick::Right, StickMode::Mouse);
}

void buildMediaLayer(ProfileLayer& l) {
  map(l, Button::A, {mouse(MouseButton::Left)});
  map(l, Button::B, {mouse(MouseButton::Right)});
  map(l, Button::X, {mouse(MouseButton::Middle)});
  map(l, Button::Y, {key(HidKey::ENTER)});
  map(l, Button::DpadUp, {consumer(HidConsumer::VOLUME_UP)});
  map(l, Button::DpadDown, {consumer(HidConsumer::VOLUME_DOWN)});
  map(l, Button::DpadLeft, {consumer(HidConsumer::PREVIOUS_TRACK)});
  map(l, Button::DpadRight, {consumer(HidConsumer::NEXT_TRACK)});
  map(l, Button::L1, {mouse(MouseButton::Back)});
  map(l, Button::R1, {mouse(MouseButton::Forward)});
  map(l, Button::L2, {key(HidKey::LEFT_ALT), key(HidKey::TAB)});
  map(l, Button::R2, {key(HidKey::LEFT_GUI), key(HidKey::D)});
  map(l, Button::L3, {consumer(HidConsumer::MUTE)});
  map(l, Button::R3, {key(HidKey::ESCAPE)});
  map(l, Button::L4, {key(HidKey::LEFT_CTRL), key(HidKey::C)});
  map(l, Button::R4, {key(HidKey::LEFT_CTRL), key(HidKey::V)});
  map(l, Button::Options, {consumer(HidConsumer::PLAY_PAUSE)});
  mapStick(l, Stick::Left, StickMode::Mouse);
  mapStick(l, Stick::Right, StickMode::Scroll);
}

// Hybrid's second layer, active while R4 is held: operate the PC without leaving the game profile.
// Everything not listed is None (disabled) while the layer is active.
void buildPcLayer(ProfileLayer& l) {
  l = ProfileLayer{};
  map(l, Button::A, {key(HidKey::ENTER)});
  map(l, Button::B, {key(HidKey::ESCAPE)});
  map(l, Button::DpadUp, {consumer(HidConsumer::VOLUME_UP)});
  map(l, Button::DpadDown, {consumer(HidConsumer::VOLUME_DOWN)});
  map(l, Button::DpadLeft, {consumer(HidConsumer::PREVIOUS_TRACK)});
  map(l, Button::DpadRight, {consumer(HidConsumer::NEXT_TRACK)});
  map(l, Button::L2, {mouse(MouseButton::Right)});
  map(l, Button::R2, {mouse(MouseButton::Left)});
  map(l, Button::Options, {consumer(HidConsumer::PLAY_PAUSE)});
  mapStick(l, Stick::Left, StickMode::Scroll);
  mapStick(l, Stick::Right, StickMode::Mouse);
}

void buildPresets() {
  Profile& gamepad = presets[code(Preset::Gamepad)];
  startProfile(gamepad, "GAMEPAD");
  buildGamepadLayer(gamepad.layers[0]);
  gamepad.layers[1] = gamepad.layers[0];

  Profile& keyboardMouse = presets[code(Preset::KeyboardMouse)];
  startProfile(keyboardMouse, "KB+MOUSE");
  buildKeyboardMouseLayer(keyboardMouse.layers[0]);
  keyboardMouse.layers[1] = keyboardMouse.layers[0];

  Profile& media = presets[code(Preset::Media)];
  startProfile(media, "MEDIA");
  buildMediaLayer(media.layers[0]);
  media.layers[1] = media.layers[0];

  Profile& hybrid = presets[code(Preset::Hybrid)];
  startProfile(hybrid, "HYBRID");
  buildGamepadLayer(hybrid.layers[0]);
  map(hybrid.layers[0], Button::R4, {layerKey()});
  buildPcLayer(hybrid.layers[1]);
}

// ---------------------------------------------------------------------------------------------
// Profile list

Profile profiles[MAX_PROFILES];
uint8_t profileCount = 0;
uint8_t activeIdx = 0;
uint8_t previousIdx = 0;

// ---------------------------------------------------------------------------------------------
// Engine state

// Anything that fires actions: a button, or one direction of a stick.
struct Source {
  bool wasActive = false;
  uint32_t nextScrollUs = 0;
};

struct ButtonRuntime {
  bool wasPressed = false;
  bool blocked = false;    // held across a profile switch: ignored until released
  bool toggledOn = false;  // Toggle mode latch
  uint8_t layer = 0;       // the layer whose mapping this press uses, so a layer change can't strand a key
  uint32_t pressUs = 0;    // turbo phase reference
  Source source;
};

ButtonRuntime buttonRuntime[MAPPABLE_BUTTON_COUNT];
bool directionOn[STICK_COUNT][STICK_DIRECTION_COUNT];
Source directionSource[STICK_COUNT][STICK_DIRECTION_COUNT];
bool resyncPending = true;
uint8_t lastLayer = 0;

void resetRuntime() {
  for (ButtonRuntime& runtime : buttonRuntime) runtime = ButtonRuntime{};
  for (uint8_t s = 0; s < STICK_COUNT; ++s) {
    for (uint8_t d = 0; d < STICK_DIRECTION_COUNT; ++d) {
      directionOn[s][d] = false;
      directionSource[s][d] = Source{};
    }
  }
  resyncPending = true;
  lastLayer = 0;
}

bool isLayerButton(const ButtonMapping& mapping) {
  for (const Action& action : mapping.actions) {
    if (action.type == ActionType::Layer) return true;
  }
  return false;
}

// ---------------------------------------------------------------------------------------------
// Emitting actions into the report

void addKey(KeyboardReport& keyboard, uint16_t usage) {
  if (usage >= HidKey::FIRST_MODIFIER && usage <= HidKey::LAST_MODIFIER) {
    keyboard.modifiers |= 1u << (usage - HidKey::FIRST_MODIFIER);
    return;
  }
  for (uint8_t& slot : keyboard.keys) {
    if (slot == usage) return;
    if (slot == 0) {
      slot = static_cast<uint8_t>(usage);
      return;
    }
  }
  // More than MAX_KEYS keys at once: the extra ones are dropped.
}

void addConsumer(OutputReport& out, uint16_t usage) {
  for (uint16_t& slot : out.consumer) {
    if (slot == usage) return;
    if (slot == 0) {
      slot = usage;
      return;
    }
  }
}

void addScrollStep(MouseReport& mouseReport, uint16_t direction) {
  switch (static_cast<StickDirection>(direction)) {
    case StickDirection::Up: mouseReport.wheel += 1.0f; break;
    case StickDirection::Down: mouseReport.wheel -= 1.0f; break;
    case StickDirection::Left: mouseReport.pan -= 1.0f; break;
    case StickDirection::Right: mouseReport.pan += 1.0f; break;
    default: break;
  }
}

void emitAction(const Action& action, bool scrollStep, OutputReport& out) {
  switch (action.type) {
    case ActionType::Xbox:
      if (action.code < code(XboxButton::Count)) out.gamepad.buttons |= 1u << action.code;
      break;
    case ActionType::Dpad:
      if (action.code < code(DpadDirection::Count)) out.gamepad.dpad |= 1u << action.code;
      break;
    case ActionType::Trigger:
      if (action.code == code(TriggerSide::Left)) out.gamepad.leftTrigger = TRIGGER_MAX;
      else out.gamepad.rightTrigger = TRIGGER_MAX;
      break;
    case ActionType::Key: addKey(out.keyboard, action.code); break;
    case ActionType::Consumer: addConsumer(out, action.code); break;
    case ActionType::System:
      if (action.code < code(SystemKey::Count)) out.system |= 1u << action.code;
      break;
    case ActionType::Mouse:
      if (action.code < code(MouseButton::Count)) out.mouse.buttons |= 1u << action.code;
      break;
    case ActionType::Scroll:
      if (scrollStep) addScrollStep(out.mouse, action.code);
      break;
    case ActionType::None:
    case ActionType::Layer: break;
  }
}

// Scroll actions step once when the source turns on, then repeat after a delay, like a held key.
void emitSource(const Action* actions, uint8_t actionCount, bool active, Source& source, uint32_t nowUs,
                OutputReport& out) {
  bool scrollStep = false;
  if (active && !source.wasActive) {
    scrollStep = true;
    source.nextScrollUs = nowUs + Config::SCROLL_REPEAT_DELAY_MS * 1000;
  } else if (active && static_cast<int32_t>(nowUs - source.nextScrollUs) >= 0) {
    scrollStep = true;
    source.nextScrollUs += Config::SCROLL_REPEAT_INTERVAL_MS * 1000;
  }
  source.wasActive = active;
  if (!active) return;

  for (uint8_t i = 0; i < actionCount; ++i) emitAction(actions[i], scrollStep, out);
}

// ---------------------------------------------------------------------------------------------
// Buttons

bool turboPhaseOn(uint32_t heldUs, uint8_t turboHz) {
  const uint8_t hz = constrain(turboHz, Config::TURBO_MIN_HZ, Config::TURBO_MAX_HZ);
  const uint64_t halfPeriods = static_cast<uint64_t>(heldUs) * 2 * hz / 1000000;
  return halfPeriods % 2 == 0;
}

// Layer buttons live in the base layer. Returns the layer the other buttons should use this cycle.
uint8_t updateLayer(const Profile& profile, uint32_t pressedMask) {
  bool layerOn = false;
  for (uint8_t i = 0; i < MAPPABLE_BUTTON_COUNT; ++i) {
    const ButtonMapping& mapping = profile.layers[0].buttons[i];
    if (!isLayerButton(mapping)) continue;
    ButtonRuntime& runtime = buttonRuntime[i];
    const bool pressed = pressedMask & (1u << i);
    if (mapping.mode == ButtonMode::Toggle) {
      if (pressed && !runtime.wasPressed) runtime.toggledOn = !runtime.toggledOn;
      layerOn |= runtime.toggledOn;
    } else {
      layerOn |= pressed;
    }
  }
  return layerOn ? 1 : 0;
}

void applyButtons(const Profile& profile, uint32_t pressedMask, uint8_t currentLayer, uint32_t nowUs,
                  OutputReport& out) {
  for (uint8_t i = 0; i < MAPPABLE_BUTTON_COUNT; ++i) {
    ButtonRuntime& runtime = buttonRuntime[i];
    const bool pressed = pressedMask & (1u << i);

    if (!isLayerButton(profile.layers[0].buttons[i])) {
      if (pressed && !runtime.wasPressed) {
        if (runtime.toggledOn) {
          runtime.toggledOn = false;  // the second press releases a latched button, whatever the layer is now
        } else {
          runtime.layer = currentLayer;
          runtime.pressUs = nowUs;
          runtime.toggledOn = profile.layers[currentLayer].buttons[i].mode == ButtonMode::Toggle;
        }
      }

      const ButtonMapping& mapping = profile.layers[runtime.layer].buttons[i];
      bool active = false;
      switch (mapping.mode) {
        case ButtonMode::Normal: active = pressed; break;
        case ButtonMode::Toggle: active = runtime.toggledOn; break;
        case ButtonMode::Turbo: active = pressed && turboPhaseOn(nowUs - runtime.pressUs, profile.turboHz); break;
      }
      emitSource(mapping.actions, ACTIONS_PER_BUTTON, active, runtime.source, nowUs, out);
    }
    runtime.wasPressed = pressed;
  }
}

// ---------------------------------------------------------------------------------------------
// Sticks

int16_t addAxis(int16_t a, int16_t b) { return static_cast<int16_t>(constrain(int32_t{a} + b, -AXIS_MAX, AXIS_MAX)); }

float sensitivityOf(const StickMapping& mapping) {
  return constrain(mapping.sensitivity, Config::SENSITIVITY_MIN, Config::SENSITIVITY_MAX);
}

void applyStick(Stick stick, const StickMapping& mapping, const InputState& input, float dtS, uint32_t nowUs,
                OutputReport& out) {
  const uint8_t s = static_cast<uint8_t>(stick);
  const int16_t rawX = input.axis(stick == Stick::Left ? Axis::LeftX : Axis::RightX);
  const int16_t rawY = input.axis(stick == Stick::Left ? Axis::LeftY : Axis::RightY);
  const float x = static_cast<float>(rawX) / AXIS_MAX;
  const float y = static_cast<float>(rawY) / AXIS_MAX;
  const float magnitude = sqrtf(x * x + y * y);

  switch (mapping.mode) {
    case StickMode::XboxLeft:
      out.gamepad.leftX = addAxis(out.gamepad.leftX, rawX);
      out.gamepad.leftY = addAxis(out.gamepad.leftY, rawY);
      break;
    case StickMode::XboxRight:
      out.gamepad.rightX = addAxis(out.gamepad.rightX, rawX);
      out.gamepad.rightY = addAxis(out.gamepad.rightY, rawY);
      break;
    case StickMode::Mouse:
      if (magnitude > 0.0f) {
        // The curve works on the deflection radius, so speed doesn't depend on the direction.
        const float countsPerS =
            Config::MOUSE_COUNTS_PER_S_PER_SENSITIVITY * sensitivityOf(mapping) * powf(magnitude, Config::MOUSE_CURVE_EXPONENT);
        out.mouse.moveX += x / magnitude * countsPerS * dtS;
        out.mouse.moveY += y / magnitude * countsPerS * dtS;
      }
      break;
    case StickMode::Scroll:
      if (magnitude > 0.0f) {
        const float stepsPerS = Config::SCROLL_STEPS_PER_S_PER_SENSITIVITY * sensitivityOf(mapping) *
                                powf(magnitude, Config::MOUSE_CURVE_EXPONENT);
        out.mouse.wheel += y / magnitude * stepsPerS * dtS;
        out.mouse.pan += x / magnitude * stepsPerS * dtS;
      }
      break;
    case StickMode::Directions:
    case StickMode::None: break;
  }

  // Directions: press past STICK_DIRECTION_PRESS, release under STICK_DIRECTION_RELEASE.
  const float lean[STICK_DIRECTION_COUNT] = {y, -y, -x, x};  // indexed by StickDirection
  for (uint8_t d = 0; d < STICK_DIRECTION_COUNT; ++d) {
    bool& on = directionOn[s][d];
    if (mapping.mode != StickMode::Directions) on = false;
    else if (!on && lean[d] >= Config::STICK_DIRECTION_PRESS) on = true;
    else if (on && lean[d] < Config::STICK_DIRECTION_RELEASE) on = false;
    emitSource(&mapping.directions[d], 1, on, directionSource[s][d], nowUs, out);
  }
}

}  // namespace

const Profile& preset(Preset p) { return presets[code(p)]; }

void begin(LoadProfiles load, uint8_t savedActive) {
  buildPresets();
  profileCount = load != nullptr ? load(profiles, MAX_PROFILES) : 0;
  if (profileCount == 0) {
    for (const Profile& p : presets) profiles[profileCount++] = p;
  }
  activeIdx = previousIdx = savedActive < profileCount ? savedActive : 0;
  resetRuntime();
}

uint8_t count() { return profileCount; }
const Profile& get(uint8_t index) { return profiles[index < profileCount ? index : 0]; }
uint8_t activeIndex() { return activeIdx; }
const Profile& active() { return profiles[activeIdx]; }

void select(uint8_t index) {
  if (index >= profileCount || index == activeIdx) return;
  previousIdx = activeIdx;
  activeIdx = index;
  resetRuntime();
}

void selectNext() { select((activeIdx + 1) % profileCount); }

void replaceAll(const Profile* list, uint8_t count, uint8_t active) {
  if (count > 0 && count <= MAX_PROFILES) {
    memcpy(profiles, list, count * sizeof(Profile));
    profileCount = count;
    // Positions may have moved, so the old "previous profile" means nothing now.
    activeIdx = previousIdx = active < count ? active : 0;
  }
  resetRuntime();
}

void resync() { resetRuntime(); }

void selectPrevious() { select(previousIdx); }

bool layerActive() { return lastLayer != 0; }

bool toggleActive() {
  const ProfileLayer& base = profiles[activeIdx].layers[0];
  for (uint8_t i = 0; i < MAPPABLE_BUTTON_COUNT; ++i) {
    if (buttonRuntime[i].toggledOn && !isLayerButton(base.buttons[i])) return true;
  }
  return false;
}

OutputReport apply(const InputState& input, uint32_t nowUs, float dtS) {
  // Buttons already held when the profile changed stay silent until released, so a switch never fires
  // the new profile's actions by itself.
  if (resyncPending) {
    for (uint8_t i = 0; i < MAPPABLE_BUTTON_COUNT; ++i) {
      const bool pressed = input.isPressed(static_cast<Button>(i));
      buttonRuntime[i].blocked = pressed;
      buttonRuntime[i].wasPressed = pressed;
    }
    resyncPending = false;
  }

  uint32_t pressedMask = 0;
  for (uint8_t i = 0; i < MAPPABLE_BUTTON_COUNT; ++i) {
    ButtonRuntime& runtime = buttonRuntime[i];
    const bool pressed = input.isPressed(static_cast<Button>(i));
    if (runtime.blocked && !pressed) runtime.blocked = false;
    if (pressed && !runtime.blocked) pressedMask |= 1u << i;
  }

  const Profile& profile = profiles[activeIdx];
  OutputReport out;
  const uint8_t currentLayer = updateLayer(profile, pressedMask);
  lastLayer = currentLayer;
  applyButtons(profile, pressedMask, currentLayer, nowUs, out);
  for (uint8_t s = 0; s < STICK_COUNT; ++s) {
    applyStick(static_cast<Stick>(s), profile.layers[currentLayer].sticks[s], input, dtS, nowUs, out);
  }
  return out;
}

}  // namespace Profiles
