// PiQuill ESP32: BLE keyboard -> editor -> e-paper, with saving, formatting and
// the status panel. Untested on hardware. Build with `pio run -e typing`.
#include <Arduino.h>
#include <esp_sleep.h>

#include <vector>

#include "battery.h"
#include "ble_keyboard.h"
#include "config.h"
#include "doc_store.h"
#include "editor.h"
#include "epd5in76.h"
#include "hid_keys.h"
#include "storage_littlefs.h"
#include "ui.h"

static const char *KEYBOARD_NAME = "Keychron";   // substring of the BLE name
static const int INPUT_STRIP_ROWS = 240;         // bottom rows refreshed while typing
static const int FULL_REFRESH_AFTER = 60;        // partial updates before a full refresh
static const int LOW_BATTERY_SHUTDOWN = 5;       // percent: save and power off (when not charging)
static const uint32_t BATTERY_POLL_MS = 30000;

static EPD5in76 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);
static BleKeyboard ble;
static KeyDecoder decoder;
static LittleFsStorage fs;
static DocStore docs(fs);
static Editor editor(docs, [] { return (uint32_t)millis(); });
static Battery battery;
static bool lowBatteryShown = false;
static uint32_t lastBatteryPoll = 0;

static BatteryInfo batteryInfo() {
  BatteryInfo b;
  b.present = battery.present();
  b.percent = battery.percent();
  b.charging = battery.charging();
  return b;
}

// Save, show a goodbye message, and go to deep sleep. Press RESET to start again.
static void powerDown(const char *msg) {
  editor.saveNow();
  ui::renderMessage(epd.frame(), msg);
  epd.display();
  delay(3000);
  epd.sleep();
  esp_deep_sleep_start();
}

static void redraw(Redraw r) {
  if (r == Redraw::None) return;
  uint8_t *frame = epd.frame();
  if (r == Redraw::Input) {
    ui::renderInput(frame, editor, batteryInfo());
    epd.displayBottomStrip(INPUT_STRIP_ROWS);
    return;
  }
  ui::renderDisplay(frame, editor, batteryInfo());
  if (r == Redraw::DisplayFull || epd.partialCount() >= FULL_REFRESH_AFTER) {
    // clear ghosting: full refresh, then back to partial mode
    epd.init();
    epd.display();
    epd.initPartial();
  } else {
    epd.display();
  }
}

static Redraw maxRedraw(Redraw a, Redraw b) { return (int)a >= (int)b ? a : b; }

// Warn once when the battery runs low; power off before it runs out.
static Redraw checkBattery() {
  if (!battery.present() || battery.charging()) {
    lowBatteryShown = false;
    return Redraw::None;
  }
  int pct = battery.percent();
  if (pct <= LOW_BATTERY_SHUTDOWN) {
    powerDown("Battery low. Powered Down.");
  } else if (pct <= ui::LOW_BATTERY_WARN && !lowBatteryShown) {
    lowBatteryShown = true;
    return Redraw::Display;  // redraw so "LOW" appears
  } else if (pct > ui::LOW_BATTERY_WARN) {
    lowBatteryShown = false;
  }
  return Redraw::None;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("PiQuill ESP32 typing app");

  epd.timing = true;
  if (!epd.begin()) {
    Serial.println("Could not allocate frame buffers");
    while (true) delay(1000);
  }
  epd.init();
  epd.clear();
  epd.initPartial();

  if (!fs.begin()) Serial.println("Could not start the file system: nothing will be saved");
  battery.begin();
  editor.begin();
  Serial.printf("Restored %d lines, document '%s', battery %s %d%%\n", (int)editor.text().lines().size(),
                editor.docName().c_str(), battery.present() ? "ok" : "not found", battery.percent());

  redraw(Redraw::Display);
  ble.begin(KEYBOARD_NAME);
}

void loop() {
  ble.loop();
  if (ble.takeDisconnected()) decoder.releaseAll();

  std::vector<KeyEvent> events;
  uint8_t report[8];
  size_t len;
  while (ble.popReport(report, len)) {
    if (!decoder.onReport(report, len, millis(), events)) Serial.printf("bad report len %u\n", (unsigned)len);
  }
  decoder.tick(millis(), events);

  // Handle everything that arrived (e.g. while the last refresh was running),
  // then redraw once with the strongest redraw any key asked for.
  Redraw r = Redraw::None;
  for (const KeyEvent &e : events) r = maxRedraw(r, editor.onKey(e));
  r = maxRedraw(r, editor.tick());

  if (editor.powerDownRequested()) powerDown("PiQuill Powered Down.");

  if (millis() - lastBatteryPoll > BATTERY_POLL_MS) {
    lastBatteryPoll = millis();
    battery.poll();
    r = maxRedraw(r, checkBattery());
  }

  redraw(r);
  delay(5);
}
