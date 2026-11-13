// PiQuill ESP32: BLE keyboard -> editor -> e-paper, with saving, formatting and
// the status panel. Untested on hardware. Build with `pio run -e typing`.
#include <Arduino.h>
#include <driver/gpio.h>
#include <esp_sleep.h>

#include <vector>

#include "battery.h"
#include "ble_keyboard.h"
#include "config.h"
#include "doc_store.h"
#include "editor.h"
#include "epd5in76.h"
#include "hid_keys.h"
#include "power_button.h"
#include "settings.h"
#include "storage_littlefs.h"
#include "time_sync.h"
#include "ui.h"

static const char *KEYBOARD_NAME = "Keychron";   // substring of the BLE name
static const int INPUT_STRIP_ROWS = 240;         // bottom rows refreshed while typing
static const int FULL_REFRESH_AFTER = 60;        // partial updates before a full refresh
static const int LOW_BATTERY_SHUTDOWN = 5;       // percent: save and power off (when not charging)
static const uint32_t BATTERY_POLL_MS = 30000;
static const uint32_t WAKE_HOLD_MS = 300;        // after a button wake, the button must stay down this long
static const uint32_t RELEASE_WAIT_MS = 10000;   // max wait for the button to be released before sleeping

static EPD5in76 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);
static BleKeyboard ble;
static KeyDecoder decoder;
static LittleFsStorage fs;
static DocStore docs(fs);
static Editor editor(docs, [] { return (uint32_t)millis(); });
static Battery battery;
static PowerButton button;
static bool lowBatteryShown = false;
static uint32_t lastBatteryPoll = 0;

static BatteryInfo batteryInfo() {
  BatteryInfo b;
  b.present = battery.present();
  b.percent = battery.percent();
  b.charging = battery.charging();
  return b;
}

static bool buttonDown() { return digitalRead(POWER_BUTTON_PIN) == LOW; }

// Switch the keyboard's supply (low-side N-FET gate). Off is held through deep
// sleep so the gate stays low and the keyboard draws nothing.
static void keyboardPower(bool on) {
  if (KEYBOARD_EN_PIN < 0) return;  // keyboard wired straight to the pack
  gpio_hold_dis((gpio_num_t)KEYBOARD_EN_PIN);
  pinMode(KEYBOARD_EN_PIN, OUTPUT);
  digitalWrite(KEYBOARD_EN_PIN, on ? HIGH : LOW);
  if (!on) gpio_hold_en((gpio_num_t)KEYBOARD_EN_PIN);
}

// Save, show a goodbye message, cut the keyboard's power and go to deep sleep.
// The power button wakes the board again.
static void powerDown(const char *msg) {
  editor.saveNow();
  ui::renderMessage(epd.frame(), msg);
  epd.display();
  delay(3000);
  epd.sleep();
  // The wake is level-triggered: if the button is still held we would wake at once.
  uint32_t t0 = millis();
  while (buttonDown() && millis() - t0 < RELEASE_WAIT_MS) delay(10);
  keyboardPower(false);
  gpio_deep_sleep_hold_en();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BUTTON_PIN, 0);  // wake when pulled low
  esp_deep_sleep_start();
}

// After waking from the button, only carry on if it is held for WAKE_HOLD_MS;
// otherwise it was a glitch, so go straight back to sleep.
static void checkWakeHold() {
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) return;
  uint32_t t0 = millis();
  while (millis() - t0 < WAKE_HOLD_MS) {
    if (!buttonDown()) {
      esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BUTTON_PIN, 0);
      gpio_deep_sleep_hold_en();  // keep the keyboard off (hold survives the wake)
      esp_deep_sleep_start();
    }
    delay(5);
  }
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

  pinMode(POWER_BUTTON_PIN, INPUT);  // external 10 k pull-up to 3V3
  checkWakeHold();
  gpio_deep_sleep_hold_dis();  // release the holds set before sleeping
  keyboardPower(true);

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
  docs.setClock([] { return time(nullptr); });
  editor.begin();
  Serial.printf("Restored %d lines, document '%s', battery %s %d%%\n", (int)editor.text().lines().size(),
                editor.docName().c_str(), battery.present() ? "ok" : "not found", battery.percent());

  // Woken (or reset) with a nearly flat battery: show the message and sleep again.
  if (battery.present() && !battery.charging() && battery.percent() <= LOW_BATTERY_SHUTDOWN)
    powerDown("Battery low. Powered Down.");

  redraw(Redraw::Display);

  // Set the clock over Wi-Fi if /config.txt has a network (blocks up to ~15 s),
  // then turn Wi-Fi off before BLE starts: they share the radio.
  std::string cfg;
  Settings settings;
  if (fs.readFile("/config.txt", cfg)) settings = parseSettings(cfg);
  else Serial.println("No /config.txt: clock will not be set (see config.example.txt)");
  syncTime(settings);

  ble.begin(KEYBOARD_NAME);

  // If the wake press is still down, wait for release so it is not read as a long press.
  uint32_t t0 = millis();
  while (buttonDown() && millis() - t0 < 5000) delay(10);
  button.update(false, millis());
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
  if (button.update(buttonDown(), millis()) == ButtonEvent::LongPress) powerDown("PiQuill Powered Down.");

  if (millis() - lastBatteryPoll > BATTERY_POLL_MS) {
    lastBatteryPoll = millis();
    battery.poll();
    r = maxRedraw(r, checkBattery());
  }

  redraw(r);
  delay(5);
}
