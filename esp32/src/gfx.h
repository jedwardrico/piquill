// Drawing into a 1 bpp frame (row 0 at the top, MSB first, 1 = white) -- the
// same layout EPD5in76::frame() uses. Bitmap-font text with the Pi's bold
// (double-strike), italic (shear) and underline styles.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "fonts.h"
#include "textfmt.h"

namespace gfx {
constexpr int SCREEN_W = 920;
constexpr int SCREEN_H = 680;
constexpr int ROW_BYTES = SCREEN_W / 8;
constexpr float ITALIC_SLANT = 0.22f;  // same as the Pi

void clear(uint8_t *frame);  // all white
void setPixel(uint8_t *frame, int x, int y, bool black);
void fillRect(uint8_t *frame, int x0, int y0, int x1, int y1, bool black);  // x1,y1 inclusive
void hline(uint8_t *frame, int x0, int x1, int y, int thickness = 1);

inline int textWidth(const Font &f, size_t nchars) { return (int)(nchars * f.w); }

// One run of text in one style, top-left at (x, y). Returns its width.
int drawRun(uint8_t *frame, const Font &f, int x, int y, const char *s, size_t n, bool bold = false,
            bool italic = false, bool underline = false);
// Plain text.
inline int drawText(uint8_t *frame, const Font &f, int x, int y, const std::string &s) {
  return drawRun(frame, f, x, y, s.data(), s.size());
}
// Text containing style markers, starting in `st`. Returns the x after it; `st`
// is updated to the style at the end.
int drawFormatted(uint8_t *frame, const Font &f, int x, int y, const std::string &s, textfmt::State &st);
void drawCentered(uint8_t *frame, const Font &f, float cx, int y, const std::string &s);
void drawRightAligned(uint8_t *frame, const Font &f, int xRight, int y, const std::string &s);
}  // namespace gfx
