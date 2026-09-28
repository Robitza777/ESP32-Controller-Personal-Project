#pragma once

// The on-controller configuration menu. UI task only.
//
// It edits a copy of the profile list and of the controller settings; the copies replace the real ones only if the
// user saves on the way out. Display settings are previewed live (main applies the copy while the menu is open).
//
//   D-pad / left stick   move (held: repeats like a keyboard key; the stick repeats faster the further it's pushed)
//   Left / Right         change the value of the highlighted row (numbers, modes, Base / RTZ layer)
//   A                    open / choose            B   back
//   L1 / R1              jump a page in long lists
//   hold Profile         leave the menu (Config::PROFILE_HOLD_MS). With unsaved changes it asks first:
//                        A = save, B = discard, Profile = keep editing
//
// Pages:
//   Edit <profile>   Buttons (chosen by pressing them; Profile = back) -> layer, actions 1-3, mode
//                    Left / Right stick -> layer, mode, speed, 4 direction actions
//                    Deadzone L / R, Turbo speed, Rename, Copy base layer to RTZ
//   New profile      from a preset or a copy of a profile, then name it; it becomes the active profile
//   Profiles         pick one -> Edit, Move up, Move down, Delete
//   Controller       Display (brightness, screen off after, name popup), Auto off (connected / offline),
//                    Recalibrate sticks, Forget Bluetooth pairings, Reset all profiles to the presets, About
//   Exit
// Action lists come from ActionCatalog, None first. While the menu is open main sends the host a neutral report.

#include "ControllerTypes.h"
#include "MenuView.h"
#include "ProfileTypes.h"

namespace ConfigMenu {

// Everything the menu edits. The menu owns it from open() until update() returns Save or Discard.
struct Workspace {
  Profile profiles[MAX_PROFILES];
  uint8_t count = 0;
  uint8_t active = 0;         // the profile to be active once the menu closes
  uint16_t changedSlots = 0;  // bit i: profiles[i] must be written to flash on save
  bool countChanged = false;
  DisplaySettings display;
  bool displayChanged = false;
  PowerSettings power;
  bool powerChanged = false;
};

// What the input task saw since the previous update().
struct Input {
  uint32_t held = 0;     // Button bits down now
  uint32_t pressed = 0;  // Button bits that went down since the previous update, even if already released
  int16_t leftX = 0;     // left stick after the deadzone, +X right, +Y up
  int16_t leftY = 0;
  bool screenWasOff = false;  // this input only wakes the screen: whatever is held now is ignored until released
};

enum class Request : uint8_t {
  None,
  ForgetPairings,  // "Forget Bluetooth pairings" was confirmed: erase them now; the menu stays open
  RecalibrateSticks,  // the user let go of the sticks and confirmed: measure their centers now; the menu stays open
  Save,            // menu closed: write what changed to flash, then hand the profiles to Profiles
  Discard,         // menu closed: the profiles stay as they were
};

// Shows the main page and starts editing `workspace`, which the caller has filled with the current profiles and
// controller settings.
void open(Workspace& workspace, uint32_t nowMs);

// Call every UI cycle while the menu is open.
Request update(const Input& input, uint32_t nowMs);

// The screen to draw after update().
const MenuView& view();

}  // namespace ConfigMenu
