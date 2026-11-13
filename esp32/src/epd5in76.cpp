#include "epd5in76.h"

#include <string.h>

namespace {
uint8_t rev8(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}
}  // namespace

EPD5in76::EPD5in76(int8_t cs, int8_t dc, int8_t rst, int8_t busy, SPIClass &spi)
    : spi_(spi), cs_(cs), dc_(dc), rst_(rst), busy_(busy) {
  // bit i from the top of a byte -> bit (14 - 2i) of a 16-bit word
  for (int b = 0; b < 256; b++) {
    uint16_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((b >> (7 - i)) & 1) << (14 - 2 * i);
    spread_[b] = v;
  }
}

EPD5in76::~EPD5in76() {
  free(frame_);
  free(prev_);
}

bool EPD5in76::begin() {
  if (!frame_) {
    frame_ = (uint8_t *)(psramFound() ? ps_malloc(FRAME_BYTES) : malloc(FRAME_BYTES));
    prev_ = (uint8_t *)(psramFound() ? ps_malloc(FRAME_BYTES) : malloc(FRAME_BYTES));
    if (!frame_ || !prev_) {
      free(frame_);
      free(prev_);
      frame_ = prev_ = nullptr;
      return false;
    }
    memset(frame_, 0xFF, FRAME_BYTES);
    memset(prev_, 0xFF, FRAME_BYTES);
    prevValid_ = false;
  }
  pinMode(cs_, OUTPUT);
  pinMode(dc_, OUTPUT);
  pinMode(rst_, OUTPUT);
  pinMode(busy_, INPUT);
  digitalWrite(cs_, HIGH);
  spi_.begin();  // default SCK/MOSI of the board variant
  return true;
}

void EPD5in76::setPixel(int x, int y, bool black) {
  if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
  uint8_t mask = 0x80 >> (x & 7);
  uint8_t &b = frame_[y * ROW_BYTES + (x >> 3)];
  if (black) b &= ~mask; else b |= mask;
}

// --- low level ----------------------------------------------------------

void EPD5in76::reset() {
  delay(10);
  digitalWrite(rst_, LOW);
  delay(10);
  digitalWrite(rst_, HIGH);
  delay(10);
}

void EPD5in76::sendCommand(uint8_t cmd) {
  spi_.beginTransaction(SPISettings(spiSpeedHz, MSBFIRST, SPI_MODE0));
  digitalWrite(dc_, LOW);
  digitalWrite(cs_, LOW);
  spi_.transfer(cmd);
  digitalWrite(cs_, HIGH);
  spi_.endTransaction();
}

void EPD5in76::sendData(uint8_t data) {
  spi_.beginTransaction(SPISettings(spiSpeedHz, MSBFIRST, SPI_MODE0));
  digitalWrite(dc_, HIGH);
  digitalWrite(cs_, LOW);
  spi_.transfer(data);
  digitalWrite(cs_, HIGH);
  spi_.endTransaction();
}

// One CS-low data transfer made of several writeBytes() calls.
void EPD5in76::beginBlock() {
  spi_.beginTransaction(SPISettings(spiSpeedHz, MSBFIRST, SPI_MODE0));
  digitalWrite(dc_, HIGH);
  digitalWrite(cs_, LOW);
}

void EPD5in76::endBlock() {
  digitalWrite(cs_, HIGH);
  spi_.endTransaction();
}

void EPD5in76::readBusy() {
  // BUSY_N: 0 = busy, 1 = ready
  uint32_t start = millis();
  while (digitalRead(busy_) == LOW) {
    if (millis() - start > busyTimeoutMs) {
      Serial.printf("e-Paper BUSY still low after %lu ms, continuing\n",
                    (unsigned long)busyTimeoutMs);
      return;
    }
    delay(5);
  }
}

void EPD5in76::setTemperature(int celsius) {
  if (celsius <= 5) temperatureCode_ = 232;
  else if (celsius <= 10) temperatureCode_ = 235;
  else if (celsius <= 20) temperatureCode_ = 238;
  else if (celsius <= 30) temperatureCode_ = 241;
  else temperatureCode_ = 244;
}

