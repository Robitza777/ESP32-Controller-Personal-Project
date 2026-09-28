#pragma once

// Every action a button or stick direction can be set to, named and grouped the way the config menu offers them.
// Constant tables in flash.
//
//   None
//   RTZ layer        while held, the RTZ layer is active (Toggle mode locks it)
//   Gamepad          A B X Y, LB RB, LT RT, View Menu Guide Share, LS RS (stick clicks), D-pad 4 directions
//   Keyboard      >  Letters, Numbers, F1-F24, Modifiers (Ctrl Shift Alt Win, left/right),
//                    Navigation (arrows, Home End PgUp PgDn Ins Del), Basic keys (Enter Esc Tab Space...),
//                    Symbols, Numpad, Other (every remaining key 0x04-0xE7: international, language, keypad extras...)
//   Mouse            Left Right Middle Back Forward, Scroll up / down / left / right
//   Media & system > Media & volume, Launch apps, Browser, Editing, Brightness, System (power, sleep, wake)

#include "ProfileTypes.h"

namespace ActionCatalog {

constexpr uint8_t NAME_CHARS = 12;  // characters, so a name fits the value column of a menu row

struct Item {
  const char* name;
  Action action;
};

// A menu list: its items come first, then its sub-categories. Position i in that combined list is item i when
// i < itemCount, else child i - itemCount.
struct Category {
  const char* name;
  const Item* items;
  uint8_t itemCount;
  const Category* children;
  uint8_t childCount;

  uint8_t size() const { return itemCount + childCount; }
};

const Category& root();

// Short name of an action for menu rows ("Xbox A", "Left Ctrl", "Volume +"): "None" for None, "?" for a code
// that isn't in the catalog (it could only come from a profile saved by a newer firmware).
const char* nameOf(const Action& action);

// Where `action` sits in the tree: the position chosen at each level, starting at the root, so a list can open with
// the cursor on the current value. Returns the number of levels, 0 if the action isn't in the catalog.
constexpr uint8_t MAX_DEPTH = 3;
uint8_t find(const Action& action, uint8_t (&path)[MAX_DEPTH]);

}  // namespace ActionCatalog
