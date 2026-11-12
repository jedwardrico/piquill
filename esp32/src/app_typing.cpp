// PiQuill ESP32: BLE keyboard -> text buffer -> e-paper. First integrated app.
// Untested on hardware. Build with `pio run -e typing`.
#include <Arduino.h>
#include <vector>

#include "ble_keyboard.h"
#include "config.h"
#include "epd5in76.h"
#include "hid_keys.h"
#include "text_buffer.h"
#include "text_render.h"

static const char *KEYBOARD_NAME = "Keychron";  // substring of the BLE name
static const int INPUT_STRIP_ROWS = 240;        // bottom rows refreshed while typing
static const int FULL_REFRESH_AFTER = 60;       // partial updates before a full refresh

static EPD5in76 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);
static BleKeyboard ble;
static KeyDecoder decoder;
static TextBuffer text;

// Returns true if the finished lines changed (needs a whole-screen redraw).
static bool applyKey(const KeyEvent &e) {
  if (e.mods & (MOD_CTRL | MOD_ALT | MOD_GUI)) return false;  // shortcuts: not yet
  switch (e.type) {
    case KeyType::Char:      return text.typeChar(e.ch);
    case KeyType::Tab:       return text.typeTab();
    case KeyType::Enter:     return text.enter();
    case KeyType::Backspace: return text.backspace();
    default:                 return false;  // arrows, delete etc.: not yet
  }
}

static void redraw(bool wholeScreen) {
  uint8_t *frame = epd.frame();
  if (wholeScreen) textrender::drawEditor(frame, text);
  else textrender::drawInputLine(frame, text);

  if (epd.partialCount() >= FULL_REFRESH_AFTER) {
    // clear ghosting: full refresh, then back to partial mode
    textrender::drawEditor(frame, text);
    epd.init();
    epd.display();
    epd.initPartial();
  } else if (wholeScreen) {
    epd.display();
  } else {
    epd.displayBottomStrip(INPUT_STRIP_ROWS);
  }
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
  textrender::drawEditor(epd.frame(), text);
  epd.display();

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

  if (!events.empty()) {
    bool wholeScreen = false;
    for (const KeyEvent &e : events) wholeScreen |= applyKey(e);
    // one redraw for everything that arrived while the last refresh was running
    redraw(wholeScreen);
  }
  delay(5);
}