int EPD5in76::readTemperature() {
  sendCommand(0x40);
  readBusy();  // wait for the panel to release the sensor value

  // Take the pins away from the SPI peripheral and clock 8 bits in by hand.
  spi_.end();
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, INPUT);  // SDA turns around to panel -> MCU
  digitalWrite(SCK, LOW);
  digitalWrite(dc_, HIGH);
  digitalWrite(cs_, LOW);
  uint8_t v = 0;
  for (int i = 0; i < 8; i++) {
    digitalWrite(SCK, LOW);
    delayMicroseconds(2);
    v = (v << 1) | (digitalRead(MOSI) ? 1 : 0);
    digitalWrite(SCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(SCK, LOW);
  }
  digitalWrite(cs_, HIGH);
  pinMode(MOSI, OUTPUT);
  spi_.begin();

  // Same as the demo: 0..127 is positive, above that is negative (two's complement).
  lastTemp_ = v > 127 ? (int)v - 256 : (int)v;
  return lastTemp_;
}

void EPD5in76::commonInit() {
  reset();
  readBusy();

  sendCommand(0x00);  // PSR
  sendData(0x27);
  sendData(0x0E);
  readBusy();

  sendCommand(0x06);  // BTST
  sendData(0x0F);
  sendData(0x8B);
  sendData(0x9C);
  sendData(0xC1);

  sendCommand(0xE7);
  sendData(0xC1);

  sendCommand(0x30);  // frame go with waveform
  sendData(0x08);

  sendCommand(0x50);  // CDI, border white
  sendData(0x77);

  sendCommand(0x61);  // TRES
  sendData(WIDTH / 256);
  sendData(WIDTH % 256);
  sendData(HEIGHT / 256);
  sendData(HEIGHT % 256);

  sendCommand(0x62);  // HTOTAL
  static const uint8_t htotal[8] = {0x98, 0x98, 0x98, 0x75, 0xCA, 0xB2, 0x98, 0x7E};
  for (uint8_t b : htotal) sendData(b);

  sendCommand(0x65);  // GSST
  for (int i = 0; i < 4; i++) sendData(0x00);

  sendCommand(0xE9);
  sendData(0x01);
}

void EPD5in76::powerOn() {
  sendCommand(0x04);
  readBusy();
}

void EPD5in76::refresh() {
  sendCommand(0x12);
  sendData(0x00);
  readBusy();
}

// --- public API ---------------------------------------------------------

void EPD5in76::init() {
  commonInit();

  if (useTempSensor) {
    setTemperature(readTemperature());
    if (timing) Serial.printf("epd temperature %d C -> code %u\n", lastTemp_, temperatureCode_);
  }

  // Forced temperature + load waveform from OTP
  sendCommand(0xE0);
  sendData(0x02);
  sendCommand(0xE6);
  sendData(temperatureCode_);
  sendCommand(0xA5);
  readBusy();
  delay(10);

  powerOn();
  partial_ = false;
}

void EPD5in76::initPartial() {
  commonInit();

  // Partial waveform from OTP
  sendCommand(0xE0);
  sendData(0x00);
  sendCommand(0xA5);
  readBusy();

  powerOn();
  partial_ = true;
  partialCount_ = 0;
}

// Copy one row of `buf` (logical order) into `out` in the order it is sent,
// applying flipX. flipY is handled by the caller through logicalRow().
void EPD5in76::loadSendRow(const uint8_t *buf, uint16_t sendRow, uint8_t *out) const {
  const uint8_t *src = buf + (size_t)logicalRow(sendRow) * ROW_BYTES;
  if (!flipX) {
    memcpy(out, src, ROW_BYTES);
  } else {
    for (size_t i = 0; i < ROW_BYTES; i++) out[i] = rev8(src[ROW_BYTES - 1 - i]);
  }
}

// Send `rows` send-order rows starting at sendRow0 from newBuf, converted to
// the panel's 2 bpp format. Must be called between beginBlock()/endBlock().
void EPD5in76::sendRows(const uint8_t *newBuf, uint16_t sendRow0, uint16_t rows) {
  uint8_t newRow[ROW_BYTES], oldRow[ROW_BYTES], out[ROW_BYTES * 2];
  for (uint16_t r = sendRow0; r < sendRow0 + rows; r++) {
    loadSendRow(newBuf, r, newRow);
    if (partial_) {
      if (prevValid_) loadSendRow(prev_, r, oldRow);
      else memset(oldRow, 0xFF, ROW_BYTES);
      for (size_t i = 0; i < ROW_BYTES; i++) {
        uint16_t v = (spread_[oldRow[i]] << 1) | spread_[newRow[i]];
        out[2 * i] = v >> 8;
        out[2 * i + 1] = v & 0xFF;
      }
    } else {
      for (size_t i = 0; i < ROW_BYTES; i++) {
        uint16_t s = spread_[newRow[i]];
        uint16_t v = (s << 1) | s;  // each set bit becomes 11
        out[2 * i] = v >> 8;
        out[2 * i + 1] = v & 0xFF;
      }
    }
    spi_.writeBytes(out, sizeof(out));
  }
}

