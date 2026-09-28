#pragma once

// Local change: connection diagnostics, printed like the firmware's LOG() (include/Log.h isn't visible to
// libraries). Only with CONTROLLER_DEBUG (platformio.ini).

#ifdef CONTROLLER_DEBUG
#include <Arduino.h>
#include <NimBLEDevice.h>
#define DIAG_LOG(fmt, ...) Serial.printf("[%lu] " fmt "\n", millis(), ##__VA_ARGS__)

// What the bond store holds: pairings, the key records behind them, and the hosts' saved subscriptions (a host
// that reconnects expects the device to remember which reports it subscribed to).
inline void diagLogBondStore(const char* when) {
  int ourKeys = 0, peerKeys = 0, subscriptions = 0;
  ble_store_util_count(BLE_STORE_OBJ_TYPE_OUR_SEC, &ourKeys);
  ble_store_util_count(BLE_STORE_OBJ_TYPE_PEER_SEC, &peerKeys);
  ble_store_util_count(BLE_STORE_OBJ_TYPE_CCCD, &subscriptions);
  DIAG_LOG("BLE: %s: %d bond(s), key records %d/%d, %d subscription(s) stored", when, NimBLEDevice::getNumBonds(),
           ourKeys, peerKeys, subscriptions);
}
#else
#define DIAG_LOG(...) \
  do {                \
  } while (0)
inline void diagLogBondStore(const char*) {}
#endif
