#pragma once

// HID usage IDs used by the presets. The full named catalogs for the config menu come with the menu.

#include <stdint.h>

namespace HidKey {  // Keyboard/Keypad page (0x07)
constexpr uint16_t A = 0x04;
constexpr uint16_t C = 0x06;
constexpr uint16_t D = 0x07;
constexpr uint16_t E = 0x08;
constexpr uint16_t F = 0x09;
constexpr uint16_t M = 0x10;
constexpr uint16_t Q = 0x14;
constexpr uint16_t R = 0x15;
constexpr uint16_t S = 0x16;
constexpr uint16_t V = 0x19;
constexpr uint16_t W = 0x1A;
constexpr uint16_t DIGIT_1 = 0x1E;
constexpr uint16_t DIGIT_2 = 0x1F;
constexpr uint16_t DIGIT_3 = 0x20;
constexpr uint16_t DIGIT_4 = 0x21;
constexpr uint16_t ENTER = 0x28;
constexpr uint16_t ESCAPE = 0x29;
constexpr uint16_t TAB = 0x2B;
constexpr uint16_t SPACE = 0x2C;
constexpr uint16_t F13 = 0x68;
constexpr uint16_t LEFT_CTRL = 0xE0;  // 0xE0..0xE7 are sent as modifier bits
constexpr uint16_t LEFT_SHIFT = 0xE1;
constexpr uint16_t LEFT_ALT = 0xE2;
constexpr uint16_t LEFT_GUI = 0xE3;  // Windows key
constexpr uint16_t FIRST_MODIFIER = LEFT_CTRL;
constexpr uint16_t LAST_MODIFIER = 0xE7;  // Right GUI
}  // namespace HidKey

namespace HidConsumer {  // Consumer page (0x0C)
constexpr uint16_t NEXT_TRACK = 0x0B5;
constexpr uint16_t PREVIOUS_TRACK = 0x0B6;
constexpr uint16_t PLAY_PAUSE = 0x0CD;
constexpr uint16_t MUTE = 0x0E2;
constexpr uint16_t VOLUME_UP = 0x0E9;
constexpr uint16_t VOLUME_DOWN = 0x0EA;
}  // namespace HidConsumer
