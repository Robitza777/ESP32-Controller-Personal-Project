#pragma once

// One screen of the config menu, as plain text: ConfigMenu fills it, Display draws it. The menu logic never deals
// with fonts or pixels, and Display never needs to know what a page means.

#include <stdint.h>

#include "ProfileTypes.h"

struct MenuView {
  static constexpr uint8_t VISIBLE_ROWS = 5;
  static constexpr uint8_t LINE_CHARS = 21;  // characters per line in the 6 px wide menu font

  enum class Kind : uint8_t {
    List,       // title + scrolling rows, the cursor row highlighted, a scrollbar when the list is longer
    Message,    // title + up to VISIBLE_ROWS lines (confirmations, "press a button", "Save changes?")
    TextEntry,  // title + a name in the big profile-name font, with a cursor under one character
  };

  struct Row {
    char label[LINE_CHARS + 1] = {};
    char value[LINE_CHARS + 1] = {};  // right-aligned; "< x >" when Left / Right change it
    bool opens = false;               // A opens a sub-page: drawn with a ">" at the right edge
  };

  Kind kind = Kind::List;
  char title[LINE_CHARS + 1] = {};

  // List: the visible rows. Message: one line per row, in `label`.
  Row rows[VISIBLE_ROWS];
  uint8_t rowCount = 0;
  uint8_t cursorRow = 0;   // List: highlighted row, 0..rowCount-1
  uint16_t firstItem = 0;  // List: position of rows[0] in the whole list (for the scrollbar)
  uint16_t itemCount = 0;  // List: length of the whole list

  // TextEntry
  char text[PROFILE_NAME_MAX + 1] = {};
  uint8_t textCursor = 0;
};
