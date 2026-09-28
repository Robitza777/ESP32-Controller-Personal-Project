#ifndef KEYBOARDDESCRIPTORS_H
#define KEYBOARDDESCRIPTORS_H

#include <HIDTypes.h>
#include "KeyboardHIDCodes.h"  // local change: MEDIA_KEY_SLOTS

#define KEYBOARD_REPORT_ID 0x40
#define MEDIA_KEYS_REPORT_ID 0x43
#define SYSTEM_KEYS_REPORT_ID 0x44  // local change: System Control collection

static const uint8_t _keyboardHIDReportDescriptor[] = {
  // Input
  USAGE_PAGE(1),      0x01,                     // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,                     // USAGE (Keyboard)
  COLLECTION(1),      0x01,                     // COLLECTION (Application)
  REPORT_ID(1),       KEYBOARD_REPORT_ID,       //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,                     //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,                     //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,                     //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,                     //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,                     //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,                     //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,                     //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,                     //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position) ; Modifier byte
  REPORT_COUNT(1),    0x01,                     //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,                     //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,                     //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position) ; Reserved byte
  REPORT_COUNT(1),    0x06,                     //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,                     //   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1), 0x00,                     //   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(2), 0xE7, 0x00,               //   LOGICAL_MAXIMUM(0xE7) ; local change: full key range (F13-F24, intl, ...)
  USAGE_PAGE(1),      0x07,                     //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,                     //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0xE7,                     //   USAGE_MAXIMUM (0xE7)
  HIDINPUT(1),        0x00,                     //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position) ; Key arrays (6 bytes)
  // Output
  REPORT_COUNT(1),    0x05,                     //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,                     //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,                     //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,                     //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,                     //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,                     //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,                     //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,                     //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,                     //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  END_COLLECTION(0),                            // END_COLLECTION
};

// Local change: any consumer-page usage 0x000-0x3FF (media, volume, app launch, browser, brightness...),
// MEDIA_KEY_SLOTS at a time. 25 bytes instead of the original 78-byte bitmask.
static const uint8_t _mediakeysHIDReportDescriptor[] = {
  USAGE_PAGE(1),      0x0C,                         // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,                         // USAGE (Consumer Control)
  COLLECTION(1),      0x01,                         // COLLECTION (Application)
  REPORT_ID(1),       MEDIA_KEYS_REPORT_ID,         //   REPORT_ID
  LOGICAL_MINIMUM(1), 0x00,                         //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(2), 0xFF, 0x03,                   //   LOGICAL_MAXIMUM (0x3FF)
  USAGE_MINIMUM(1),   0x00,                         //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(2),   0xFF, 0x03,                   //   USAGE_MAXIMUM (0x3FF)
  REPORT_SIZE(1),     0x10,                         //   REPORT_SIZE (16)
  REPORT_COUNT(1),    MEDIA_KEY_SLOTS,              //   REPORT_COUNT
  HIDINPUT(1),        0x00,                         //   INPUT (Data,Array,Abs)
  END_COLLECTION(0)                                 // END_COLLECTION
};

// Local change: System Control (power down, sleep, wake up), one bit each.
static const uint8_t _systemKeysHIDReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,                         // USAGE_PAGE (Generic Desktop)
  USAGE(1),           0x80,                         // USAGE (System Control)
  COLLECTION(1),      0x01,                         // COLLECTION (Application)
  REPORT_ID(1),       SYSTEM_KEYS_REPORT_ID,        //   REPORT_ID
  LOGICAL_MINIMUM(1), 0x00,                         //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,                         //   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,                         //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x03,                         //   REPORT_COUNT (3)
  USAGE(1),           0x81,                         //   USAGE (System Power Down)  bit 0
  USAGE(1),           0x82,                         //   USAGE (System Sleep)       bit 1
  USAGE(1),           0x83,                         //   USAGE (System Wake Up)     bit 2
  HIDINPUT(1),        0x02,                         //   INPUT (Data,Var,Abs)
  REPORT_COUNT(1),    0x01,                         //   REPORT_COUNT (1)
  REPORT_SIZE(1),     0x05,                         //   REPORT_SIZE (5)
  HIDINPUT(1),        0x01,                         //   INPUT (Const) ; padding
  END_COLLECTION(0)                                 // END_COLLECTION
};


#endif // KEYBOARDDESCRIPTORS_H
