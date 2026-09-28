# User Guide

How to use the controller with firmware **1.0.0**: switching it on and off, pairing, the status screen, profiles, the configuration menu, and what to do when something doesn't work.

---

## Controls

| Control | Location |
|---|---|
| A / B / X / Y | Face buttons (down / right / left / up) |
| D-pad | Up / Down / Left / Right |
| L1 / R1 | Shoulder buttons |
| L2 / R2 | Triggers (digital: either released or fully pressed) |
| L3 / R3 | Stick clicks |
| L4 / R4 | Back paddles |
| Options | Middle button |
| Profile | Square button next to the screen: switches profiles and opens the configuration menu |
| On/off switch | Square button next to the screen: switches the power on and off |

What each button does depends on the active profile (see [Built-in Presets](#built-in-presets)). Only the **Profile** button is fixed.

---

## Turning On and Off

* **On:** press the on/off switch. Keep your hands off the sticks for the first moment: the controller measures their resting position at power-on. Don't hold any button while switching on, since several buttons sit on ESP32 boot pins.
* **Off:** press the on/off switch again. It cuts the power immediately, so unsaved changes in the configuration menu are lost.
* **Automatic power-off:** after a period without input (5 minutes by default) the controller counts down for 10 seconds and switches itself off. Any button cancels the countdown. See [Power and Battery](#power-and-battery).
* **After an automatic power-off**, the on/off switch is still in the "on" position: switch it off, then on again.

---

## Pairing

The controller has been tested with **Windows 11**.

1. Switch the controller on.
2. In Windows, open **Settings → Bluetooth & devices → Add device → Bluetooth**.
3. Select **Robitza's ESP32 Gamepad**.

Windows installs it as an Xbox controller, a keyboard and a mouse at once. From then on, the controller reconnects by itself every time it is switched on, and its battery level is shown next to it in the Bluetooth settings.

To pair it from scratch, use **Controller → Forget Bluetooth** in the configuration menu, then remove the controller from the Windows Bluetooth list and pair it again.

---

## Status Screen

```
 ᛒ connected          ⚡ 78% [▮▮▮▯]

             GAMEPAD
              1 / 4
                                 RTZ
```

* **Top left:** Bluetooth status, `connected` or `disconnected`.
* **Top right:** battery percentage and a 4-bar indicator. A lightning bolt appears while charging (it can take a minute or two to show up after plugging in the charger).
* **Middle:** the name of the active profile and its position in the profile list.
* **Bottom right:** `RTZ` while the RTZ layer is active, `TOGGLE` while a Toggle-mode button is switched on.

After a profile switch, the new profile's name is shown full screen for about a second. The screen switches off after 30 seconds without input (adjustable in **Controller → Display**). Any button, or a stick pushed past about a quarter of its travel, wakes it up; small stick movements are ignored.

---

## Profile Button

| Gesture | Result |
|---|---|
| Short press | Next profile |
| Double press | Back to the previous profile (repeat to toggle between the two) |
| Hold Profile + press A / B / X / Y | Jump to profile 1 / 2 / 3 / 4 |
| Hold for 3 seconds (nothing else pressed) | Open the configuration menu |

Buttons pressed while the Profile button is held are never sent to the computer, so the shortcuts can't trigger anything in a game.

The controller remembers the active profile and starts on it after the next power-on.

---

## Built-in Presets

On first boot the profile list contains one profile per preset, in the order of the table: GAMEPAD is profile 1, HYBRID is profile 4. The presets also serve as templates for new profiles, and **Reset all profiles** brings this list back.

| Input | GAMEPAD | KB+MOUSE | MEDIA | HYBRID: RTZ layer (R4 held) |
|---|---|---|---|---|
| A | Xbox A | Space | Left click | Enter |
| B | Xbox B | Left Ctrl | Right click | Esc |
| X | Xbox X | R | Middle click | — |
| Y | Xbox Y | E | Enter | — |
| D-pad Up | D-pad Up | 1 | Volume + | Volume + |
| D-pad Right | D-pad Right | 2 | Next track | Next track |
| D-pad Down | D-pad Down | 3 | Volume − | Volume − |
| D-pad Left | D-pad Left | 4 | Previous track | Previous track |
| L1 | LB | Q | Mouse Back | — |
| R1 | RB | F | Mouse Forward | — |
| L2 | LT | Right click | Alt+Tab | Right click |
| R2 | RT | Left click | Win+D | Left click |
| L3 | Left stick click | Left Shift | Mute | — |
| R3 | Right stick click | V | Esc | — |
| L4 | View | Tab | Ctrl+C | — |
| R4 | Xbox Guide | M | Ctrl+V | *held: activates this layer* |
| Options | Menu | Esc | Play / Pause | Play / Pause |
| Left stick | Xbox left stick | W / A / S / D | Mouse | Scroll |
| Right stick | Xbox right stick | Mouse | Scroll | Mouse |

* **GAMEPAD** — a regular Xbox controller. The controller has only one middle button, so View and the Guide button sit on the back paddles.
* **KB+MOUSE** — for PC games without controller support (FPS layout).
* **MEDIA** — for using the computer from the couch.
* **HYBRID** — works exactly like GAMEPAD, except for R4: while R4 is held, the controller switches to the layer in the last column, to operate the computer without leaving the game profile. A dash (—) means the button does nothing while the layer is active.

All presets start with a 5% stick deadzone, a turbo speed of 10 presses per second and a mouse / scroll speed of 5.

---

## Profiles

The controller holds up to **16 profiles**, each with a name of up to 10 characters. Everything in a profile can be changed in the configuration menu:

* **Actions** — each button sends up to three actions at the same time (for example Ctrl + C, or LB + RB, or an Xbox button plus a keyboard key). Actions come from these lists:

  | List | Contents |
  |---|---|
  | None | The button does nothing |
  | RTZ layer | Activates the RTZ layer (see below) |
  | Gamepad | A, B, X, Y, LB, RB, LT, RT, View, Menu, Guide, Share, stick clicks, D-pad |
  | Keyboard | Letters, Numbers, F1–F24, Modifiers, Navigation, Basic keys, Symbols, Numpad, Other (every remaining key) |
  | Mouse | Left, Right, Middle, Back, Forward, scroll up / down / left / right |
  | Media & system | Media & volume, Launch apps (media player, mail, calculator, File Explorer), Browser, Editing, Brightness, System (power down, sleep, wake up) |

  Share is available, but games don't see it. The *Editing* keys (Copy, Paste, Undo...) only work in applications that support them, and the *Brightness* keys only on laptops and displays that support them.
* **Button mode** — **Normal** (the actions follow the button), **Toggle** (one press switches the actions on, the next one off) or **Turbo** (while held, the actions repeat at the profile's turbo speed, 1–20 per second).
* **RTZ layer** ("Robitza-Shift") — every profile has a second set of mappings, the RTZ layer. A button set to *RTZ layer* activates it while held; in Toggle mode, it locks the layer on until the next press. On the RTZ layer, *None* means the button is disabled while the layer is active.
* **Stick mode** — *Xbox L*, *Xbox R*, *Mouse*, *Scroll* (vertical and horizontal), *4-way* (each direction sends its own actions, for example W / A / S / D or the arrow keys) or *None*. Mouse and Scroll have a speed setting (1–10); the mouse moves slowly near the center for precision and fast at full deflection.
* **Deadzone** — per stick, 0–50%. Stick movement inside the deadzone is ignored.

---

## Configuration Menu

Hold the **Profile** button for 3 seconds to open the menu. While the menu is open, the computer receives no input from the controller.

### Navigation

| Control | Action |
|---|---|
| D-pad / left stick | Move the cursor (held: repeats; the stick repeats faster the further it's pushed) |
| Left / Right | Change a value shown as `< value >` |
| A | Open / select |
| B | Back |
| L1 / R1 | Jump a page in long lists |
| Profile (short press) | Back |
| Profile (hold 3 s) | Leave the menu |

* A `>` at the end of a row opens a page; a row showing a value opens a list of choices.
* In a list, `*` marks the current choice.
* A fresh press at the end of a list wraps around to the other end; a held direction stops at the end.
* If the screen is off, the first press only wakes it up.

### Menu Pages

```
MENU
├── Edit <profile>
│   ├── Buttons ........... press the button to edit → Layer, Action 1–3, Mode
│   ├── Left stick ........ Layer, Mode, Speed or the 4 direction actions
│   ├── Right stick ....... Layer, Mode, Speed or the 4 direction actions
│   ├── Deadzone L / R .... 0–50%
│   ├── Turbo speed ....... 1–20 Hz
│   ├── Rename
│   └── Copy base to RTZ .. replaces the RTZ layer with a copy of the base layer
├── New profile ........... from a preset or a copy of a profile, then name it
├── Profiles .............. pick one → Edit, Move up, Move down, Delete
├── Controller
│   ├── Display ........... Brightness 1–10, Screen off, Name popup
│   ├── Auto off .......... Connected, Offline
│   ├── Recalibrate sticks
│   ├── Forget Bluetooth
│   ├── Reset all profiles
│   └── About ............. firmware version
└── Exit
```

* **Edit** always opens on the active profile. To edit another one, pick it on the **Profiles** page.
* **Buttons:** press the button you want to edit on the controller itself. On this screen B is also a button you may want to edit, so a short press of **Profile** goes back.
* **Layer** (on the button and stick pages) switches between the base layer and the RTZ layer; the rows below show the selected layer.
* **New profile** becomes the active profile and opens its Edit page. There is room for 16 profiles.
* **Delete** asks for confirmation. The last remaining profile can't be deleted.
* **Display** changes are shown immediately, so you can see the brightness while adjusting it.
* **Recalibrate sticks:** let go of both sticks, then press A. Useful if a stick was touched while the controller was switched on.
* **Forget Bluetooth** erases all pairings immediately (after a confirmation). Remove the controller in Windows and pair it again.
* **Reset all profiles** replaces every profile with the four presets.

### Entering a Name

| Control | Action |
|---|---|
| Up / Down | Change the character under the cursor |
| Left / Right | Move the cursor |
| L1 / R1 | Jump to the previous / next character group (A–Z, a–z, 0–9, symbols) |
| X | Delete the character (the rest moves left) |
| Y | Insert a space |
| A | Done |
| B | Cancel |

An empty name becomes `PROFILE`.

### Saving

All changes are made on a copy. When you leave the menu (hold **Profile** for 3 seconds, or select **Exit**) and something has changed, the menu asks **Save changes?**:

* **A** — save and leave.
* **B** — discard the changes and leave.
* **Profile** — keep editing.

Deleting a profile and resetting all profiles are also only kept if you save. **Forget Bluetooth** and **Recalibrate sticks** take effect immediately.

---

## Power and Battery

### Automatic Power-Off

**Controller → Auto off** sets how long the controller waits without input before switching itself off:

* **Connected** — while connected to a computer.
* **Offline** — while not connected.

Each can be 1, 2, 3, 5, 10, 15, 30 or 60 minutes, or **Never**. Both are 5 minutes by default.

Any button counts as input, including a button that is held down (for example a held accelerator in a racing game). A stick only counts when pushed past about a quarter of its travel, so a stick resting slightly off-center doesn't keep the controller awake.

During the last 10 seconds, the screen shows a countdown; any button cancels it. The power-off also happens while the menu is open, in which case unsaved changes are lost.

When it powers off, the controller disconnects from the computer, switches the screen off and goes to sleep. About 30 seconds later the power module cuts the power completely, so the battery is not drained. To switch the controller on again, switch it off and on with the on/off switch.

### Battery Warnings

* **5% or less:** a full-screen *Battery critical* warning for a few seconds, repeated every minute. Charge the controller.
* **2% or less:** a *Battery empty* message, then the controller switches itself off to protect the battery.

Neither happens while the controller is charging.

---

## Troubleshooting

| Problem | Solution |
|---|---|
| The controller doesn't switch on after an automatic power-off | Switch it off, then on again with the on/off switch. |
| Connected, but the gamepad doesn't work, or Windows shows a phone or computer icon | Remove the controller in the Windows Bluetooth settings and pair it again. Right after **Forget Bluetooth**, it can take one or two re-pairs before the pairing sticks. |
| A browser gamepad tester doesn't see the controller after re-pairing | Reload the page: Windows creates a new device after a re-pair. |
| A stick drifts or doesn't rest at zero | Use **Controller → Recalibrate sticks** with both sticks released, or switch the controller off and on without touching the sticks. |
| The screen doesn't switch off | A button is being held (for example pressed by the charging cable or the enclosure). |
| The controller switches off or restarts randomly | Check that the ESP32 board is pushed fully into its headers: a loose 5 V pin causes exactly this. |
| Something else | Flash the debug build and read the serial log (see [`firmware/README.md`](../firmware/README.md)). |
