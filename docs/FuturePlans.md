# Future Plans

Ideas and features being considered for after the first stable release (firmware 1.0.0). Nothing here is committed — this is a backlog of possibilities, not a roadmap with deadlines.

## Firmware

- Companion configuration tool (desktop or web-based) for managing profiles without using the on-device menu, and for backing up and sharing profiles.
- **Macros** (timed sequences of actions), configured through the companion tool: too fiddly to set up on the controller's small screen.
- Firmware OTA (over-the-air) update support.
- Find the cause of the occasional failed first pairing right after erasing the Bluetooth pairings (the debug build already logs the details).
- Measure the controller's current draw, to see whether light sleep between reports could save battery without making the power module cut the power.
- Verify the *Editing* (application commands) and *Brightness* consumer keys on Windows, and test other hosts (Android, Linux, macOS).

## Hardware

- Custom PCB to replace the protoboard assembly, reducing wiring complexity and improving reliability.
- Investigate alternative/additional joystick modules for comparison against the Ginfull hall-effect sticks.
- Analog triggers instead of the digital L2 / R2 buttons.
- Rumble/haptic feedback support, power budget permitting.

## Enclosure

- Iterate on the enclosure design beyond the current Alpakka-1-inspired shell, exploring ergonomic refinements specific to this hardware layout.
- Additional colorways/finishes for the 3D-printed shells.
- Finalize and publish the OLED/profile button suspended mounting platform as a proper 3D model.

## Documentation & Community

- Expand the Build Guide with step-by-step photos and wiring diagrams.
- Publish a demo video showing the profiles and the configuration menu in action.
- Open the project up for community-contributed enclosure variants (see [`CONTRIBUTING.md`](../CONTRIBUTING.md)).
