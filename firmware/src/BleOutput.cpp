#include "BleOutput.h"

#include <Arduino.h>
#include <BleCompositeHID.h>
#include <KeyboardDevice.h>
#include <MouseDevice.h>
#include <NimBLEDevice.h>
#include <XboxGamepadDevice.h>

#include "Config.h"
#include "Log.h"

static_assert(TRIGGER_MAX == XBOX_TRIGGER_MAX, "trigger range must match the Xbox report");
static_assert(MAX_CONSUMER_KEYS == MEDIA_KEY_SLOTS, "consumer slots must match the HID descriptor");
static_assert(MAX_KEYS == sizeof(KeyboardInputReport::keys), "key slots must match the HID keyboard report");

namespace BleOutput {
namespace {

BleCompositeHID compositeHid(Config::BLE_DEVICE_NAME, Config::BLE_MANUFACTURER, 100);
XboxGamepadDevice* gamepad = nullptr;
KeyboardDevice* keyboard = nullptr;
MouseDevice* mouse = nullptr;

// Indexed by XboxButton. Share isn't part of the button bitfield and is sent separately.
constexpr uint16_t XBOX_BUTTON_MASKS[] = {
  XBOX_BUTTON_A,  XBOX_BUTTON_B,      XBOX_BUTTON_X,     XBOX_BUTTON_Y,    XBOX_BUTTON_LB, XBOX_BUTTON_RB,
  XBOX_BUTTON_SELECT, XBOX_BUTTON_START, XBOX_BUTTON_HOME, XBOX_BUTTON_LS, XBOX_BUTTON_RS, 0,
};
static_assert(sizeof(XBOX_BUTTON_MASKS) / sizeof(XBOX_BUTTON_MASKS[0]) == static_cast<size_t>(XboxButton::Count),
              "one mask per XboxButton");

// Indexed by DpadDirection.
constexpr uint8_t DPAD_FLAGS[] = {XboxDpadFlags::NORTH, XboxDpadFlags::SOUTH, XboxDpadFlags::WEST, XboxDpadFlags::EAST};
static_assert(sizeof(DPAD_FLAGS) == static_cast<size_t>(DpadDirection::Count), "one flag per DpadDirection");

// Indexed by MouseButton; the library numbers mouse buttons from 1.
constexpr uint8_t MOUSE_BUTTON_NUMBERS[] = {MOUSE_LOGICAL_LEFT_BUTTON, MOUSE_LOGICAL_RIGHT_BUTTON, MOUSE_LOGICAL_BUTTON_3,
                                            MOUSE_LOGICAL_BUTTON_4, MOUSE_LOGICAL_BUTTON_5};
static_assert(sizeof(MOUSE_BUTTON_NUMBERS) == static_cast<size_t>(MouseButton::Count), "one number per MouseButton");

// One per HID device, so a busy mouse can't hold back a button press on the gamepad.
struct Channel {
  uint32_t lastSendUs = 0;

