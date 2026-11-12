#include "text_render.h"

#include <string.h>

#include "font_courier28.h"

namespace textrender {

void drawChar(uint8_t *frame, int x, int y, char c) {
  if (c < font::FIRST || c > font::LAST) c = '?';
  const uint8_t *g = font::GLYPHS[c - font::FIRST];
  for (int row = 0; row < font::CELL_H; row++) {
    int py = y + row;
    if (py < 0 || py >= SCREEN_H) continue;
    for (int col = 0; col < font::CELL_W; col++) {
      if (!(g[row * font::ROW_BYTES + (col >> 3)] & (0x80 >> (col & 7)))) continue;
      int px = x + col;
      if (px < 0 || px >= SCREEN_W) continue;
      frame[py * ROW_BYTES + (px >> 3)] &= ~(0x80 >> (px & 7));  // ink = black = 0
    }
  }
}

int drawString(uint8_t *frame, int x, int y, const char *s, size_t len) {
  for (size_t i = 0; i < len && x < SCREEN_W; i++) {
    drawChar(frame, x, y, s[i]);
    x += font::CELL_W;
  }
  return x;
}

static void clearBand(uint8_t *frame, int y, int h) {
  if (y < 0) y = 0;
  if (y + h > SCREEN_H) h = SCREEN_H - y;
  if (h > 0) memset(frame + (size_t)y * ROW_BYTES, 0xFF, (size_t)h * ROW_BYTES);
}

void drawInputLine(uint8_t *frame, const TextBuffer &buf) {
  clearBand(frame, INPUT_Y, LINE_H);
  const std::string &in = buf.input();
  size_t cur = buf.cursor() > in.size() ? in.size() : buf.cursor();
  int x = drawString(frame, MARGIN_X, INPUT_Y, in.data(), cur);
  drawChar(frame, x, INPUT_Y, '|');
  x += font::CELL_W;
  drawString(frame, x, INPUT_Y, in.data() + cur, in.size() - cur);
}

void drawEditor(uint8_t *frame, const TextBuffer &buf, int scroll) {
  memset(frame, 0xFF, (size_t)ROW_BYTES * SCREEN_H);
  auto vis = buf.visibleLines(scroll);
  // newest visible line sits just above the input line
  int y = INPUT_Y - LINE_H;
  for (auto it = vis.rbegin(); it != vis.rend(); ++it) {
    size_t n = it->size() > TextBuffer::COLS ? TextBuffer::COLS : it->size();
    drawString(frame, MARGIN_X, y, it->data(), n);
    y -= LINE_H;
  }
  drawInputLine(frame, buf);
}

}  // namespace textrender
