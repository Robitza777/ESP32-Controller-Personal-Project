#ifndef ESP32_KEYBOARD_DEVICE_H
#define ESP32_KEYBOARD_DEVICE_H

#include "NimBLECharacteristic.h"
#include <KeyboardHIDCodes.h>
#include <KeyboardConfiguration.h>
#include <BaseCompositeDevice.h>
#include <Callback.h>
#include <mutex>

struct KeyboardInputReport {
    uint8_t modifiers = 0x00;
    uint8_t reserved = 0x00;
    uint8_t keys[6]; // 8 bits per key * 101 keys = 6 bytes
};

// Local change: consumer usages (0 = empty slot) instead of a 24-bit key bitmask.
struct KeyboardMediaInputReport {
    uint16_t usages[MEDIA_KEY_SLOTS] = {};
};

struct KeyboardOutputReport {
    bool numLockActive;
    bool capsLockActive;
    bool scrollLockActive;
    bool composeActive;
    bool kanaActive;

    constexpr KeyboardOutputReport(uint8_t value = 0) noexcept : 
        numLockActive(value & KEY_LED_NUMLOCK),
        capsLockActive(value & KEY_LED_CAPSLOCK),
        scrollLockActive(value & KEY_LED_SCROLLLOCK),
        composeActive(value & KEY_LED_COMPOSE),
        kanaActive(value & KEY_LED_KANA)
    {}
};

// Forwards 
class KeyboardDevice;

class KeyboardCallbacks : public NimBLECharacteristicCallbacks {
public:
    KeyboardCallbacks(KeyboardDevice* device);

    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    void onStatus(NimBLECharacteristic* pCharacteristic, int code) override;
    void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override;

private:
    KeyboardDevice* _device;
};


class KeyboardDevice : public BaseCompositeDevice {
private:
    KeyboardConfiguration _config;
    NimBLECharacteristic* _input;
    NimBLECharacteristic* _mediaInput;
    NimBLECharacteristic* _systemInput;  // local change
    NimBLECharacteristic* _output;

    KeyboardInputReport _inputReport;
    KeyboardMediaInputReport _mediaKeyInputReport;
    uint8_t _systemKeys = 0;  // local change: bit 0 power down, bit 1 sleep, bit 2 wake up
    KeyboardCallbacks* _callbacks;

public:
    KeyboardDevice();
    KeyboardDevice(const KeyboardConfiguration& config);
    ~KeyboardDevice();
    
    void init(NimBLEHIDDevice* hid) override;
    const BaseCompositeDeviceConfiguration* getDeviceConfig() const override;

    void resetKeys();

    void keyPress(uint8_t keyCode);
    void keyRelease(uint8_t keyCode);
    void modifierKeyPress(uint8_t modifier);
    void modifierKeyRelease(uint8_t modifier);
    // Local change: replaces mediaKeyPress/mediaKeyRelease. Sets the held consumer usages (0 = none).
    void setMediaKeys(const uint16_t (&usages)[MEDIA_KEY_SLOTS]);
    // Local change: bit 0 = System Power Down, bit 1 = System Sleep, bit 2 = System Wake Up.
    void setSystemKeys(uint8_t keys);

    Signal<KeyboardOutputReport> onLED;

    void sendKeyReport(bool defer = false);
    void sendMediaKeyReport(bool defer = false);
    void sendSystemKeyReport(bool defer = false);  // local change

private:
    void sendKeyReportImpl();
    void sendMediaKeyReportImpl();
    void sendSystemKeyReportImpl();  // local change

    // Threading
    std::mutex _mutex;
};

#endif