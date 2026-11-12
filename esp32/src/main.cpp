// PiQuill ESP32 firmware: display bring-up test.
//
// Draws a test pattern with a full update, then moves a box with partial
// updates, then puts the panel to sleep. Open the serial monitor (115200) to
// see timings. Untested on hardware.
#include <Arduino.h>

#include "config.h"
#include "epd5in76.h"

EPD5in76 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);

static void fillRect(int x, int y, int w, int h, bool black) {
  for (int j = y; j < y + h; j++)
    for (int i = x; i < x + w; i++) epd.setPixel(i, j, black);
}

static void drawPattern() {
  memset(epd.frame(), 0xFF, EPD5in76::FRAME_BYTES);
  for (int i = 0; i < EPD5in76::WIDTH; i++) {  // border
    for (int t = 0; t < 4; t++) {
      epd.setPixel(i, t, true);
      epd.setPixel(i, EPD5in76::HEIGHT - 1 - t, true);
    }
  }
  for (int j = 0; j < EPD5in76::HEIGHT; j++) {
    for (int t = 0; t < 4; t++) {
      epd.setPixel(t, j, true);
      epd.setPixel(EPD5in76::WIDTH - 1 - t, j, true);
    }
  }
  for (int i = 0; i < EPD5in76::HEIGHT; i++) {  // diagonal, top-left to bottom-right
    epd.setPixel(i * EPD5in76::WIDTH / EPD5in76::HEIGHT, i, true);
  }
  fillRect(40, 40, 120, 60, true);  // marks the TOP-LEFT of the image
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("PiQuill ESP32 display test");

  epd.timing = true;
  if (!epd.begin()) {
    Serial.println("Could not allocate frame buffers");
    return;
  }

  // If the picture is upside down or mirrored, toggle these.
  // epd.flipX = true;
  // epd.flipY = false;

  epd.init();
  drawPattern();
  epd.display();

  epd.initPartial();
  for (int step = 0; step < 5; step++) {
    fillRect(300 + step * 60, 300, 40, 40, true);
    epd.display();
  }

  epd.sleep();
  Serial.println("done");
}

void loop() {
  delay(1000);
}
