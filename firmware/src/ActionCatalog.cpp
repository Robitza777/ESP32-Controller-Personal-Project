#include "ActionCatalog.h"

#include <stddef.h>

#include "ControllerTypes.h"

namespace ActionCatalog {
namespace {

template <typename E>
constexpr uint16_t code(E value) {
  return static_cast<uint16_t>(value);
}

constexpr Item key(const char* name, uint16_t usage) { return {name, {ActionType::Key, usage}}; }
constexpr Item consumer(const char* name, uint16_t usage) { return {name, {ActionType::Consumer, usage}}; }
constexpr Item xbox(const char* name, XboxButton button) { return {name, {ActionType::Xbox, code(button)}}; }
constexpr Item dpad(const char* name, DpadDirection direction) { return {name, {ActionType::Dpad, code(direction)}}; }
constexpr Item trigger(const char* name, TriggerSide side) { return {name, {ActionType::Trigger, code(side)}}; }
constexpr Item mouse(const char* name, MouseButton button) { return {name, {ActionType::Mouse, code(button)}}; }
constexpr Item scroll(const char* name, StickDirection direction) { return {name, {ActionType::Scroll, code(direction)}}; }
constexpr Item systemKey(const char* name, SystemKey key) { return {name, {ActionType::System, code(key)}}; }

template <size_t N>
constexpr Category list(const char* name, const Item (&items)[N]) {
  return {name, items, static_cast<uint8_t>(N), nullptr, 0};
}

template <size_t N>
constexpr Category group(const char* name, const Category (&children)[N]) {
  return {name, nullptr, 0, children, static_cast<uint8_t>(N)};
}

// ---------------------------------------------------------------------------------------------
// Keyboard (HID page 0x07). Names stay unique across the whole catalog, because a menu row shows only the name.

constexpr Item LETTERS[] = {
  key("A", 0x04), key("B", 0x05), key("C", 0x06), key("D", 0x07), key("E", 0x08), key("F", 0x09),
  key("G", 0x0A), key("H", 0x0B), key("I", 0x0C), key("J", 0x0D), key("K", 0x0E), key("L", 0x0F),
  key("M", 0x10), key("N", 0x11), key("O", 0x12), key("P", 0x13), key("Q", 0x14), key("R", 0x15),
  key("S", 0x16), key("T", 0x17), key("U", 0x18), key("V", 0x19), key("W", 0x1A), key("X", 0x1B),
  key("Y", 0x1C), key("Z", 0x1D),
};

constexpr Item NUMBERS[] = {
  key("1", 0x1E), key("2", 0x1F), key("3", 0x20), key("4", 0x21), key("5", 0x22),
  key("6", 0x23), key("7", 0x24), key("8", 0x25), key("9", 0x26), key("0", 0x27),
};

constexpr Item FUNCTION_KEYS[] = {
  key("F1", 0x3A),  key("F2", 0x3B),  key("F3", 0x3C),  key("F4", 0x3D),  key("F5", 0x3E),  key("F6", 0x3F),
  key("F7", 0x40),  key("F8", 0x41),  key("F9", 0x42),  key("F10", 0x43), key("F11", 0x44), key("F12", 0x45),
  key("F13", 0x68), key("F14", 0x69), key("F15", 0x6A), key("F16", 0x6B), key("F17", 0x6C), key("F18", 0x6D),
  key("F19", 0x6E), key("F20", 0x6F), key("F21", 0x70), key("F22", 0x71), key("F23", 0x72), key("F24", 0x73),
};

constexpr Item MODIFIERS[] = {
  key("Left Ctrl", 0xE0),  key("Left Shift", 0xE1),  key("Left Alt", 0xE2),  key("Left Win", 0xE3),
  key("Right Ctrl", 0xE4), key("Right Shift", 0xE5), key("Right Alt", 0xE6), key("Right Win", 0xE7),
};

constexpr Item NAVIGATION[] = {
  key("Arrow Up", 0x52), key("Arrow Down", 0x51), key("Arrow Left", 0x50), key("Arrow Right", 0x4F),
  key("Home", 0x4A),     key("End", 0x4D),        key("Page Up", 0x4B),    key("Page Down", 0x4E),
  key("Insert", 0x49),   key("Delete", 0x4C),
};

constexpr Item BASIC_KEYS[] = {
  key("Enter", 0x28),     key("Escape", 0x29),       key("Backspace", 0x2A),   key("Tab", 0x2B),
  key("Space", 0x2C),     key("Caps Lock", 0x39),    key("Print Screen", 0x46), key("Scroll Lock", 0x47),
  key("Pause", 0x48),     key("Context menu", 0x65),
};

constexpr Item SYMBOLS[] = {
  key("- _", 0x2D), key("= +", 0x2E), key("[ {", 0x2F), key("] }", 0x30), key("\\ |", 0x31), key("; :", 0x33),
  key("' \"", 0x34), key("` ~", 0x35), key(", <", 0x36), key(". >", 0x37), key("/ ?", 0x38),
  key("Non-US # ~", 0x32), key("Non-US \\ |", 0x64),
};

constexpr Item NUMPAD[] = {
  key("Num 0", 0x62), key("Num 1", 0x59), key("Num 2", 0x5A), key("Num 3", 0x5B), key("Num 4", 0x5C),
  key("Num 5", 0x5D), key("Num 6", 0x5E), key("Num 7", 0x5F), key("Num 8", 0x60), key("Num 9", 0x61),
  key("Num .", 0x63), key("Num Enter", 0x58), key("Num +", 0x57), key("Num -", 0x56), key("Num *", 0x55),
  key("Num /", 0x54), key("Num =", 0x67), key("Num ,", 0x85), key("Num Lock", 0x53),
};

// Everything else on the keyboard page, in code order. Most of it does nothing on Windows, but the user wants
// every key available (e.g. for apps that read raw keys).
constexpr Item OTHER_KEYS[] = {
  key("Power", 0x66),        key("Execute", 0x74),      key("Help", 0x75),         key("Key Menu", 0x76),
  key("Select", 0x77),       key("Stop", 0x78),         key("Again", 0x79),        key("Undo", 0x7A),
  key("Cut", 0x7B),          key("Copy", 0x7C),         key("Paste", 0x7D),        key("Find", 0x7E),
  key("Key Mute", 0x7F),     key("Key Vol Up", 0x80),   key("Key Vol Down", 0x81), key("Lock Caps", 0x82),
  key("Lock Num", 0x83),     key("Lock Scroll", 0x84),  key("Num = AS400", 0x86),  key("Intl 1", 0x87),
  key("Intl 2", 0x88),       key("Intl 3", 0x89),       key("Intl 4", 0x8A),       key("Intl 5", 0x8B),
  key("Intl 6", 0x8C),       key("Intl 7", 0x8D),       key("Intl 8", 0x8E),       key("Intl 9", 0x8F),
  key("Lang 1", 0x90),       key("Lang 2", 0x91),       key("Lang 3", 0x92),       key("Lang 4", 0x93),
  key("Lang 5", 0x94),       key("Lang 6", 0x95),       key("Lang 7", 0x96),       key("Lang 8", 0x97),
  key("Lang 9", 0x98),       key("Alt Erase", 0x99),    key("SysReq", 0x9A),       key("Cancel", 0x9B),
  key("Clear", 0x9C),        key("Prior", 0x9D),        key("Return", 0x9E),       key("Separator", 0x9F),
  key("Out", 0xA0),          key("Oper", 0xA1),         key("Clear/Again", 0xA2),  key("CrSel", 0xA3),
  key("ExSel", 0xA4),        key("Num 00", 0xB0),       key("Num 000", 0xB1),      key("Thousand sep", 0xB2),
  key("Decimal sep", 0xB3),  key("Currency", 0xB4),     key("Sub-currency", 0xB5), key("Num (", 0xB6),
  key("Num )", 0xB7),        key("Num {", 0xB8),        key("Num }", 0xB9),        key("Num Tab", 0xBA),
  key("Num Backspc", 0xBB),  key("Num A", 0xBC),        key("Num B", 0xBD),        key("Num C", 0xBE),
  key("Num D", 0xBF),        key("Num E", 0xC0),        key("Num F", 0xC1),        key("Num XOR", 0xC2),
  key("Num ^", 0xC3),        key("Num %", 0xC4),        key("Num <", 0xC5),        key("Num >", 0xC6),
  key("Num &", 0xC7),        key("Num &&", 0xC8),       key("Num |", 0xC9),        key("Num ||", 0xCA),
  key("Num :", 0xCB),        key("Num #", 0xCC),        key("Num Space", 0xCD),    key("Num @", 0xCE),
  key("Num !", 0xCF),        key("Mem Store", 0xD0),    key("Mem Recall", 0xD1),   key("Mem Clear", 0xD2),
  key("Mem Add", 0xD3),      key("Mem Subtract", 0xD4), key("Mem Multiply", 0xD5), key("Mem Divide", 0xD6),
  key("Num +/-", 0xD7),      key("Num Clear", 0xD8),    key("Clear Entry", 0xD9),  key("Num Binary", 0xDA),
  key("Num Octal", 0xDB),    key("Num Decimal", 0xDC),  key("Num Hex", 0xDD),
};

constexpr Category KEYBOARD_GROUPS[] = {
  list("Letters", LETTERS),       list("Numbers", NUMBERS), list("F1-F24", FUNCTION_KEYS),
  list("Modifiers", MODIFIERS),   list("Navigation", NAVIGATION), list("Basic keys", BASIC_KEYS),
  list("Symbols", SYMBOLS),       list("Numpad", NUMPAD),   list("Other", OTHER_KEYS),
};

// ---------------------------------------------------------------------------------------------
// Gamepad and mouse

constexpr Item GAMEPAD[] = {
  xbox("Xbox A", XboxButton::A),      xbox("Xbox B", XboxButton::B),        xbox("Xbox X", XboxButton::X),
  xbox("Xbox Y", XboxButton::Y),      xbox("LB", XboxButton::LB),           xbox("RB", XboxButton::RB),
  trigger("LT", TriggerSide::Left),   trigger("RT", TriggerSide::Right),    xbox("View", XboxButton::View),
  xbox("Menu", XboxButton::Menu),     xbox("Guide", XboxButton::Guide),     xbox("Share", XboxButton::Share),
  xbox("LS click", XboxButton::LS),   xbox("RS click", XboxButton::RS),     dpad("D-pad Up", DpadDirection::Up),
  dpad("D-pad Down", DpadDirection::Down), dpad("D-pad Left", DpadDirection::Left),
  dpad("D-pad Right", DpadDirection::Right),
};

constexpr Item MOUSE[] = {
  mouse("Left click", MouseButton::Left),     mouse("Right click", MouseButton::Right),
  mouse("Middle click", MouseButton::Middle), mouse("Back (4)", MouseButton::Back),
  mouse("Forward (5)", MouseButton::Forward), scroll("Scroll up", StickDirection::Up),
  scroll("Scroll down", StickDirection::Down), scroll("Scroll left", StickDirection::Left),
  scroll("Scroll right", StickDirection::Right),
};

// ---------------------------------------------------------------------------------------------
// Consumer page (0x0C) and system keys: the groups the user kept (CLAUDE.md, "Consumer/system keys kept").

constexpr Item MEDIA[] = {
  consumer("Play/Pause", 0x0CD), consumer("Next track", 0x0B5), consumer("Prev track", 0x0B6),
  consumer("Stop media", 0x0B7), consumer("Volume +", 0x0E9),   consumer("Volume -", 0x0EA),
  consumer("Mute", 0x0E2),
};

constexpr Item LAUNCH[] = {
  consumer("Media player", 0x183), consumer("Mail", 0x18A), consumer("Calculator", 0x192),
  consumer("Explorer", 0x194),
};

constexpr Item BROWSER[] = {
  consumer("Web Search", 0x221), consumer("Web Home", 0x223),    consumer("Web Back", 0x224),
  consumer("Web Forward", 0x225), consumer("Web Stop", 0x226),   consumer("Web Refresh", 0x227),
  consumer("Web Favorite", 0x22A),
};

// Windows turns these into app commands; what they do depends on the app in front.
constexpr Item EDITING[] = {
  consumer("App New", 0x201),   consumer("App Open", 0x202),     consumer("App Close", 0x203),
  consumer("App Save", 0x207),  consumer("App Print", 0x208),    consumer("App Undo", 0x21A),
  consumer("App Copy", 0x21B),  consumer("App Cut", 0x21C),      consumer("App Paste", 0x21D),
  consumer("App Find", 0x21F),  consumer("App Redo", 0x279),     consumer("App Reply", 0x289),
  consumer("App Fwd mail", 0x28B), consumer("App Send", 0x28C),
};

constexpr Item BRIGHTNESS[] = {
  consumer("Brightness +", 0x06F), consumer("Brightness -", 0x070),
};

constexpr Item SYSTEM[] = {
  systemKey("Power down", SystemKey::PowerDown), systemKey("Sleep", SystemKey::Sleep),
  systemKey("Wake up", SystemKey::WakeUp),
};

constexpr Category MEDIA_GROUPS[] = {
  list("Media & volume", MEDIA), list("Launch apps", LAUNCH), list("Browser", BROWSER),
  list("Editing", EDITING),      list("Brightness", BRIGHTNESS), list("System", SYSTEM),
};

// ---------------------------------------------------------------------------------------------
// Root

constexpr Item ROOT_ITEMS[] = {
  {"None", {ActionType::None, 0}},
  {"RTZ layer", {ActionType::Layer, 0}},
};

constexpr Category ROOT_GROUPS[] = {
  list("Gamepad", GAMEPAD),
  group("Keyboard", KEYBOARD_GROUPS),
  list("Mouse", MOUSE),
  group("Media & system", MEDIA_GROUPS),
};

constexpr Category ROOT = {"Actions", ROOT_ITEMS, 2, ROOT_GROUPS, 4};

constexpr size_t length(const char* text) {
  size_t n = 0;
  while (text[n] != '\0') ++n;
  return n;
}

template <size_t N>
constexpr bool namesFit(const Item (&items)[N]) {
  for (const Item& item : items) {
    if (length(item.name) > NAME_CHARS) return false;
  }
  return true;
}

static_assert(namesFit(LETTERS) && namesFit(NUMBERS) && namesFit(FUNCTION_KEYS) && namesFit(MODIFIERS) &&
                  namesFit(NAVIGATION) && namesFit(BASIC_KEYS) && namesFit(SYMBOLS) && namesFit(NUMPAD) &&
                  namesFit(OTHER_KEYS) && namesFit(GAMEPAD) && namesFit(MOUSE) && namesFit(MEDIA) &&
                  namesFit(LAUNCH) && namesFit(BROWSER) && namesFit(EDITING) && namesFit(BRIGHTNESS) &&
                  namesFit(SYSTEM) && namesFit(ROOT_ITEMS),
              "every action name must fit the value column (NAME_CHARS)");

bool same(const Action& a, const Action& b) {
  // Layer and None carry no code.
  if (a.type != b.type) return false;
  return a.type == ActionType::None || a.type == ActionType::Layer || a.code == b.code;
}

// Depth-first search; fills path[depth..] on success and returns the total depth.
uint8_t search(const Category& category, const Action& action, uint8_t (&path)[MAX_DEPTH], uint8_t depth,
               const Item** found) {
  if (depth >= MAX_DEPTH) return 0;
  for (uint8_t i = 0; i < category.itemCount; ++i) {
    if (same(category.items[i].action, action)) {
      path[depth] = i;
      if (found != nullptr) *found = &category.items[i];
      return depth + 1;
    }
  }
  for (uint8_t c = 0; c < category.childCount; ++c) {
    const uint8_t result = search(category.children[c], action, path, depth + 1, found);
    if (result != 0) {
      path[depth] = category.itemCount + c;
      return result;
    }
  }
  return 0;
}

}  // namespace

const Category& root() { return ROOT; }

const char* nameOf(const Action& action) {
  uint8_t path[MAX_DEPTH];
  const Item* item = nullptr;
  return search(ROOT, action, path, 0, &item) != 0 ? item->name : "?";
}

uint8_t find(const Action& action, uint8_t (&path)[MAX_DEPTH]) { return search(ROOT, action, path, 0, nullptr); }

}  // namespace ActionCatalog
