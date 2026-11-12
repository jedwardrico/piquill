// Good Display GDEH0576T81 (5.76", 920x680, SSD2677) driver for the ESP32.
//
// C++ port of src/waveshare_epd/epd5in76.py, which is itself a port of Good
// Display's Arduino demo. Behaviour is kept identical to the Python driver so
// the two can be compared byte for byte (the command/data stream is the same).
//
// Frame format: 1 bit per pixel, MSB first, 1 = white, row 0 at the top,
// ROW_BYTES (115) bytes per row. Draw into frame() and call display().
//
// The panel always takes a whole frame at 2 bits per pixel:
//   Full update:    each pixel is 00 (black) or 11 (white).
//   Partial update: each pixel is (previous bit, new bit), so the driver keeps
//                   the last frame it sent.
// Partial updates leave ghosting; do a full update (init + display +
// initPartial) now and then, see partialCount().
// BUSY is active low (0 = busy). Always call sleep() when finished.
//
// Not thread-safe: call it from one task only.
#pragma once

#include <Arduino.h>
#include <SPI.h>

class EPD5in76 {
 public:
  static constexpr uint16_t WIDTH = 920;
  static constexpr uint16_t HEIGHT = 680;
  static constexpr size_t ROW_BYTES = WIDTH / 8;
  static constexpr size_t FRAME_BYTES = ROW_BYTES * HEIGHT;

  EPD5in76(int8_t cs, int8_t dc, int8_t rst, int8_t busy, SPIClass &spi = SPI);
  ~EPD5in76();

  // Allocate the frame buffers (PSRAM if present) and set up the pins.
  // Returns false if memory could not be allocated.
  bool begin();

  // Mode inits. init() = full-update waveform, initPartial() = flicker-free.
  void init();
  void initPartial();

  // Drawing buffer (FRAME_BYTES long). Fill with 0xFF for a white screen.
  uint8_t *frame() { return frame_; }
  void setPixel(int x, int y, bool black);

  // Show frame() using whichever mode was last initialised.
  void display();
  // EXPERIMENTAL partial update of only send-order rows y0..y0+rows (see the
  // Python docstring). Partial mode only.
  void displayWindow(int y0, int rows, int repeat = 1);
  // Update only the bottom `rows` lines as shown on screen.
  void displayBottomStrip(int rows, int repeat = 1);
  void clear();   // fill white, using the current mode
  void sleep();   // power off + deep sleep; call init()/initPartial() to wake

  // Pick the waveform temperature code (same thresholds as the demo).
  void setTemperature(int celsius);

  // Orientation fix-ups, same meaning as the Python driver.
  bool flipX = false;
  bool flipY = true;

  uint32_t spiSpeedHz = 10000000;      // SSD2677 max clock is TBD in its datasheet
  uint32_t busyTimeoutMs = 30000;
  bool timing = false;                 // print per-update timings to Serial
  uint16_t partialCount() const { return partialCount_; }

 private:
  void reset();
  void sendCommand(uint8_t cmd);
  void sendData(uint8_t data);
  void beginBlock();
  void endBlock();
  void readBusy();
  void commonInit();
  void powerOn();
  void refresh();
  void sendTres(uint16_t rows);
  void sendFrameRows(uint16_t sendRow0, uint16_t rows);
  void sendRows(const uint8_t *newBuf, uint16_t sendRow0, uint16_t rows);
  void sendRow(const uint8_t *newRow, const uint8_t *oldRow);
  uint16_t logicalRow(uint16_t sendRow) const {
    return flipY ? (HEIGHT - 1 - sendRow) : sendRow;
  }
  void loadSendRow(const uint8_t *buf, uint16_t sendRow, uint8_t *out) const;

  SPIClass &spi_;
  int8_t cs_, dc_, rst_, busy_;
  uint8_t *frame_ = nullptr;
  uint8_t *prev_ = nullptr;
  bool prevValid_ = false;
  bool partial_ = false;
  uint8_t temperatureCode_ = 241;      // register 0xE6, 20-30 C
  uint16_t partialCount_ = 0;
  uint16_t spread_[256];
};
