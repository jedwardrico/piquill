#include "gfx.h"

#include <math.h>
#include <string.h>

namespace gfx {

void clear(uint8_t *frame) { memset(frame, 0xFF, (size_t)ROW_BYTES * SCREEN_H); }

void setPixel(uint8_t *frame, int x, int y, bool black) {
  if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return;
  uint8_t mask = 0x80 >> (x & 7);
  uint8_t &b = frame[y * ROW_BYTES + (x >> 3)];
  if (black) b &= ~mask; else b |= mask;
}

void fillRect(uint8_t *frame, int x0, int y0, int x1, int y1, bool black) {
  for (int y = y0; y <= y1; y++)
    for (int x = x0; x <= x1; x++) setPixel(frame, x, y, black);
}

void hline(uint8_t *frame, int x0, int x1, int y, int thickness) {
  fillRect(frame, x0, y, x1, y + thickness - 1, true);
}

// Ink of one glyph, optionally sheared (italic) and/or doubled one pixel to the
// right (bold). The shear leans the top of each glyph right, about the baseline.
static void drawGlyph(uint8_t *frame, const Font &f, int x, int y, char c, bool bold, bool italic) {
  const uint8_t *g = f.glyph(c);
  for (int row = 0; row < f.h; row++) {
    int dx = italic ? (int)lroundf(ITALIC_SLANT * (f.ascent - row)) : 0;
    for (int col = 0; col < f.w; col++) {
      if (!(g[row * f.rowBytes + (col >> 3)] & (0x80 >> (col & 7)))) continue;
      setPixel(frame, x + col + dx, y + row, true);
      if (bold) setPixel(frame, x + col + dx + 1, y + row, true);
    }
  }
}

int drawRun(uint8_t *frame, const Font &f, int x, int y, const char *s, size_t n, bool bold, bool italic,
            bool underline) {
  int width = textWidth(f, n);
  if (n == 0) return 0;
  for (size_t i = 0; i < n && x + (int)(i * f.w) < SCREEN_W; i++)
    drawGlyph(frame, f, x + (int)(i * f.w), y, s[i], bold, italic);
  if (underline) hline(frame, x, x + width, y + f.ascent + 3);
  return width;
}

int drawFormatted(uint8_t *frame, const Font &f, int x, int y, const std::string &s, textfmt::State &st) {
  size_t runStart = 0;
  auto flush = [&](size_t end) {
    if (end > runStart) x += drawRun(frame, f, x, y, s.data() + runStart, end - runStart, st.bold, st.italic, st.underline);
  };
  for (size_t i = 0; i < s.size(); i++) {
    if (!textfmt::isMarker(s[i])) continue;
    flush(i);
    runStart = i + 1;
    if (s[i] == textfmt::BOLD) st.bold = !st.bold;
    else if (s[i] == textfmt::ITALIC) st.italic = !st.italic;
    else st.underline = !st.underline;
  }
  flush(s.size());
  return x;
}

void drawCentered(uint8_t *frame, const Font &f, float cx, int y, const std::string &s) {
  drawText(frame, f, (int)lroundf(cx - textWidth(f, s.size()) / 2.0f), y, s);
}

void drawRightAligned(uint8_t *frame, const Font &f, int xRight, int y, const std::string &s) {
  drawText(frame, f, xRight - textWidth(f, s.size()), y, s);
}

}  // namespace gfx
