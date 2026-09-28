#include "Display.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "Config.h"
#include "Log.h"

namespace Display {
namespace {

constexpr int WIDTH = 128;
constexpr int HEIGHT = 64;
constexpr int MARGIN = 2;

// Full-buffer mode: the whole frame is drawn in RAM (1 KB) and sent in one go. U8G2_R2 = mounted upside down.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R2, U8X8_PIN_NONE);
bool present = false;

// One size for every profile name (user): monospace, 12 px per character, so the longest possible name
// (PROFILE_NAME_MAX = 10 characters, 120 px) still fits and short names look the same.
const uint8_t* const NAME_FONT = u8g2_font_profont22_tr;
const uint8_t* const SMALL_FONT = u8g2_font_5x8_tr;
const uint8_t* const MENU_FONT = u8g2_font_6x10_tr;  // 6 px wide: MenuView::LINE_CHARS = 21 characters per row

// Menu layout: a title line with a rule under it, then MenuView::VISIBLE_ROWS rows.
constexpr int MENU_ROW_H = 10;
constexpr int MENU_BASELINE = 8;  // inside a row
constexpr int MENU_TOP = 12;
constexpr int SCROLLBAR_W = 2;
static_assert(MENU_TOP + MenuView::VISIBLE_ROWS * MENU_ROW_H <= HEIGHT, "menu rows must fit the screen");
static_assert(MenuView::LINE_CHARS * 6 <= WIDTH, "a menu line must fit the screen");

// 5x9 Bluetooth rune and 5x8 lightning bolt, XBM (LSB = leftmost pixel).
constexpr uint8_t BLUETOOTH_BITS[] = {0x04, 0x0C, 0x15, 0x0E, 0x04, 0x0E, 0x15, 0x0C, 0x04};
constexpr uint8_t BOLT_BITS[] = {0x18, 0x0C, 0x06, 0x1F, 0x0C, 0x06, 0x03, 0x01};

// What is on the panel now; redraw only when this changes.
struct Shown {
  char profileName[PROFILE_NAME_MAX + 1];
  uint8_t profileNumber;
  uint8_t profileCount;
  bool layerActive;
  bool toggleActive;
  bool bleConnected;
  bool batteryPresent;
  uint8_t batteryPercent;
  bool charging;
  bool popup;
  bool asleep;
};

Shown shown;
bool shownValid = false;  // `shown` is what the panel shows (false after the menu was up)
MenuView shownMenu;
bool menuShown = false;   // `shownMenu` is what the panel shows
bool panelAsleep = false;
bool panelStateKnown = false;

// The alert on the panel, if any.
struct ShownAlert {
  char top[MenuView::LINE_CHARS + 1];
  char big[MenuView::LINE_CHARS + 1];
  char bottom[MenuView::LINE_CHARS + 1];
};
ShownAlert shownAlert;
bool alertShown = false;
uint32_t lastProfileSwitches = 0;
uint32_t popupUntilMs = 0;
DisplaySettings settings;

uint8_t contrastOf(const DisplaySettings& s) {
  const uint8_t level = constrain(s.brightness, 1, Config::DISPLAY_BRIGHTNESS_LEVELS);
  return Config::DISPLAY_CONTRAST_LEVELS[level - 1];
}

// Signed: lastInputMs comes from the other task and can be a little newer than nowMs.
bool timedOut(uint32_t lastInputMs, uint32_t nowMs) {
  const uint16_t timeoutS = Config::DISPLAY_SLEEP_OPTIONS_S[settings.screenOff < Config::DISPLAY_SLEEP_OPTION_COUNT
                                                                 ? settings.screenOff
                                                                 : 0];
  return timeoutS != 0 && static_cast<int32_t>(nowMs - lastInputMs) > static_cast<int32_t>(timeoutS * 1000u);
}

bool deviceAnswers(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

uint8_t batteryBars(uint8_t percent) {
  uint8_t bars = 0;
  for (uint8_t threshold : Config::BATTERY_BAR_THRESHOLDS) {
    if (percent >= threshold) ++bars;
  }
  return bars;
}

void drawCentered(const char* text, int baselineY) { oled.drawStr((WIDTH - oled.getStrWidth(text)) / 2, baselineY, text); }

// Top-right: [bolt] 78% [battery with 4 bars]
void drawBattery(const Shown& s) {
  // 1 px frame, 1 px gap, 4 bars of 3 px with 1 px between them, 1 px gap, 1 px frame = 19 px wide.
  constexpr int BODY_W = 19, BODY_H = 9, NUB_W = 2;
  constexpr int bodyX = WIDTH - NUB_W - BODY_W;
  oled.drawFrame(bodyX, 0, BODY_W, BODY_H);
  oled.drawBox(bodyX + BODY_W, 2, NUB_W, BODY_H - 4);

  char text[6];
  if (s.batteryPresent) {
    const uint8_t bars = batteryBars(s.batteryPercent);
    for (uint8_t i = 0; i < bars; ++i) oled.drawBox(bodyX + 2 + i * 4, 2, 3, BODY_H - 4);
    snprintf(text, sizeof(text), "%u%%", s.batteryPercent);
  } else {
    snprintf(text, sizeof(text), "--%%");
  }
  oled.setFont(SMALL_FONT);
  const int textX = bodyX - 3 - oled.getStrWidth(text);
  oled.drawStr(textX, 8, text);
  if (s.charging) oled.drawXBM(textX - 7, 0, 5, 8, BOLT_BITS);
}

void drawStatusScreen(const Shown& s) {
  oled.drawXBM(0, 0, 5, 9, BLUETOOTH_BITS);
  oled.setFont(SMALL_FONT);
  oled.drawStr(8, 8, s.bleConnected ? "connected" : "disconnected");
  drawBattery(s);

  oled.setFont(NAME_FONT);
  drawCentered(s.profileName, 34);

  char number[8];
  snprintf(number, sizeof(number), "%u / %u", s.profileNumber, s.profileCount);
  oled.setFont(SMALL_FONT);
  drawCentered(number, 50);

  // RTZ ("Robitza-Shift") is the device's name for the second layer.
  const char* tag = s.layerActive && s.toggleActive ? "RTZ TOGGLE"
                    : s.layerActive                 ? "RTZ"
                    : s.toggleActive                ? "TOGGLE"
                                                    : nullptr;
  if (tag != nullptr) {
    const int w = oled.getStrWidth(tag) + 4;
    oled.drawBox(WIDTH - w, HEIGHT - 10, w, 10);
    oled.setDrawColor(0);
    oled.drawStr(WIDTH - w + 2, HEIGHT - 2, tag);
    oled.setDrawColor(1);
  }
}

void drawPopup(const Shown& s) {
  oled.setFont(NAME_FONT);
  drawCentered(s.profileName, (HEIGHT + oled.getAscent()) / 2);
}

void setAsleep(bool asleep) {
  if (panelStateKnown && asleep == panelAsleep) return;
  oled.setPowerSave(asleep ? 1 : 0);
  panelAsleep = asleep;
  panelStateKnown = true;
}

bool sameRow(const MenuView::Row& a, const MenuView::Row& b) {
  return a.opens == b.opens && strcmp(a.label, b.label) == 0 && strcmp(a.value, b.value) == 0;
}

bool sameView(const MenuView& a, const MenuView& b) {
  if (a.kind != b.kind || a.rowCount != b.rowCount || a.cursorRow != b.cursorRow || a.firstItem != b.firstItem ||
      a.itemCount != b.itemCount || a.textCursor != b.textCursor || strcmp(a.title, b.title) != 0 ||
      strcmp(a.text, b.text) != 0) {
    return false;
  }
  for (uint8_t i = 0; i < a.rowCount && i < MenuView::VISIBLE_ROWS; ++i) {
    if (!sameRow(a.rows[i], b.rows[i])) return false;
  }
  return true;
}

// Label on the left, value right-aligned, ">" at the far right when A opens something. The label is cut off
// before it can run into the value.
void drawMenuRow(const MenuView::Row& row, int y, int right, bool highlighted) {
  if (highlighted) {
    oled.drawBox(0, y, right, MENU_ROW_H);
    oled.setDrawColor(0);
  }
  int valueRight = right - 1;
  if (row.opens) {
    oled.drawStr(right - 6, y + MENU_BASELINE, ">");
    valueRight = right - 8;
  }
  int labelLimit = valueRight;
  if (row.value[0] != '\0') {
    const int valueX = valueRight - oled.getStrWidth(row.value);
    oled.drawStr(valueX, y + MENU_BASELINE, row.value);
    labelLimit = valueX - 3;
  }
  oled.setClipWindow(0, y, labelLimit, y + MENU_ROW_H);
  oled.drawStr(1, y + MENU_BASELINE, row.label);
  oled.setMaxClipWindow();
  oled.setDrawColor(1);
}

void drawMenu(const MenuView& view) {
  oled.setFont(MENU_FONT);
  oled.drawStr(1, MENU_BASELINE, view.title);
  oled.drawHLine(0, MENU_TOP - 2, WIDTH);

  const uint8_t rows = view.rowCount < MenuView::VISIBLE_ROWS ? view.rowCount : MenuView::VISIBLE_ROWS;
  switch (view.kind) {
    case MenuView::Kind::List: {
      const bool scrollbar = view.itemCount > MenuView::VISIBLE_ROWS;
      const int right = scrollbar ? WIDTH - SCROLLBAR_W - 1 : WIDTH;
      for (uint8_t i = 0; i < rows; ++i) {
        drawMenuRow(view.rows[i], MENU_TOP + i * MENU_ROW_H, right, i == view.cursorRow);
      }
      if (scrollbar) {
        constexpr int trackH = MenuView::VISIBLE_ROWS * MENU_ROW_H;
        const int thumbH = max(4, trackH * MenuView::VISIBLE_ROWS / view.itemCount);
        const int maxFirst = view.itemCount - MenuView::VISIBLE_ROWS;
        const int thumbY = MENU_TOP + (trackH - thumbH) * min<int>(view.firstItem, maxFirst) / maxFirst;
        oled.drawBox(WIDTH - SCROLLBAR_W, thumbY, SCROLLBAR_W, thumbH);
      }
      break;
    }
    case MenuView::Kind::Message:
      for (uint8_t i = 0; i < rows; ++i) drawMenuRow(view.rows[i], MENU_TOP + i * MENU_ROW_H, WIDTH, false);
      break;
    case MenuView::Kind::TextEntry: {
      // The name in the same font as on the status screen, so it looks exactly as it will there.
      oled.setFont(NAME_FONT);
      const int charW = oled.getMaxCharWidth();
      const int textX = (WIDTH - PROFILE_NAME_MAX * charW) / 2;
      oled.drawStr(textX, 34, view.text);
      oled.drawBox(textX + view.textCursor * charW, 37, charW - 1, 2);
      oled.setFont(SMALL_FONT);
      for (uint8_t i = 0; i < rows && i < 2; ++i) {
        oled.drawStr(1, 50 + i * 9, view.rows[i].label);
        oled.drawStr(WIDTH - 1 - oled.getStrWidth(view.rows[i].value), 50 + i * 9, view.rows[i].value);
      }
      break;
    }
  }
}

}  // namespace

void begin() {
  present = deviceAnswers(Config::OLED_ADDRESS);
  LOG("Display: SSD1306 %s", present ? "found" : "NOT found");
  if (!present) return;

  oled.setI2CAddress(Config::OLED_ADDRESS << 1);  // U8g2 takes the 8-bit address
  oled.setBusClock(Config::I2C_FREQUENCY_HZ);
  oled.begin();
  oled.setContrast(contrastOf(settings));

  oled.setFont(NAME_FONT);
  if (PROFILE_NAME_MAX * oled.getMaxCharWidth() > WIDTH - 2 * MARGIN) {
    LOG("Display: WARNING, a %u-character profile name won't fit with this font", PROFILE_NAME_MAX);
  }
}

void update(const State& state, uint32_t nowMs) {
  if (!present) return;

  if (state.profileSwitches != lastProfileSwitches) {
    lastProfileSwitches = state.profileSwitches;
    if (settings.namePopup) popupUntilMs = nowMs + Config::PROFILE_POPUP_MS;
  }
  const bool popup = static_cast<int32_t>(popupUntilMs - nowMs) > 0;
  const bool asleep = !popup && timedOut(state.lastInputMs, nowMs);

  Shown next;
  memset(&next, 0, sizeof(next));  // padding too, so memcmp works
  memcpy(next.profileName, state.profileName, sizeof(next.profileName));
  next.profileNumber = state.profileNumber;
  next.profileCount = state.profileCount;
  next.layerActive = state.layerActive;
  next.toggleActive = state.toggleActive;
  next.bleConnected = state.bleConnected;
  next.batteryPresent = state.battery.present;
  next.batteryPercent = state.battery.percent;
  next.charging = state.battery.present && state.battery.charging;
  next.popup = popup;
  next.asleep = asleep;

  menuShown = false;
  alertShown = false;
  if (shownValid && memcmp(&next, &shown, sizeof(next)) == 0) return;

  setAsleep(next.asleep);
  if (!next.asleep) {
    oled.clearBuffer();
    if (next.popup) drawPopup(next);
    else drawStatusScreen(next);
    oled.sendBuffer();
  }
  shown = next;
  shownValid = true;
}

void showMenu(const MenuView& view, uint32_t lastInputMs, uint32_t nowMs) {
  if (!present) return;
  shownValid = false;  // the status screen must be redrawn in full when it comes back
  alertShown = false;
  const bool asleep = timedOut(lastInputMs, nowMs);
  if (menuShown && asleep == panelAsleep && sameView(view, shownMenu)) return;

  setAsleep(asleep);
  if (!asleep) {
    oled.clearBuffer();
    drawMenu(view);
    oled.sendBuffer();
  }
  shownMenu = view;
  menuShown = true;
}

bool asleep() { return present && panelStateKnown && panelAsleep; }

void showAlert(const char* top, const char* big, const char* bottom) {
  if (!present) return;
  // Whatever comes back afterwards is drawn in full.
  shownValid = false;
  menuShown = false;

  ShownAlert next;
  memset(&next, 0, sizeof(next));
  snprintf(next.top, sizeof(next.top), "%s", top);
  snprintf(next.big, sizeof(next.big), "%s", big);
  snprintf(next.bottom, sizeof(next.bottom), "%s", bottom);
  if (alertShown && !panelAsleep && memcmp(&next, &shownAlert, sizeof(next)) == 0) return;

  setAsleep(false);
  oled.clearBuffer();
  oled.setFont(MENU_FONT);
  drawCentered(next.top, 12);
  drawCentered(next.bottom, 60);
  oled.setFont(NAME_FONT);
  constexpr int BIG_BASELINE = 42;
  drawCentered(next.big, BIG_BASELINE);
  // A big "!" on each side marks it as a warning.
  oled.drawStr(MARGIN, BIG_BASELINE, "!");
  oled.drawStr(WIDTH - MARGIN - oled.getStrWidth("!"), BIG_BASELINE, "!");
  oled.sendBuffer();

  shownAlert = next;
  alertShown = true;
}

void powerOff() {
  if (!present) return;
  oled.setPowerSave(1);
  panelAsleep = true;
  panelStateKnown = true;
}

void applySettings(const DisplaySettings& next) {
  if (present && contrastOf(next) != contrastOf(settings)) oled.setContrast(contrastOf(next));
  if (!next.namePopup) popupUntilMs = 0;
  settings = next;
}

}  // namespace Display
