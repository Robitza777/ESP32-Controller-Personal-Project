#pragma once

// One BLE connection exposing an Xbox Series gamepad (native XInput), a keyboard, media keys and a mouse,
// through ESP32-BLE-CompositeHID.
//
// send() takes the complete desired state every input cycle. Each device's report goes out only when its part
// changed, at most once per Config::MIN_REPORT_INTERVAL_US per device, so the newest state always wins and the
// link is never flooded. Axis conventions are converted here (Xbox and mouse reports use +Y = down).
//
// Threads: begin() and send() belong to the input task; isConnected(), setBatteryLevel(), forgetPairings() and
// shutdown() are safe from the UI task.

#include "ControllerTypes.h"

namespace BleOutput {

// Starts advertising. After the first pairing, the host reconnects by itself on every power-on.
void begin();

bool isConnected();

// Call every input cycle with the full state; nowUs is micros() taken at the start of the cycle.
// While disconnected it only resets its bookkeeping, so everything is re-sent after the next connection.
void send(const OutputReport& report, uint32_t nowUs);

// 0-100 %. Shown by Windows next to the device in the Bluetooth settings.
void setBatteryLevel(uint8_t percent);

// Erases every stored host pairing and drops the current connection. Each host must then remove the controller
// from its Bluetooth list and pair again. UI task.
void forgetPairings();

// Before powering off: drops the connection so the host sees the controller leave at once instead of timing out.
// UI task.
void shutdown();

}  // namespace BleOutput