  bool ready(uint32_t nowUs) const { return nowUs - lastSendUs >= Config::MIN_REPORT_INTERVAL_US; }
};

Channel gamepadChannel, keyboardChannel, consumerChannel, systemChannel, mouseChannel;

// What the host has been told so far.
GamepadReport sentGamepad;
KeyboardReport sentKeyboard;
uint16_t sentConsumer[MAX_CONSUMER_KEYS] = {};
uint8_t sentSystem = 0;
uint8_t sentMouseButtons = 0;

// Mouse motion not sent yet (fractions of a count, or more than one report can carry).
float pendingMoveX = 0.0f, pendingMoveY = 0.0f, pendingWheel = 0.0f, pendingPan = 0.0f;

bool wasConnected = false;

bool sameGamepad(const GamepadReport& a, const GamepadReport& b) {
  return a.buttons == b.buttons && a.dpad == b.dpad && a.leftX == b.leftX && a.leftY == b.leftY &&
         a.rightX == b.rightX && a.rightY == b.rightY && a.leftTrigger == b.leftTrigger &&
         a.rightTrigger == b.rightTrigger;
}

bool sameKeyboard(const KeyboardReport& a, const KeyboardReport& b) {
  return a.modifiers == b.modifiers && memcmp(a.keys, b.keys, sizeof(a.keys)) == 0;
}

void resetSentState() {
  sentGamepad = GamepadReport{};
  sentKeyboard = KeyboardReport{};
  memset(sentConsumer, 0, sizeof(sentConsumer));
  sentSystem = 0;
  sentMouseButtons = 0;
  pendingMoveX = pendingMoveY = pendingWheel = pendingPan = 0.0f;
}

void sendGamepad(const GamepadReport& report) {
  uint16_t buttonMask = 0;
  for (uint8_t i = 0; i < static_cast<uint8_t>(XboxButton::Count); ++i) {
    if (report.buttons & (1u << i)) buttonMask |= XBOX_BUTTON_MASKS[i];
  }
  uint8_t dpadFlags = XboxDpadFlags::NONE;
  for (uint8_t i = 0; i < static_cast<uint8_t>(DpadDirection::Count); ++i) {
    if (report.dpad & (1u << i)) dpadFlags |= DPAD_FLAGS[i];
  }

  gamepad->resetInputs();
  gamepad->press(buttonMask);
  if (report.buttons & (1u << static_cast<uint8_t>(XboxButton::Share))) gamepad->pressShare();
  if (dpadFlags != XboxDpadFlags::NONE) gamepad->pressDPadDirectionFlag(static_cast<XboxDpadFlags>(dpadFlags));
  // The Xbox BLE report uses the HID convention, +Y = down.
  gamepad->setLeftThumb(report.leftX, static_cast<int16_t>(-report.leftY));
  gamepad->setRightThumb(report.rightX, static_cast<int16_t>(-report.rightY));
  gamepad->setTriggers(report.leftTrigger, report.rightTrigger);
  gamepad->sendGamepadReport();
}

void sendKeyboard(const KeyboardReport& report) {
  // resetKeys() also clears the library's consumer/system state; that's fine because those are always set in full
  // right before they are sent.
  keyboard->resetKeys();
  keyboard->modifierKeyPress(report.modifiers);
  for (uint8_t key : report.keys) {
    if (key != 0) keyboard->keyPress(key);
  }
  keyboard->sendKeyReport();
}

// When motion stops, the leftover fraction is dropped so a released stick never produces a late extra count;
// whole counts still pending (e.g. a scroll step from a button) are kept until the next report carries them.
void accumulate(float& pending, float delta) { pending = delta == 0.0f ? truncf(pending) : pending + delta; }

// Whole counts to send now; the rest waits for the next report. The library can't send -127.
int8_t takeWhole(float& pending) {
  const float whole = constrain(truncf(pending), -126.0f, 126.0f);
  pending -= whole;
  return static_cast<int8_t>(whole);
}

void sendMouse(uint8_t buttons) {
  mouse->resetButtons();
  for (uint8_t i = 0; i < static_cast<uint8_t>(MouseButton::Count); ++i) {
    if (buttons & (1u << i)) mouse->mousePress(MOUSE_BUTTON_NUMBERS[i]);
  }
  const int8_t x = takeWhole(pendingMoveX);
  const int8_t y = takeWhole(pendingMoveY);
  const int8_t wheel = takeWhole(pendingWheel);
  const int8_t pan = takeWhole(pendingPan);
  mouse->mouseMove(x, static_cast<int8_t>(-y), pan, wheel);  // HID mouse: +Y = down
  mouse->sendMouseReport();
}

bool hasWholeCount(float pending) { return fabsf(pending) >= 1.0f; }

void disconnectAll() {
  NimBLEServer* server = NimBLEDevice::getServer();
  if (server == nullptr) return;
  uint16_t handles[CONFIG_BT_NIMBLE_MAX_CONNECTIONS];
  uint8_t count = 0;
  for (uint8_t i = 0; i < server->getConnectedCount() && count < CONFIG_BT_NIMBLE_MAX_CONNECTIONS; ++i) {
    handles[count++] = server->getPeerInfo(i).getConnHandle();
  }
  for (uint8_t i = 0; i < count; ++i) server->disconnect(handles[i]);
}

#ifdef CONTROLLER_DEBUG
// NimBLE reports here when a pairing can't be stored; its default handler then drops an older host's pairing.
class BondStoreLogger : public NimBLEDeviceCallbacks {
  int onStoreStatus(ble_store_status_event* event, void* arg) override {
    const bool overflow = event->event_code == BLE_STORE_EVENT_OVERFLOW;
    LOG("BLE: bond store %s, object type %d (1/2 keys, 3 subscriptions)", overflow ? "OVERFLOW" : "full",
        overflow ? event->overflow.obj_type : event->full.obj_type);
    return ble_store_util_status_rr(event, arg);
  }
};
BondStoreLogger bondStoreLogger;

void logReportMapSize(const XboxGamepadDeviceConfiguration& xbox, const KeyboardConfiguration& keyboardConfig,
                      const MouseConfiguration& mouseConfig) {
  static uint8_t scratch[BLE_ATT_ATTR_MAX_LEN];
  const size_t total = xbox.makeDeviceReport(scratch, sizeof(scratch)) +
                       keyboardConfig.makeDeviceReport(scratch, sizeof(scratch)) +
                       mouseConfig.makeDeviceReport(scratch, sizeof(scratch));
  LOG("BleOutput: HID report map %u / %u bytes%s", total, BLE_ATT_ATTR_MAX_LEN,
      total > BLE_ATT_ATTR_MAX_LEN ? " -- TOO BIG, Windows will reject the device" : "");
}
#endif

}  // namespace

void begin() {
  auto* xboxConfig = new XboxSeriesXControllerDeviceConfiguration();  // the gamepad keeps this pointer
  xboxConfig->setAutoReport(false);
  gamepad = new XboxGamepadDevice(xboxConfig);

  KeyboardConfiguration keyboardConfig;
  keyboardConfig.setAutoReport(false);
  keyboardConfig.setUseMediaKeys(true);
  keyboardConfig.setUseSystemKeys(true);
  keyboard = new KeyboardDevice(keyboardConfig);

  MouseConfiguration mouseConfig;
  mouseConfig.setAutoReport(false);
  mouse = new MouseDevice(mouseConfig);

#ifdef CONTROLLER_DEBUG
  logReportMapSize(*xboxConfig, keyboardConfig, mouseConfig);
  NimBLEDevice::setDeviceCallbacks(&bondStoreLogger);
#endif

  // The Xbox device must come first in the report map for Windows to bind its XInput driver.
  compositeHid.addDevice(gamepad);
  compositeHid.addDevice(keyboard);
  compositeHid.addDevice(mouse);

  BLEHostConfiguration hostConfig = xboxConfig->getIdealHostConfiguration();
  hostConfig.setMinConnectionInterval(Config::BLE_CONNECTION_INTERVAL);
  hostConfig.setMaxConnectionInterval(Config::BLE_CONNECTION_INTERVAL);
  compositeHid.begin(hostConfig);
}

bool isConnected() { return compositeHid.isConnected(); }

void forgetPairings() {
  NimBLEDevice::deleteAllBonds();
  // The link stays encrypted with the old keys until it drops; drop it now so the controller advertises for a
  // new pairing.
  disconnectAll();
  LOG("BLE: all pairings erased");
}

void shutdown() {
  NimBLEServer* server = NimBLEDevice::getServer();
  if (server != nullptr) server->advertiseOnDisconnect(false);
  disconnectAll();
  NimBLEDevice::stopAdvertising();
  LOG("BLE: disconnected for power-off");
}

void send(const OutputReport& report, uint32_t nowUs) {
  const bool connected = compositeHid.isConnected();
  if (connected != wasConnected) {
    wasConnected = connected;
    resetSentState();
    LOG("BleOutput: %s", connected ? "connected" : "disconnected, advertising");
  }
  if (!connected) return;

  if (!sameGamepad(report.gamepad, sentGamepad) && gamepadChannel.ready(nowUs)) {
    sendGamepad(report.gamepad);
    sentGamepad = report.gamepad;
    gamepadChannel.lastSendUs = nowUs;
  }

  if (!sameKeyboard(report.keyboard, sentKeyboard) && keyboardChannel.ready(nowUs)) {
    sendKeyboard(report.keyboard);
    sentKeyboard = report.keyboard;
    keyboardChannel.lastSendUs = nowUs;
  }

  if (memcmp(report.consumer, sentConsumer, sizeof(sentConsumer)) != 0 && consumerChannel.ready(nowUs)) {
    keyboard->setMediaKeys(report.consumer);
    keyboard->sendMediaKeyReport();
    memcpy(sentConsumer, report.consumer, sizeof(sentConsumer));
    consumerChannel.lastSendUs = nowUs;
  }

  if (report.system != sentSystem && systemChannel.ready(nowUs)) {
    keyboard->setSystemKeys(report.system);
    keyboard->sendSystemKeyReport();
    sentSystem = report.system;
    systemChannel.lastSendUs = nowUs;
  }

  const MouseReport& m = report.mouse;
  accumulate(pendingMoveX, m.moveX);
  accumulate(pendingMoveY, m.moveY);
  accumulate(pendingWheel, m.wheel);
  accumulate(pendingPan, m.pan);
  const bool moved = hasWholeCount(pendingMoveX) || hasWholeCount(pendingMoveY) || hasWholeCount(pendingWheel) ||
                     hasWholeCount(pendingPan);
  if ((moved || m.buttons != sentMouseButtons) && mouseChannel.ready(nowUs)) {
    sendMouse(m.buttons);
    sentMouseButtons = m.buttons;
    mouseChannel.lastSendUs = nowUs;
  }
}

void setBatteryLevel(uint8_t percent) { compositeHid.setBatteryLevel(min<uint8_t>(percent, 100)); }

}  // namespace BleOutput