void EPD5in76::display() {
  if (!frame_) return;
  // Nothing changed since the last partial update: skip the transfer.
  if (partial_ && prevValid_ && memcmp(frame_, prev_, FRAME_BYTES) == 0) return;
  uint32_t t0 = millis();
  sendCommand(0x10);
  readBusy();
  uint32_t t1 = millis();
  beginBlock();
  sendRows(frame_, 0, HEIGHT);
  endBlock();
  uint32_t t2 = millis();
  if (partial_) partialCount_++;
  refresh();
  memcpy(prev_, frame_, FRAME_BYTES);
  prevValid_ = true;
  if (timing) {
    uint32_t t3 = millis();
    Serial.printf("epd %s: wait %lums  spi %lums  refresh %lums  total %lums\n",
                  partial_ ? "partial" : "full", (unsigned long)(t1 - t0),
                  (unsigned long)(t2 - t1), (unsigned long)(t3 - t2),
                  (unsigned long)(t3 - t0));
  }
}

void EPD5in76::sendTres(uint16_t rows) {
  sendCommand(0x61);
  sendData(WIDTH / 256);
  sendData(WIDTH % 256);
  sendData(rows / 256);
  sendData(rows % 256);
}

void EPD5in76::displayWindow(int y0, int rows, int repeat) {
  if (!frame_ || !partial_) return;  // needs initPartial()
  rows = max(120, min(rows, (int)HEIGHT));  // datasheet: VRES >= 120
  y0 = max(0, min(y0, (int)HEIGHT - rows));

  // Nothing changed in this window? (send-order rows y0..y0+rows)
  bool changed = !prevValid_;
  for (int r = y0; r < y0 + rows && !changed; r++) {
    size_t off = (size_t)logicalRow(r) * ROW_BYTES;
    if (memcmp(frame_ + off, prev_ + off, ROW_BYTES) != 0) changed = true;
  }
  if (!changed) return;

  int gateStart = HEIGHT - (y0 + rows);
  uint32_t t0 = millis();

  sendTres(rows);  // window size
  if (gateStart) {
    sendCommand(0x65);  // GSST: gate start line (layout guessed)
    sendData(0x00);
    sendData(0x00);
    sendData((gateStart >> 8) & 0x03);
    sendData(gateStart & 0xFF);
  }

  for (int n = 0; n < max(1, repeat); n++) {
    sendCommand(0x10);
    readBusy();
    beginBlock();
    sendRows(frame_, y0, rows);
    endBlock();
    refresh();
  }

  // restore the full-frame window for normal updates
  sendTres(HEIGHT);
  if (gateStart) {
    sendCommand(0x65);
    for (int i = 0; i < 4; i++) sendData(0x00);
  }

  if (!prevValid_) memset(prev_, 0xFF, FRAME_BYTES);
  for (int r = y0; r < y0 + rows; r++) {
    size_t off = (size_t)logicalRow(r) * ROW_BYTES;
    memcpy(prev_ + off, frame_ + off, ROW_BYTES);
  }
  prevValid_ = true;
  partialCount_++;
  if (timing)
    Serial.printf("epd window gate %d+%d x%d: total %lums\n", gateStart, rows, repeat,
                  (unsigned long)(millis() - t0));
}

void EPD5in76::displayBottomStrip(int rows, int repeat) {
  if (!(partial_ && flipY) || rows >= HEIGHT) {
    display();
    return;
  }
  // with flipY the image's bottom rows are the first rows sent
  displayWindow(0, rows, repeat);
}

void EPD5in76::clear() {
  if (!frame_) return;
  memset(frame_, 0xFF, FRAME_BYTES);
  display();
}

void EPD5in76::sleep() {
  sendCommand(0x02);  // power off
  sendData(0x00);
  readBusy();
  sendCommand(0x07);  // deep sleep
  sendData(0xA5);
  delay(2000);
  prevValid_ = false;
}
