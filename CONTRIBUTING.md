# Contributing to ESP32 BLE Gamepad

Thanks for your interest in contributing! This is a hobbyist hardware + firmware project, so contributions of all kinds are welcome — bug reports, firmware improvements, hardware revisions, documentation fixes, or enclosure redesigns.

## Ways to Contribute

- **Report a bug** — firmware crashes, incorrect input mapping, BLE connection issues, etc.
- **Suggest a feature** — new profile types, remapping improvements, power management tweaks.
- **Submit firmware fixes or improvements** — via pull request.
- **Propose hardware changes** — alternative components, wiring improvements, PCB design.
- **Improve or redesign the enclosure** — new STL revisions, ergonomic improvements.
- **Improve documentation** — clarify build steps, fix errors, add diagrams.

## Reporting Issues

When opening an Issue, please include:

- A clear description of the problem or suggestion.
- Steps to reproduce (for bugs), including which firmware version/commit you're on.
- Your hardware setup, if relevant (ESP32 variant, joystick modules, power module, etc.).
- Logs, serial output, or photos where applicable.

## Submitting Pull Requests

1. Fork the repository and create a branch from `main` (or the current development branch).
2. Keep pull requests focused — one feature or fix per PR is easier to review than a large bundle of unrelated changes.
3. Follow the existing code style and modular structure (`.h` in `include/`, `.cpp` in `src/`).
4. **Test on real hardware** before submitting. This project is closely tied to specific hardware behavior (BLE timing, sensor readings, power management), so changes that only compile but haven't been tested on a physical board are much harder to review and merge safely. Please state clearly in your PR description whether the change has been tested on hardware, and on what configuration.
5. Update relevant documentation alongside code changes (see below).
6. Write clear commit messages in English, describing *what* changed and *why*.

## Hardware & Enclosure Contributions

Hardware and enclosure changes carry extra requirements, since they affect physical builds for everyone using this repo:

- **STL + source file required** — any enclosure change must include both the exported `.stl` file(s) and the original editable source (Blender `.blend`, or equivalent). STL-only submissions cannot be properly reviewed or modified by others.
- **`Pinout.md` must be updated** — any change affecting GPIO usage, wiring, or component connections must be reflected in [`hardware/pinout/Pinout.md`](hardware/pinout/Pinout.md) as part of the same PR. Undocumented pin changes are a common source of hard-to-debug issues for other builders.
- Include photos of the physical result where practical (assembled hardware, printed part, etc.).
- Clearly describe compatibility: does this change require reprinting the whole enclosure, or is it a drop-in replacement for a single part?

## What We Don't Accept

- Changes that break native XInput compatibility without an opt-in fallback.
- Enclosure changes without both STL and source files.
- Hardware/pin changes without an updated `Pinout.md`.
- Untested firmware changes presented as final/ready-to-merge.

## Code of Conduct

Be respectful and constructive. This is a hobby project maintained in personal time — patience and clear communication go a long way, both when asking for help and when reviewing others' contributions.

## Acknowledgements

Thanks to everyone who contributes fixes, ideas, hardware revisions, or testing feedback. Contributors may be credited in the project's documentation and release notes.
