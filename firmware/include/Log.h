#pragma once

// LOG() prints only when CONTROLLER_DEBUG is defined (see platformio.ini); otherwise it compiles to nothing.
// Keep it out of per-scan input code: Serial blocks once its TX buffer is full.

#ifdef CONTROLLER_DEBUG
#include <Arduino.h>
#define LOG(fmt, ...) Serial.printf("[%lu] " fmt "\n", millis(), ##__VA_ARGS__)
#else
#define LOG(...) \
  do {           \
  } while (0)
#endif
