#include "KeyboardDevice.h"
#include "KeyboardDescriptors.h"
#include "BleCompositeHID.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG "KeyboardDevice"
#else
#include "esp_log.h"
static const char *LOG_TAG = "KeyboardDevice";
#endif

KeyboardCallbacks::KeyboardCallbacks(KeyboardDevice* device) :
    _device(device)
{
}

void KeyboardCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo)
{
    KeyboardOutputReport ledReport = pCharacteristic->getValue<uint8_t>();
    ESP_LOGD(LOG_TAG, "KeyboardDevice::onWrite - LED Report: %d", ledReport);
    _device->onLED.fire(ledReport);
}

void KeyboardCallbacks::onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo)
{
}

void KeyboardCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue)
{
}

void KeyboardCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, int code)
{
}

KeyboardDevice::KeyboardDevice() :
    _config(KeyboardConfiguration(KEYBOARD_REPORT_ID)),
    _input(),
    _mediaInput(nullptr),
    _systemInput(nullptr),
    _output(),
    _inputReport(),
    _callbacks(nullptr)
{
    resetKeys();
}

KeyboardDevice::KeyboardDevice(const KeyboardConfiguration& config) :
    _config(config),
    _input(),
    _mediaInput(nullptr),
    _systemInput(nullptr),
    _output(),
    _inputReport(),
    _callbacks(nullptr)
{
    resetKeys();
}

KeyboardDevice::~KeyboardDevice()
{
    if (getOutput() && _callbacks){
        getOutput()->setCallbacks(nullptr);
        delete _callbacks;
        _callbacks = nullptr;
    }
}

void KeyboardDevice::init(NimBLEHIDDevice* hid)
{
    _input = hid->getInputReport(_config.getReportId());
    // Local change: only create report characteristics that exist in the report map.
    _mediaInput = _config.getUseMediaKeys() ? hid->getInputReport(MEDIA_KEYS_REPORT_ID) : nullptr;
    _systemInput = _config.getUseSystemKeys() ? hid->getInputReport(SYSTEM_KEYS_REPORT_ID) : nullptr;
    _output = hid->getOutputReport(_config.getReportId());
    _callbacks = new KeyboardCallbacks(this);
    _output->setCallbacks(_callbacks);

    setCharacteristics(_input, _output);
}

const BaseCompositeDeviceConfiguration* KeyboardDevice::getDeviceConfig() const
{
    return &_config;
}

void KeyboardDevice::resetKeys()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _inputReport.modifiers = 0x00;
    _inputReport.reserved = 0x00;
    memset(&_inputReport.keys, KEY_NONE, sizeof(_inputReport.keys));
    memset(_mediaKeyInputReport.usages, 0, sizeof(_mediaKeyInputReport.usages));
    _systemKeys = 0;

}

void KeyboardDevice::modifierKeyPress(uint8_t modifier)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _inputReport.modifiers |= modifier;
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::modifierKeyRelease(uint8_t modifier)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _inputReport.modifiers ^= modifier;
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::setMediaKeys(const uint16_t (&usages)[MEDIA_KEY_SLOTS])
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        memcpy(_mediaKeyInputReport.usages, usages, sizeof(_mediaKeyInputReport.usages));
    }

    if (_config.getAutoReport())
    {
        sendMediaKeyReport();
    }
}

void KeyboardDevice::setSystemKeys(uint8_t keys)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _systemKeys = keys;
    }

    if (_config.getAutoReport())
    {
        sendSystemKeyReport();
    }
}

void KeyboardDevice::keyPress(uint8_t keyCode)
{
    // Find the first empty slot
    bool full = true;
    for (int slotIdx = 0; slotIdx < 6; slotIdx++)
    {
        if (_inputReport.keys[slotIdx] == 0x00)
        {
            full = false;
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.keys[slotIdx] = keyCode;
            break;
        }
    }

    // If no slots are free, set the overflow flag
    if(full){
        std::lock_guard<std::mutex> lock(_mutex);
        memset(_inputReport.keys, KEY_ERR_OVF, sizeof(_inputReport.keys));
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::keyRelease(uint8_t keyCode)
{
    for (int slotIdx = 0; slotIdx < 6; slotIdx++)
    {
        if (_inputReport.keys[slotIdx] == keyCode)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _inputReport.keys[slotIdx] = 0x00;
            break;
        }
    }

    if (_config.getAutoReport())
    {
        sendKeyReport();
    }
}

void KeyboardDevice::sendKeyReport(bool defer)
{
    if(defer || _config.getAutoDefer()){
        queueDeferredReport(std::bind(&KeyboardDevice::sendKeyReportImpl, this));
    } else {
        sendKeyReportImpl();
    }
}

void KeyboardDevice::sendKeyReportImpl()
{
    auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;

    if(!parentDevice->isConnected())
        return;

    uint8_t currentReportIndex = 0;
    uint8_t m[_config.getDeviceReportSize()];
    memset(&m, 0, sizeof(m));

    // Copy key input report into buffer
    {
        std::lock_guard<std::mutex> lock(_mutex);
        memcpy(&m[currentReportIndex], &_inputReport, sizeof(_inputReport));
        input->setValue((uint8_t*)&_inputReport, sizeof(_inputReport));
    }
    input->notify();
}

void KeyboardDevice::sendMediaKeyReport(bool defer)
{
    if(defer || _config.getAutoDefer()){
        queueDeferredReport(std::bind(&KeyboardDevice::sendMediaKeyReportImpl, this));
    } else {
        sendMediaKeyReportImpl();
    }
}


void KeyboardDevice::sendMediaKeyReportImpl()
{
   auto input = getInput();
    auto parentDevice = this->getParent();

    if (!input || !parentDevice)
        return;

    if(!parentDevice->isConnected() || !_mediaInput)
        return;

    // Local change: little-endian 16-bit usages, one per slot.
    uint8_t m[MEDIA_KEY_SLOTS * 2];
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (int slot = 0; slot < MEDIA_KEY_SLOTS; slot++) {
            m[slot * 2] = _mediaKeyInputReport.usages[slot] & 0xFF;
            m[slot * 2 + 1] = (_mediaKeyInputReport.usages[slot] >> 8) & 0xFF;
        }
        _mediaInput->setValue((uint8_t*)&m, sizeof(m));
    }
    _mediaInput->notify();
}

void KeyboardDevice::sendSystemKeyReport(bool defer)
{
    if(defer || _config.getAutoDefer()){
        queueDeferredReport(std::bind(&KeyboardDevice::sendSystemKeyReportImpl, this));
    } else {
        sendSystemKeyReportImpl();
    }
}

void KeyboardDevice::sendSystemKeyReportImpl()
{
    auto parentDevice = this->getParent();
    if (!parentDevice || !parentDevice->isConnected() || !_systemInput)
        return;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _systemInput->setValue(&_systemKeys, sizeof(_systemKeys));
    }
    _systemInput->notify();
}