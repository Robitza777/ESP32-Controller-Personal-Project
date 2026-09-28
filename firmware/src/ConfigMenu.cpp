#include "ConfigMenu.h"

#include <Arduino.h>

#include "ActionCatalog.h"
#include "Config.h"
#include "Log.h"
#include "Profiles.h"

namespace ConfigMenu {
namespace {

using Row = MenuView::Row;
using Text = char[MenuView::LINE_CHARS + 1];
constexpr uint8_t ROWS = MenuView::VISIBLE_ROWS;

constexpr uint32_t PROFILE_BIT = buttonBit(Button::Profile);
constexpr uint32_t MAPPABLE_MASK = (1u << MAPPABLE_BUTTON_COUNT) - 1;
constexpr uint8_t PRESET_COUNT = static_cast<uint8_t>(Profiles::Preset::Count);

// Indexed by Button; they stand alone in a page title.
constexpr const char* BUTTON_TITLES[MAPPABLE_BUTTON_COUNT] = {
  "A", "B", "X", "Y", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right",
  "L1", "L2", "L3", "L4", "R1", "R2", "R3", "R4", "Options",
};
constexpr const char* STICK_TITLES[STICK_COUNT] = {"LEFT STICK", "RIGHT STICK"};
constexpr const char* STICK_ROW_NAMES[STICK_COUNT] = {"Left stick", "Right stick"};
constexpr const char* DIRECTION_NAMES[STICK_DIRECTION_COUNT] = {"Up", "Down", "Left", "Right"};
constexpr const char* LAYER_NAMES[LAYER_COUNT] = {"Base", "RTZ"};
constexpr const char* BUTTON_MODE_NAMES[] = {"Normal", "Toggle", "Turbo"};
constexpr uint8_t BUTTON_MODE_COUNT = sizeof(BUTTON_MODE_NAMES) / sizeof(BUTTON_MODE_NAMES[0]);
// "4-way": each direction fires its own action (WASD, arrows, D-pad...).
constexpr const char* STICK_MODE_NAMES[] = {"None", "Xbox L", "Xbox R", "Mouse", "Scroll", "4-way"};
constexpr uint8_t STICK_MODE_COUNT = sizeof(STICK_MODE_NAMES) / sizeof(STICK_MODE_NAMES[0]);
static_assert(STICK_MODE_COUNT == static_cast<uint8_t>(StickMode::Directions) + 1, "one name per StickMode");

// Name entry: Up / Down step through this list, L1 / R1 jump between its groups (space, A-Z, a-z, 0-9, symbols).
constexpr char CHARSET[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.+!#&()*/:=?@'";
constexpr uint8_t CHARSET_SIZE = sizeof(CHARSET) - 1;
constexpr uint8_t CHARSET_GROUPS[] = {0, 1, 27, 53, 63};
constexpr uint8_t CHARSET_GROUP_COUNT = sizeof(CHARSET_GROUPS);
static_assert(CHARSET[1] == 'A' && CHARSET[27] == 'a' && CHARSET[53] == '0' && CHARSET[63] == '-',
              "group starts must match CHARSET");

// ---------------------------------------------------------------------------------------------
// Pages. Each open page is a frame on a stack; B (or a short Profile press) pops back to the one below.

enum class Page : uint8_t {
  Main,
  Edit,            // the profile's settings
  PressButton,     // "press the button to edit"
  Button,          // one button: layer, 3 actions, mode
  Stick,           // one stick: layer, mode, speed or 4 directions
  Picker,          // one level of the action catalog
  NameEntry,       // rename, or name a new profile
  NewProfile,      // pick what the new profile starts from
  Profiles,        // the list
  ProfileActions,  // one profile: edit, move, delete
  Controller,      // display, auto off, sticks, Bluetooth, reset, about
  DisplayOptions,  // brightness, screen off, name popup
  AutoOff,         // minutes without activity, connected / offline
  Recalibrate,     // "let go of the sticks"
  Confirm,         // yes / no, see Question
  Info,            // a message, see Notice
  SavePrompt,
};

enum class Question : uint8_t { CopyToRtz, DeleteProfile, ForgetPairings, ResetProfiles };
enum class Notice : uint8_t { PairingsErased, About, ProfilesFull, LastProfile, SticksRecalibrated };

struct Frame {
  Page page = Page::Main;
  uint8_t cursor = 0;
  uint8_t first = 0;  // first visible item
  uint8_t kind = 0;   // Confirm: Question, Info: Notice
  const ActionCatalog::Category* category = nullptr;  // Picker only
};

// Deepest path: Main > Profiles > ProfileActions > Edit > PressButton > Button > Picker x3.
constexpr uint8_t STACK_MAX = 10;
Frame stack[STACK_MAX];
uint8_t depth = 0;

Workspace* list = nullptr;
MenuView menuView;
Request pending = Request::None;

uint8_t editIdx = 0;          // the profile the Edit page works on
uint8_t selectedProfile = 0;  // the profile ProfileActions works on
uint8_t editLayer = 0;        // Base / RTZ, shared by the button and stick pages so it sticks while you work
uint8_t editButton = 0;
uint8_t editStick = 0;
Action* pickTarget = nullptr;  // what the picker writes into
Text pickTitle = {};

bool namingNewProfile = false;              // NameEntry: false = rename profiles[editIdx]
char nameBuffer[PROFILE_NAME_MAX + 1] = {};  // always PROFILE_NAME_MAX characters, padded with spaces
uint8_t nameCursor = 0;
Profile newProfile;  // the start of a new profile while it's being named

Frame& top() { return stack[depth - 1]; }

Frame* below() { return depth >= 2 ? &stack[depth - 2] : nullptr; }

void push(Page page, uint8_t cursor = 0, const ActionCatalog::Category* category = nullptr, uint8_t kind = 0) {
  if (depth >= STACK_MAX) return;
  stack[depth++] = Frame{page, cursor, 0, kind, category};
}

void pop() {
  if (depth > 1) --depth;
}

void confirm(Question question) { push(Page::Confirm, 0, nullptr, static_cast<uint8_t>(question)); }
void notify(Notice notice) { push(Page::Info, 0, nullptr, static_cast<uint8_t>(notice)); }

Profile& profile() { return list->profiles[editIdx]; }
ProfileLayer& layer() { return profile().layers[editLayer]; }
ButtonMapping& buttonMapping() { return layer().buttons[editButton]; }
StickMapping& stickMapping() { return layer().sticks[editStick]; }

uint16_t slotBit(uint8_t index) { return static_cast<uint16_t>(1u << index); }
void markChanged() { list->changedSlots |= slotBit(editIdx); }
bool hasChanges() {
  return list->changedSlots != 0 || list->countChanged || list->displayChanged || list->powerChanged;
}

// ---------------------------------------------------------------------------------------------
// Profile list operations (on the working copy)

void swapProfiles(uint8_t a, uint8_t b) {
  const Profile temp = list->profiles[a];
  list->profiles[a] = list->profiles[b];
  list->profiles[b] = temp;
  list->changedSlots |= slotBit(a) | slotBit(b);
  if (list->active == a) list->active = b;
  else if (list->active == b) list->active = a;
}

void deleteProfile(uint8_t index) {
  for (uint8_t i = index; i + 1 < list->count; ++i) list->profiles[i] = list->profiles[i + 1];
  --list->count;
  list->countChanged = true;
  for (uint8_t i = index; i < list->count; ++i) list->changedSlots |= slotBit(i);
  list->changedSlots &= static_cast<uint16_t>((1u << list->count) - 1);  // slots past the end are never written
  if (list->active > index) --list->active;
  else if (list->active >= list->count) list->active = list->count - 1;
  editIdx = list->active;
}

void resetToPresets() {
  list->count = PRESET_COUNT;
  for (uint8_t i = 0; i < PRESET_COUNT; ++i) {
    list->profiles[i] = Profiles::preset(static_cast<Profiles::Preset>(i));
  }
  list->changedSlots = static_cast<uint16_t>((1u << PRESET_COUNT) - 1);
  list->countChanged = true;
  list->active = 0;
  editIdx = 0;
}

// ---------------------------------------------------------------------------------------------
// Input: edges, holds, repeats

enum Direction : uint8_t { Up, Down, Left, Right, DIRECTION_COUNT };
constexpr Button DPAD_BUTTONS[DIRECTION_COUNT] = {Button::DpadUp, Button::DpadDown, Button::DpadLeft,
                                                  Button::DpadRight};

struct Repeat {
  bool started = false;  // a press was seen; repeats run until release
  uint32_t nextMs = 0;
};

Repeat repeats[DIRECTION_COUNT];
int8_t stickDirection = -1;  // the direction the left stick points, with hysteresis; -1 = centered
float stickMagnitude = 0.0f;
bool stickBlocked = false;   // held since the screen woke up: ignored until it returns to center
uint32_t blocked = 0;        // buttons held since the menu opened / the screen woke up: ignored until released
bool blockOnNextUpdate = false;

bool profileDown = false;
bool profileHoldFired = false;
uint32_t profileDownMs = 0;

// Direction the stick points (dominant axis only, the menu has no diagonals), with the same hysteresis as a stick in
// 4-way mode so it can't chatter at the threshold.
void readStick(const Input& in) {
  const float x = in.leftX / static_cast<float>(AXIS_MAX);
  const float y = in.leftY / static_cast<float>(AXIS_MAX);
  const float components[DIRECTION_COUNT] = {y, -y, -x, x};
  if (stickDirection >= 0 && components[stickDirection] >= Config::STICK_DIRECTION_RELEASE) {
    stickMagnitude = components[stickDirection];
    return;
  }
  stickDirection = -1;
  stickMagnitude = 0.0f;
  const bool vertical = fabsf(y) >= fabsf(x);
  const float value = vertical ? y : x;
  if (fabsf(value) >= Config::MENU_STICK_PRESS) {
    stickDirection = vertical ? (value > 0 ? Up : Down) : (value > 0 ? Right : Left);
    stickMagnitude = fabsf(value);
  }
}

// Keyboard-style repeat: once on the press, again after MENU_REPEAT_DELAY_MS, then every interval while held.
// `initial` tells a fresh press from a repeat (only a fresh press wraps around the end of a list).
bool fires(Repeat& repeat, bool edge, bool held, uint32_t intervalMs, uint32_t nowMs, bool& initial) {
  if (edge) {
    repeat.started = true;
    repeat.nextMs = nowMs + Config::MENU_REPEAT_DELAY_MS;
    initial = true;
    return true;
  }
  if (!held) {
    repeat.started = false;
    return false;
  }
  if (repeat.started && static_cast<int32_t>(nowMs - repeat.nextMs) >= 0) {
    repeat.nextMs = nowMs + intervalMs;
    initial = false;
    return true;
  }
  return false;
}

// The further the stick is pushed, the faster the cursor repeats.
uint32_t stickInterval() {
  const float t = constrain((stickMagnitude - Config::MENU_STICK_PRESS) / (1.0f - Config::MENU_STICK_PRESS), 0.0f, 1.0f);
  return Config::MENU_REPEAT_INTERVAL_MS - static_cast<uint32_t>(t * (Config::MENU_REPEAT_INTERVAL_MS - Config::MENU_STICK_FASTEST_MS));
}

// Which directions (D-pad or left stick) fire this update; `initial` gets a bit per fresh press.
uint8_t firedDirections(uint32_t edges, uint32_t held, int8_t stick, uint32_t nowMs, uint8_t& initial) {
  uint8_t fired = 0;
  initial = 0;
  for (uint8_t d = 0; d < DIRECTION_COUNT; ++d) {
    const uint32_t bit = buttonBit(DPAD_BUTTONS[d]);
    const bool viaStick = stick == d;
    const bool edge = (edges & bit) != 0 || (viaStick && !repeats[d].started);
    const bool isHeld = (held & bit) != 0 || viaStick;
    const uint32_t interval = viaStick ? stickInterval() : Config::MENU_REPEAT_INTERVAL_MS;
    bool fresh = false;
    if (fires(repeats[d], edge, isHeld, interval, nowMs, fresh)) {
      fired |= 1u << d;
      if (fresh) initial |= 1u << d;
    }
  }
  return fired;
}

void resetInputState() {
  for (Repeat& repeat : repeats) repeat = Repeat{};
  profileDown = false;
  profileHoldFired = false;
}

// ---------------------------------------------------------------------------------------------
// Row text

template <typename... Args>
void format(Text& out, const char* fmt, Args... args) {
  snprintf(out, sizeof(out), fmt, args...);
}

void upperCase(char* text) {
  for (char* c = text; *c != '\0'; ++c) *c = static_cast<char>(toupper(*c));
}

void setRow(Row& row, const char* label, const char* value = "", bool opens = false) {
  format(row.label, "%s", label);
  format(row.value, "%s", value);
  row.opens = opens;
}

// "< value >": Left / Right change it.
void setAdjustRow(Row& row, const char* label, const char* value) {
  format(row.label, "%s", label);
  format(row.value, "< %s >", value);
  row.opens = false;
}

void setTitle(const char* text) { format(menuView.title, "%s", text); }

// ---------------------------------------------------------------------------------------------
// Name entry

uint8_t charIndex(char c) {
  for (uint8_t i = 0; i < CHARSET_SIZE; ++i) {
    if (CHARSET[i] == c) return i;
  }
  return 0;  // unknown characters (only possible from an older firmware) count as a space
}

void openNameEntry(const char* startName, bool forNewProfile) {
  namingNewProfile = forNewProfile;
  memset(nameBuffer, ' ', PROFILE_NAME_MAX);
  nameBuffer[PROFILE_NAME_MAX] = '\0';
  const size_t length = strnlen(startName, PROFILE_NAME_MAX);
  memcpy(nameBuffer, startName, length);
  nameCursor = static_cast<uint8_t>(length < PROFILE_NAME_MAX ? length : PROFILE_NAME_MAX - 1);
  push(Page::NameEntry);
}

void stepChar(int delta) {
  const uint8_t index = charIndex(nameBuffer[nameCursor]);
  nameBuffer[nameCursor] = CHARSET[(index + CHARSET_SIZE + delta) % CHARSET_SIZE];
}

// R1: start of the next group; L1: start of this group, or of the previous one when already there.
void jumpGroup(bool forward) {
  const uint8_t index = charIndex(nameBuffer[nameCursor]);
  uint8_t target = forward ? CHARSET_GROUPS[0] : CHARSET_GROUPS[CHARSET_GROUP_COUNT - 1];
  if (forward) {
    for (uint8_t start : CHARSET_GROUPS) {
      if (start > index) {
        target = start;
        break;
      }
    }
  } else {
    for (uint8_t start : CHARSET_GROUPS) {
      if (start < index) target = start;
    }
  }
  nameBuffer[nameCursor] = CHARSET[target];
}

void acceptName() {
  char name[PROFILE_NAME_MAX + 1];
  memcpy(name, nameBuffer, sizeof(name));
  for (int i = PROFILE_NAME_MAX - 1; i >= 0 && name[i] == ' '; --i) name[i] = '\0';
  if (name[0] == '\0') snprintf(name, sizeof(name), "PROFILE");  // an empty name would leave a blank screen

  if (!namingNewProfile) {
    memcpy(profile().name, name, sizeof(name));
    markChanged();
    pop();
    return;
  }
  const uint8_t index = list->count++;
  list->profiles[index] = newProfile;
  memcpy(list->profiles[index].name, name, sizeof(name));
  list->changedSlots |= slotBit(index);
  list->countChanged = true;
  list->active = index;  // a new profile is the one you want to try next
  editIdx = index;
  depth = 1;             // straight on to editing it: Main > Edit
  push(Page::Edit);
}

void nameEntryInput(uint32_t edges, uint32_t held, int8_t stick, uint32_t nowMs) {
  if (edges & buttonBit(Button::A)) {
    acceptName();
    return;
  }
  if (edges & buttonBit(Button::B)) {
    pop();
    return;
  }
  if (edges & buttonBit(Button::X)) {  // delete: the rest moves left
    memmove(&nameBuffer[nameCursor], &nameBuffer[nameCursor + 1], PROFILE_NAME_MAX - nameCursor - 1);
    nameBuffer[PROFILE_NAME_MAX - 1] = ' ';
  }
  if (edges & buttonBit(Button::Y)) {  // insert a space: the rest moves right, the last character drops off
    memmove(&nameBuffer[nameCursor + 1], &nameBuffer[nameCursor], PROFILE_NAME_MAX - nameCursor - 1);
    nameBuffer[nameCursor] = ' ';
  }
  if (edges & buttonBit(Button::L1)) jumpGroup(false);
  if (edges & buttonBit(Button::R1)) jumpGroup(true);

  uint8_t initial = 0;
  const uint8_t fired = firedDirections(edges, held, stick, nowMs, initial);
  if (fired & (1u << Up)) stepChar(1);
  if (fired & (1u << Down)) stepChar(-1);
  if ((fired & (1u << Left)) && nameCursor > 0) --nameCursor;
  if ((fired & (1u << Right)) && nameCursor < PROFILE_NAME_MAX - 1) ++nameCursor;
}

// ---------------------------------------------------------------------------------------------
// List pages: how many items, what each row shows, what A and Left / Right do.

bool isList(Page page) {
  switch (page) {
    case Page::Main:
    case Page::Edit:
    case Page::Button:
    case Page::Stick:
    case Page::Picker:
    case Page::NewProfile:
    case Page::Profiles:
    case Page::ProfileActions:
    case Page::Controller:
    case Page::DisplayOptions:
    case Page::AutoOff: return true;
    default: return false;
  }
}

uint8_t itemCount(const Frame& frame) {
  switch (frame.page) {
    case Page::Main: return 5;
    case Page::Edit: return 8;
    case Page::Button: return 2 + ACTIONS_PER_BUTTON;
    case Page::Stick:
      switch (stickMapping().mode) {
        case StickMode::Mouse:
        case StickMode::Scroll: return 3;
        case StickMode::Directions: return 2 + STICK_DIRECTION_COUNT;
        default: return 2;
      }
    case Page::Picker: return frame.category->size();
    case Page::NewProfile: return PRESET_COUNT + list->count;
    case Page::Profiles: return list->count;
    case Page::ProfileActions: return 4;
    case Page::Controller: return 6;
    case Page::DisplayOptions: return 3;
    case Page::AutoOff: return 2;
    default: return 0;  // message pages
  }
}

void describe(const Frame& frame, uint8_t index, Row& row) {
  Text text;
  switch (frame.page) {
    case Page::Main: {
      static const char* const labels[] = {nullptr, "New profile", "Profiles", "Controller", "Exit"};
      if (index == 0) {
        format(text, "Edit %s", list->profiles[list->active].name);
        setRow(row, text, "", true);
      } else {
        setRow(row, labels[index], "", index != 4);
      }
      break;
    }
    case Page::Edit:
      switch (index) {
        case 0: setRow(row, "Buttons", "", true); break;
        case 1:
        case 2: {
          // The base layer's mode; the stick page itself switches between layers.
          const StickMode mode = profile().layers[0].sticks[index - 1].mode;
          setRow(row, STICK_ROW_NAMES[index - 1], STICK_MODE_NAMES[static_cast<uint8_t>(mode)]);
          break;
        }
        case 3:
        case 4:
          format(text, "%u%%", profile().deadzonePercent[index - 3]);
          setAdjustRow(row, index == 3 ? "Deadzone L" : "Deadzone R", text);
          break;
        case 5:
          format(text, "%u Hz", profile().turboHz);
          setAdjustRow(row, "Turbo speed", text);
          break;
        case 6: setRow(row, "Rename", "", true); break;
        case 7: setRow(row, "Copy base to RTZ", "", true); break;
      }
      break;
    case Page::Button:
      if (index == 0) {
        setAdjustRow(row, "Layer", LAYER_NAMES[editLayer]);
      } else if (index <= ACTIONS_PER_BUTTON) {
        format(text, "Action %u", index);
        setRow(row, text, ActionCatalog::nameOf(buttonMapping().actions[index - 1]));
      } else {
        setAdjustRow(row, "Mode", BUTTON_MODE_NAMES[static_cast<uint8_t>(buttonMapping().mode)]);
      }
      break;
    case Page::Stick: {
      const StickMapping& mapping = stickMapping();
      if (index == 0) {
        setAdjustRow(row, "Layer", LAYER_NAMES[editLayer]);
      } else if (index == 1) {
        setAdjustRow(row, "Mode", STICK_MODE_NAMES[static_cast<uint8_t>(mapping.mode)]);
      } else if (mapping.mode == StickMode::Directions) {
        setRow(row, DIRECTION_NAMES[index - 2], ActionCatalog::nameOf(mapping.directions[index - 2]));
      } else {
        format(text, "%u", mapping.sensitivity);
        setAdjustRow(row, "Speed", text);
      }
      break;
    }
    case Page::Picker: {
      const ActionCatalog::Category& category = *frame.category;
      if (index < category.itemCount) {
        const ActionCatalog::Item& item = category.items[index];
        const bool current = item.action.type == pickTarget->type &&
                             (item.action.type == ActionType::None || item.action.type == ActionType::Layer ||
                              item.action.code == pickTarget->code);
        setRow(row, item.name, current ? "*" : "");
      } else {
        setRow(row, category.children[index - category.itemCount].name, "", true);
      }
      break;
    }
    case Page::NewProfile:
      if (index < PRESET_COUNT) {
        format(text, "Preset %s", Profiles::preset(static_cast<Profiles::Preset>(index)).name);
      } else {
        format(text, "Copy %s", list->profiles[index - PRESET_COUNT].name);
      }
      setRow(row, text, "", true);
      break;
    case Page::Profiles:
      format(text, "%u %s", index + 1, list->profiles[index].name);
      setRow(row, text, index == list->active ? "*" : "");
      break;
    case Page::ProfileActions: {
      static const char* const labels[] = {"Edit", "Move up", "Move down", "Delete"};
      setRow(row, labels[index], "", index == 0 || index == 3);
      break;
    }
    case Page::Controller: {
      static const char* const labels[] = {"Display", "Auto off", "Recalibrate sticks", "Forget Bluetooth",
                                           "Reset all profiles", "About"};
      setRow(row, labels[index], "", true);
      break;
    }
    case Page::DisplayOptions: {
      const DisplaySettings& display = list->display;
      switch (index) {
        case 0:
          format(text, "%u", display.brightness);
          setAdjustRow(row, "Brightness", text);
          break;
        case 1: {
          const uint16_t seconds = Config::DISPLAY_SLEEP_OPTIONS_S[display.screenOff];
          if (seconds == 0) format(text, "Never");
          else if (seconds < 60) format(text, "%u s", seconds);
          else format(text, "%u min", seconds / 60);
          setAdjustRow(row, "Screen off", text);
          break;
        }
        case 2: setAdjustRow(row, "Name popup", display.namePopup ? "On" : "Off"); break;
      }
      break;
    }
    case Page::AutoOff: {
      const uint8_t option = index == 0 ? list->power.offWhenConnected : list->power.offWhenOffline;
      const uint8_t minutes = Config::AUTO_OFF_OPTIONS_MIN[option];
      if (minutes == 0) format(text, "Never");
      else format(text, "%u min", minutes);
      setAdjustRow(row, index == 0 ? "Connected" : "Offline", text);
      break;
    }
    default: break;
  }
}

template <typename T>
T cycle(T value, int delta, uint8_t count) {
  return static_cast<T>((static_cast<uint8_t>(value) + count + delta) % count);
}

uint8_t clampAdd(uint8_t value, int delta, uint8_t low, uint8_t high) {
  return static_cast<uint8_t>(constrain(static_cast<int>(value) + delta, static_cast<int>(low), static_cast<int>(high)));
}

void adjust(const Frame& frame, int delta) {
  switch (frame.page) {
    case Page::Edit: {
      Profile& p = profile();
      if (frame.cursor == 3 || frame.cursor == 4) {
        uint8_t& deadzone = p.deadzonePercent[frame.cursor - 3];
        const uint8_t next = clampAdd(deadzone, delta, 0, Config::STICK_DEADZONE_MAX_PERCENT);
        if (next != deadzone) { deadzone = next; markChanged(); }
      } else if (frame.cursor == 5) {
        const uint8_t next = clampAdd(p.turboHz, delta, Config::TURBO_MIN_HZ, Config::TURBO_MAX_HZ);
        if (next != p.turboHz) { p.turboHz = next; markChanged(); }
      }
      break;
    }
    case Page::Button:
      if (frame.cursor == 0) {
        editLayer = cycle(editLayer, delta, LAYER_COUNT);
      } else if (frame.cursor == 1 + ACTIONS_PER_BUTTON) {
        buttonMapping().mode = cycle(buttonMapping().mode, delta, BUTTON_MODE_COUNT);
        markChanged();
      }
      break;
    case Page::DisplayOptions: {
      // Previewed at once: main applies the working copy to the screen while the menu is open.
      DisplaySettings& display = list->display;
      const DisplaySettings before = display;
      if (frame.cursor == 0) {
        display.brightness = clampAdd(display.brightness, delta, 1, Config::DISPLAY_BRIGHTNESS_LEVELS);
      } else if (frame.cursor == 1) {
        display.screenOff = clampAdd(display.screenOff, delta, 0, Config::DISPLAY_SLEEP_OPTION_COUNT - 1);
      } else if (frame.cursor == 2) {
        display.namePopup = !display.namePopup;
      }
      if (memcmp(&before, &display, sizeof(display)) != 0) list->displayChanged = true;
      break;
    }
    case Page::AutoOff: {
      uint8_t& option = frame.cursor == 0 ? list->power.offWhenConnected : list->power.offWhenOffline;
      const uint8_t next = clampAdd(option, delta, 0, Config::AUTO_OFF_OPTION_COUNT - 1);
      if (next != option) {
        option = next;
        list->powerChanged = true;
      }
      break;
    }
    case Page::Stick: {
      StickMapping& mapping = stickMapping();
      if (frame.cursor == 0) {
        editLayer = cycle(editLayer, delta, LAYER_COUNT);
      } else if (frame.cursor == 1) {
        mapping.mode = cycle(mapping.mode, delta, STICK_MODE_COUNT);
        markChanged();
      } else if (mapping.mode == StickMode::Mouse || mapping.mode == StickMode::Scroll) {
        const uint8_t next = clampAdd(mapping.sensitivity, delta, Config::SENSITIVITY_MIN, Config::SENSITIVITY_MAX);
        if (next != mapping.sensitivity) { mapping.sensitivity = next; markChanged(); }
      }
      break;
    }
    default: break;
  }
}

// Opens the catalog with the cursor on the current value (inside its sub-list, if it has one).
void openPicker(Action* target, const char* title) {
  pickTarget = target;
  format(pickTitle, "%s", title);
  uint8_t path[ActionCatalog::MAX_DEPTH] = {};
  const uint8_t levels = ActionCatalog::find(*target, path);
  const ActionCatalog::Category* category = &ActionCatalog::root();
  push(Page::Picker, levels > 0 ? path[0] : 0, category);
  for (uint8_t level = 1; level < levels; ++level) {
    category = &category->children[path[level - 1] - category->itemCount];
    push(Page::Picker, path[level], category);
  }
}

void closePicker() {
  while (depth > 1 && top().page == Page::Picker) pop();
}

void requestExit() {
  if (!hasChanges()) {
    pending = Request::Discard;
    return;
  }
  if (top().page != Page::SavePrompt) push(Page::SavePrompt);
}

// Moves the selected profile one place and keeps the list's cursor on it.
void moveSelected(int delta) {
  const int target = selectedProfile + delta;
  if (target < 0 || target >= list->count) return;
  swapProfiles(selectedProfile, static_cast<uint8_t>(target));
  selectedProfile = static_cast<uint8_t>(target);
  if (Frame* profiles = below()) profiles->cursor = selectedProfile;
}

void answerYes(Question question) {
  switch (question) {
    case Question::CopyToRtz:
      profile().layers[1] = profile().layers[0];
      markChanged();
      pop();
      break;
    case Question::DeleteProfile:
      deleteProfile(selectedProfile);
      pop();  // the question
      pop();  // the profile's actions: its profile is gone
      break;
    case Question::ForgetPairings:
      pending = Request::ForgetPairings;
      pop();
      notify(Notice::PairingsErased);
      break;
    case Question::ResetProfiles:
      resetToPresets();
      pop();
      break;
  }
}

void activate(Frame& frame) {
  Text title;
  switch (frame.page) {
    case Page::Main:
      switch (frame.cursor) {
        case 0:
          editIdx = list->active;
          push(Page::Edit);
          break;
        case 1:
          if (list->count >= MAX_PROFILES) notify(Notice::ProfilesFull);
          else push(Page::NewProfile);
          break;
        case 2: push(Page::Profiles, list->active); break;
        case 3: push(Page::Controller); break;
        case 4: requestExit(); break;
      }
      break;
    case Page::Edit:
      switch (frame.cursor) {
        case 0: push(Page::PressButton); break;
        case 1:
        case 2:
          editStick = frame.cursor - 1;
          push(Page::Stick);
          break;
        case 6: openNameEntry(profile().name, false); break;
        case 7: confirm(Question::CopyToRtz); break;
        default: break;
      }
      break;
    case Page::Button:
      if (frame.cursor >= 1 && frame.cursor <= ACTIONS_PER_BUTTON) {
        format(title, "%s ACTION %u", BUTTON_TITLES[editButton], frame.cursor);
        openPicker(&buttonMapping().actions[frame.cursor - 1], title);
      }
      break;
    case Page::Stick:
      if (stickMapping().mode == StickMode::Directions && frame.cursor >= 2) {
        const uint8_t direction = frame.cursor - 2;
        format(title, "%s %s", STICK_TITLES[editStick], DIRECTION_NAMES[direction]);
        upperCase(title);
        openPicker(&stickMapping().directions[direction], title);
      }
      break;
    case Page::Picker: {
      const ActionCatalog::Category& category = *frame.category;
      if (frame.cursor < category.itemCount) {
        *pickTarget = category.items[frame.cursor].action;
        markChanged();
        closePicker();
      } else {
        push(Page::Picker, 0, &category.children[frame.cursor - category.itemCount]);
      }
      break;
    }
    case Page::NewProfile:
      newProfile = frame.cursor < PRESET_COUNT
                       ? Profiles::preset(static_cast<Profiles::Preset>(frame.cursor))
                       : list->profiles[frame.cursor - PRESET_COUNT];
      openNameEntry(newProfile.name, true);
      break;
    case Page::Profiles:
      selectedProfile = frame.cursor;
      push(Page::ProfileActions);
      break;
    case Page::ProfileActions:
      switch (frame.cursor) {
        case 0:
          editIdx = selectedProfile;
          push(Page::Edit);
          break;
        case 1: moveSelected(-1); break;
        case 2: moveSelected(1); break;
        case 3:
          if (list->count <= 1) notify(Notice::LastProfile);
          else confirm(Question::DeleteProfile);
          break;
      }
      break;
    case Page::Controller:
      switch (frame.cursor) {
        case 0: push(Page::DisplayOptions); break;
        case 1: push(Page::AutoOff); break;
        case 2: push(Page::Recalibrate); break;
        case 3: confirm(Question::ForgetPairings); break;
        case 4: confirm(Question::ResetProfiles); break;
        case 5: notify(Notice::About); break;
      }
      break;
    case Page::Recalibrate:
      pending = Request::RecalibrateSticks;
      pop();
      notify(Notice::SticksRecalibrated);
      break;
    case Page::Confirm: answerYes(static_cast<Question>(frame.kind)); break;
    case Page::Info: pop(); break;
    case Page::SavePrompt: pending = Request::Save; break;
    default: break;
  }
}

// B on each page.
void back(const Frame& frame) {
  switch (frame.page) {
    case Page::Main: break;  // leaving takes a Profile hold (or Exit), so a stray B can't close the menu
    case Page::PressButton: break;  // B is a button you may want to edit; Profile goes back here
    case Page::SavePrompt: pending = Request::Discard; break;
    default: pop(); break;
  }
}

void keepVisible(Frame& frame) {
  const uint8_t count = itemCount(frame);
  if (count == 0) return;
  if (frame.cursor >= count) frame.cursor = count - 1;
  if (frame.cursor < frame.first) frame.first = frame.cursor;
  if (frame.cursor >= frame.first + ROWS) frame.first = frame.cursor - ROWS + 1;
  const uint8_t maxFirst = count > ROWS ? count - ROWS : 0;
  if (frame.first > maxFirst) frame.first = maxFirst;
}

// A fresh press wraps from one end of the list to the other; a repeat stops at the end.
void moveCursor(Frame& frame, int delta, bool wrap) {
  const int count = itemCount(frame);
  if (count == 0) return;
  int cursor = frame.cursor + delta;
  if (cursor < 0) cursor = wrap && delta == -1 ? count - 1 : 0;
  if (cursor >= count) cursor = wrap && delta == 1 ? 0 : count - 1;
  frame.cursor = static_cast<uint8_t>(cursor);
}

// ---------------------------------------------------------------------------------------------
// Screens

void listTitle(const Frame& frame) {
  Text text;
  switch (frame.page) {
    case Page::Main: setTitle("MENU"); break;
    case Page::Edit:
      format(text, "EDIT %s", profile().name);
      setTitle(text);
      break;
    case Page::Button:
      format(text, "BUTTON %s", BUTTON_TITLES[editButton]);
      setTitle(text);
      break;
    case Page::Stick: setTitle(STICK_TITLES[editStick]); break;
    case Page::Picker:
      if (frame.category == &ActionCatalog::root()) {
        setTitle(pickTitle);
      } else {
        format(text, "%s", frame.category->name);
        upperCase(text);
        setTitle(text);
      }
      break;
    case Page::NewProfile: setTitle("NEW PROFILE FROM"); break;
    case Page::Profiles:
      format(text, "PROFILES %u/%u", list->count, MAX_PROFILES);
      setTitle(text);
      break;
    case Page::ProfileActions:
      format(text, "PROFILE %s", list->profiles[selectedProfile].name);
      setTitle(text);
      break;
    case Page::Controller: setTitle("CONTROLLER"); break;
    case Page::DisplayOptions: setTitle("DISPLAY"); break;
    case Page::AutoOff: setTitle("AUTO OFF WHEN IDLE"); break;
    default: break;
  }
}

void messageLines(const char* title, const char* const (&lines)[ROWS - 1], const char* hintLeft,
                  const char* hintRight) {
  setTitle(title);
  for (uint8_t i = 0; i < ROWS - 1; ++i) setRow(menuView.rows[i], lines[i] != nullptr ? lines[i] : "");
  setRow(menuView.rows[ROWS - 1], hintLeft, hintRight);
  menuView.rowCount = ROWS;
}

void confirmView(Question question) {
  Text line;
  switch (question) {
    case Question::CopyToRtz:
      messageLines("COPY BASE TO RTZ", {"Replace the RTZ layer", "with a copy of the", "base layer?", nullptr},
                   "A: copy", "B: cancel");
      break;
    case Question::DeleteProfile:
      format(line, "Delete %s?", list->profiles[selectedProfile].name);
      messageLines("DELETE PROFILE", {line, nullptr, "Kept only if you", "save on the way out."}, "A: delete",
                   "B: cancel");
      break;
    case Question::ForgetPairings:
      messageLines("FORGET BLUETOOTH", {"Erase all pairings,", "then remove it in", "Windows & pair again.", nullptr},
                   "A: erase", "B: cancel");
      break;
    case Question::ResetProfiles:
      messageLines("RESET PROFILES", {"Replace all profiles", "with the 4 presets?", "Kept only if you",
                                      "save on the way out."},
                   "A: reset", "B: cancel");
      break;
  }
}

void noticeView(Notice notice) {
  Text line;
  switch (notice) {
    case Notice::PairingsErased:
      messageLines("BLUETOOTH", {"Pairings erased.", "Remove the controller", "in Windows, then pair", "it again."}, "",
                   "B: back");
      break;
    case Notice::About:
      format(line, "Firmware %s", Config::FIRMWARE_VERSION);
      messageLines("ABOUT", {"ESP32 Gamepad", line, "Robitza Electrical", "Engineering"}, "", "B: back");
      break;
    case Notice::ProfilesFull:
      format(line, "Already %u profiles,", MAX_PROFILES);
      messageLines("NEW PROFILE", {line, "the maximum. Delete", "one on the Profiles", "page first."}, "", "B: back");
      break;
    case Notice::LastProfile:
      messageLines("DELETE PROFILE", {"The last profile", "can't be deleted.", nullptr, nullptr}, "", "B: back");
      break;
    case Notice::SticksRecalibrated:
      messageLines("RECALIBRATE STICKS", {"Stick centers", "measured again.", nullptr, nullptr}, "", "B: back");
      break;
  }
}

void buildView() {
  menuView = MenuView{};
  if (pending == Request::Save) {
    menuView.kind = MenuView::Kind::Message;
    setTitle("SAVING");
    setRow(menuView.rows[0], "Saving profiles...");
    menuView.rowCount = 1;
    return;
  }

  Frame& frame = top();
  if (isList(frame.page)) {
    keepVisible(frame);
    listTitle(frame);
    const uint8_t count = itemCount(frame);
    menuView.itemCount = count;
    menuView.firstItem = frame.first;
    menuView.cursorRow = frame.cursor - frame.first;
    for (uint8_t i = 0; i < ROWS && frame.first + i < count; ++i) {
      describe(frame, frame.first + i, menuView.rows[i]);
      menuView.rowCount = i + 1;
    }
    return;
  }

  menuView.kind = MenuView::Kind::Message;
  Row* rows = menuView.rows;
  switch (frame.page) {
    case Page::PressButton:
      setTitle("EDIT BUTTON");
      setRow(rows[0], "Press the button");
      setRow(rows[1], "you want to edit.");
      setRow(rows[3], "Profile", "back");
      menuView.rowCount = 4;
      break;
    case Page::NameEntry:
      menuView.kind = MenuView::Kind::TextEntry;
      setTitle(namingNewProfile ? "NAME NEW PROFILE" : "RENAME");
      memcpy(menuView.text, nameBuffer, sizeof(menuView.text));
      menuView.textCursor = nameCursor;
      setRow(rows[0], "Up/Dn char", "L1/R1 group");
      setRow(rows[1], "X del  Y ins", "A ok  B back");
      menuView.rowCount = 2;
      break;
    case Page::Recalibrate:
      messageLines("RECALIBRATE STICKS", {"Let go of both", "sticks, then press A.", nullptr, nullptr}, "A: measure",
                   "B: cancel");
      break;
    case Page::Confirm: confirmView(static_cast<Question>(frame.kind)); break;
    case Page::Info: noticeView(static_cast<Notice>(frame.kind)); break;
    case Page::SavePrompt:
      setTitle("SAVE CHANGES?");
      setRow(rows[0], "A", "Save");
      setRow(rows[1], "B", "Discard");
      setRow(rows[2], "Profile", "Keep editing");
      menuView.rowCount = 3;
      break;
    default: break;
  }
}

}  // namespace

void open(Workspace& profiles, uint32_t nowMs) {
  (void)nowMs;
  list = &profiles;
  list->changedSlots = 0;
  list->countChanged = false;
  list->displayChanged = false;
  list->powerChanged = false;
  if (list->active >= list->count) list->active = 0;
  editIdx = list->active;
  editLayer = 0;
  depth = 0;
  push(Page::Main);
  pending = Request::None;
  resetInputState();
  stickDirection = -1;
  stickBlocked = false;
  blocked = 0;
  blockOnNextUpdate = true;  // Profile is still held from the gesture that opened the menu
  buildView();
  LOG("menu: open on profile %u (%s)", editIdx + 1, profile().name);
}

Request update(const Input& in, uint32_t nowMs) {
  if (list == nullptr) return Request::Discard;
  pending = Request::None;

  // Whatever is held when the menu opens or the screen wakes up does nothing until released.
  if (blockOnNextUpdate || in.screenWasOff) {
    blockOnNextUpdate = false;
    blocked |= in.held | in.pressed;
    readStick(in);
    stickBlocked = stickDirection >= 0;
    resetInputState();
    buildView();
    return Request::None;
  }
  blocked &= in.held;
  readStick(in);
  if (stickBlocked && stickDirection < 0) stickBlocked = false;
  const int8_t stick = stickBlocked ? -1 : stickDirection;

  const uint32_t edges = in.pressed & ~blocked;
  const uint32_t held = in.held & ~blocked;

  // Profile: short press = back, hold = leave.
  bool profileShort = false;
  if (edges & PROFILE_BIT) {
    profileDown = true;
    profileHoldFired = false;
    profileDownMs = nowMs;
  }
  if (profileDown) {
    if (!(held & PROFILE_BIT)) {
      profileDown = false;
      profileShort = !profileHoldFired;
    } else if (!profileHoldFired && nowMs - profileDownMs >= Config::PROFILE_HOLD_MS) {
      profileHoldFired = true;
      if (top().page != Page::SavePrompt) requestExit();
    }
  }

  Frame& frame = top();
  if (pending != Request::None) {
    // leaving: nothing else this update
  } else if (frame.page == Page::PressButton && (edges & MAPPABLE_MASK) != 0) {
    // The lowest pressed button wins; it stays blocked, so it can't also act on the next page.
    editButton = static_cast<uint8_t>(__builtin_ctz(edges & MAPPABLE_MASK));
    blocked |= in.held | edges;
    resetInputState();
    push(Page::Button);
  } else if (profileShort) {
    pop();
  } else if (frame.page == Page::NameEntry) {
    nameEntryInput(edges, held, stick, nowMs);
  } else if (edges & buttonBit(Button::A)) {
    activate(frame);
  } else if (edges & buttonBit(Button::B)) {
    back(frame);
  } else if (isList(frame.page) && (edges & (buttonBit(Button::L1) | buttonBit(Button::R1)))) {
    moveCursor(frame, (edges & buttonBit(Button::L1)) ? -ROWS : ROWS, false);
  } else {
    uint8_t initial = 0;
    const uint8_t fired = firedDirections(edges, held, stick, nowMs, initial);
    if (isList(frame.page)) {
      for (uint8_t d = 0; d < DIRECTION_COUNT; ++d) {
        if (!(fired & (1u << d))) continue;
        const bool fresh = initial & (1u << d);
        if (d == Up || d == Down) moveCursor(frame, d == Up ? -1 : 1, fresh);
        else adjust(frame, d == Left ? -1 : 1);
      }
    }
  }

  buildView();
  if (pending == Request::Save || pending == Request::Discard) {
    LOG("menu: closed, %s", pending == Request::Save ? "saving" : "nothing to save / discarded");
    list = nullptr;  // the caller owns the list again
  }
  return pending;
}

const MenuView& view() { return menuView; }

}  // namespace ConfigMenu
