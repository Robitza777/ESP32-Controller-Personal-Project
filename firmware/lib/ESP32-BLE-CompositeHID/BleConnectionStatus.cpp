#include "BleConnectionStatus.h"
#include "BLEHostConfiguration.h"
#include <NimBLEDevice.h>
#include "DiagLog.h"  // Local change: connection and security diagnostics

BleConnectionStatus::BleConnectionStatus(void) : _configuration(nullptr)
{
}

void BleConnectionStatus::setConfiguration(const BLEHostConfiguration* config)
{
    _configuration = config;
}

void BleConnectionStatus::onConnect(NimBLEServer *pServer, NimBLEConnInfo& connInfo)
{
    DIAG_LOG("BLE: host %s connected, %s", connInfo.getAddress().toString().c_str(),
        NimBLEDevice::isBonded(connInfo.getIdAddress()) ? "bond found" : "no bond for it");

    uint16_t minInterval = 6;
    uint16_t maxInterval = 7;
    uint16_t latency = 0;
    uint16_t timeout = 600;

    if (_configuration) {
        minInterval = _configuration->getMinConnectionInterval();
        maxInterval = _configuration->getMaxConnectionInterval();
        latency = _configuration->getSlaveLatency();
        timeout = _configuration->getSupervisionTimeout();
    }

    pServer->updateConnParams(connInfo.getConnHandle(), minInterval, maxInterval, latency, timeout);
}

void BleConnectionStatus::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason)
{
    this->connected = false;
    // NimBLE reason = 0x200 + HCI code: 0x213 host closed it, 0x208 link lost, 0x206 key missing, 0x23D MIC failure.
    DIAG_LOG("BLE: host %s disconnected, reason 0x%03X", connInfo.getAddress().toString().c_str(), reason);
}

bool BleConnectionStatus::isConnected(){
    return this->connected;
}

void BleConnectionStatus::onAuthenticationComplete(NimBLEConnInfo& connInfo)
{
    // Local change: NimBLE calls this for failed encryption too (e.g. the host has keys this device lost). The host's
    // HID driver can't read an unencrypted link, so only an encrypted one counts as connected.
    this->connected = connInfo.isEncrypted();
    DIAG_LOG("BLE: encryption %s (bonded: %s, %u-byte key)", connInfo.isEncrypted() ? "OK" : "FAILED",
        connInfo.isBonded() ? "yes" : "no", connInfo.getSecKeySize());
    diagLogBondStore("after encryption");
}
