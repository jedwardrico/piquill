#include "ble_keyboard.h"

#include <string.h>

namespace {
const uint16_t UUID_HID_SERVICE = 0x1812;
const uint16_t UUID_HID_REPORT = 0x2A4D;       // Report (input/output/feature)
const uint16_t UUID_BOOT_KB_INPUT = 0x2A22;    // Boot Keyboard Input Report
}  // namespace

void BleKeyboard::begin(const char *nameFilter) {
  nameFilter_ = nameFilter ? nameFilter : "";
  queue_ = xQueueCreate(32, sizeof(Report));

  NimBLEDevice::init("PiQuill");
  // Bonding + LE Secure Connections, no passkey ("just works"). If the keyboard
  // insists on a passkey, pairing will fail and the log will say so.
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  NimBLEScan *scan = NimBLEDevice::getScan();
  scan->setScanCallbacks(this, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(80);
  Serial.printf("BLE: ready, %d bond(s) stored\n", NimBLEDevice::getNumBonds());
}

void BleKeyboard::forgetBonds() {
  NimBLEDevice::deleteAllBonds();
  Serial.println("BLE: bonds cleared");
}

void BleKeyboard::onResult(const NimBLEAdvertisedDevice *dev) {
  if (haveTarget_ || connected_) return;
  if (!dev->isAdvertisingService(NimBLEUUID(UUID_HID_SERVICE))) return;
  bool nameMatch = nameFilter_.length() == 0 ||
                   String(dev->getName().c_str()).indexOf(nameFilter_) >= 0;
  bool bonded = NimBLEDevice::isBonded(dev->getAddress());
  if (!nameMatch && !bonded) return;
  Serial.printf("BLE: found %s (%s)%s\n", dev->getName().c_str(),
                dev->getAddress().toString().c_str(), bonded ? " [bonded]" : "");
  target_ = dev->getAddress();
  haveTarget_ = true;
  NimBLEDevice::getScan()->stop();  // connect from loop(), not from this callback
}

void BleKeyboard::onConnect(NimBLEClient *) { Serial.println("BLE: connected"); }

void BleKeyboard::onDisconnect(NimBLEClient *, int reason) {
  Serial.printf("BLE: disconnected (reason %d)\n", reason);
  connected_ = false;
  disconnected_ = true;
}

bool BleKeyboard::takeDisconnected() {
  bool d = disconnected_;
  disconnected_ = false;
  return d;
}

void BleKeyboard::onNotify(NimBLERemoteCharacteristic *, uint8_t *data, size_t len) {
  // Runs in the BLE task: only copy the report out.
  if (len != 8) {
    // NKRO/consumer/other reports: not handled. Log a few so the first run
    // shows what this keyboard sends.
    if (oddLengthLogged_++ < 8) Serial.printf("BLE: ignoring %u-byte report\n", (unsigned)len);
    return;
  }
  Report r;
  memcpy(r.d, data, 8);
  r.len = 8;
  xQueueSend(queue_, &r, 0);
}

bool BleKeyboard::popReport(uint8_t out[8], size_t &len) {
  Report r;
  if (!queue_ || xQueueReceive(queue_, &r, 0) != pdTRUE) return false;
  memcpy(out, r.d, 8);
  len = r.len;
  return true;
}

bool BleKeyboard::subscribeAll(NimBLEClient *client) {
  NimBLERemoteService *hid = client->getService(NimBLEUUID(UUID_HID_SERVICE));
  if (!hid) {
    Serial.println("BLE: no HID service (0x1812) on this device");
    return false;
  }
  int subscribed = 0;
  for (NimBLERemoteCharacteristic *chr : hid->getCharacteristics(true)) {
    Serial.printf("BLE:  characteristic %s%s%s\n", chr->getUUID().toString().c_str(),
                  chr->canNotify() ? " notify" : "", chr->canRead() ? " read" : "");
    bool wanted = chr->getUUID() == NimBLEUUID(UUID_HID_REPORT) ||
                  chr->getUUID() == NimBLEUUID(UUID_BOOT_KB_INPUT);
    if (!wanted || !chr->canNotify()) continue;
    bool ok = chr->subscribe(true, [this](NimBLERemoteCharacteristic *c, uint8_t *d, size_t n, bool) {
      onNotify(c, d, n);
    });
    Serial.printf("BLE:  subscribe %s\n", ok ? "ok" : "FAILED");
    if (ok) subscribed++;
  }
  return subscribed > 0;
}

bool BleKeyboard::connectToTarget() {
  if (!client_) {
    client_ = NimBLEDevice::createClient();
    client_->setClientCallbacks(this, false);
    client_->setConnectionParams(12, 12, 0, 150);  // ~15 ms interval for low typing latency
  }
  Serial.printf("BLE: connecting to %s\n", target_.toString().c_str());
  if (!client_->connect(target_)) {
    Serial.println("BLE: connect failed");
    return false;
  }
  // Pairs and bonds on first use; on later connections the stored keys are used.
  if (!client_->secureConnection()) {
    Serial.println("BLE: security/pairing failed (is the keyboard in pairing mode?)");
    client_->disconnect();
    return false;
  }
  if (!subscribeAll(client_)) {
    client_->disconnect();
    return false;
  }
  connected_ = true;
  Serial.println("BLE: keyboard ready");
  return true;
}

void BleKeyboard::loop() {
  if (connected_) return;
  if (haveTarget_) {
    haveTarget_ = false;
    scanning_ = false;
    lastAttemptMs_ = millis();
    connectToTarget();
    return;
  }
  NimBLEScan *scan = NimBLEDevice::getScan();
  if (!scan->isScanning() && millis() - lastAttemptMs_ > 1000) {
    lastAttemptMs_ = millis();
    Serial.println("BLE: scanning...");
    scan->start(0, false, true);  // scan until stopped
  }
}
