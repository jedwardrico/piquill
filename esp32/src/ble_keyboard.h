// BLE (HID over GATT) keyboard client for the ESP32-S3, using NimBLE-Arduino.
//
// UNTESTED: written against the NimBLE-Arduino 2.x API from memory, without the
// library or a keyboard to build against. Expect to fix compile errors and
// details on the first build. All the BLE-specific code is in this file pair;
// the rest of the firmware only sees raw 8-byte keyboard reports via
// popReport(), which hid_keys.h turns into key events.
//
// How it works:
//  1. Scan for a device that advertises the HID service (0x1812) and either
//     matches nameFilter or is already bonded.
//  2. Connect, call secureConnection() (pairs and bonds the first time; the bond
//     is stored in flash so later connections are automatic).
//  3. Subscribe to every notifying HID Report (0x2A4D) and Boot Keyboard Input
//     (0x2A22) characteristic. Notification callbacks run in the BLE task, so
//     reports are only copied into a FreeRTOS queue there.
//  4. If the link drops, scan again.
// Put the keyboard in pairing mode the first time; after that just switch it on.
#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class BleKeyboard : public NimBLEClientCallbacks, public NimBLEScanCallbacks {
 public:
  // nameFilter: substring of the keyboard's BLE name (e.g. "Keychron"). A bonded
  // device is accepted whatever it is called. Empty string = any HID device.
  void begin(const char *nameFilter);
  // Call every loop: starts scans, connects, reconnects.
  void loop();
  bool connected() const { return connected_; }
  // Next raw input report, if any (copies up to 8 bytes into out, returns length).
  bool popReport(uint8_t out[8], size_t &len);
  // True once after each disconnect, so the caller can release held keys.
  bool takeDisconnected();
  // Forget all bonds (use if pairing gets stuck; the keyboard must be re-paired).
  void forgetBonds();

 private:
  // NimBLEScanCallbacks
  void onResult(const NimBLEAdvertisedDevice *dev) override;
  // NimBLEClientCallbacks
  void onConnect(NimBLEClient *client) override;
  void onDisconnect(NimBLEClient *client, int reason) override;

  bool connectToTarget();
  bool subscribeAll(NimBLEClient *client);
  void onNotify(NimBLERemoteCharacteristic *chr, uint8_t *data, size_t len);

  struct Report { uint8_t d[8]; uint8_t len; };

  String nameFilter_;
  QueueHandle_t queue_ = nullptr;
  NimBLEClient *client_ = nullptr;
  NimBLEAddress target_;
  volatile bool haveTarget_ = false;
  volatile bool connected_ = false;
  volatile bool disconnected_ = false;
  bool scanning_ = false;
  uint32_t lastAttemptMs_ = 0;
  uint32_t oddLengthLogged_ = 0;
};
