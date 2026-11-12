// Draws text into a 1 bpp frame (row 0 at the top, MSB first, 1 = white), the
// same layout EPD5in76::frame() uses. Layout constants match the Pi version.
#pragma once

#include <stdint.h>
#include <string>

#include "text_buffer.h"

namespace textrender {
constexpr int SCREEN_W = 920;
constexpr int SCREEN_H = 680;
constexpr int ROW_BYTES = SCREEN_W / 8;
constexpr int MARGIN_X = 10;
constexpr int LINE_H = 36;
constexpr int TEXT_TOP = 4;
// top of the input line, below the 14 finished lines
constexpr int INPUT_Y = TEXT_TOP + TextBuffer::LINES_ON_SCREEN * LINE_H;

// Draw one character cell (CELL_W x CELL_H) with its top-left at (x, y).
void drawChar(uint8_t *frame, int x, int y, char c);
// Draw a string; returns the x after it. Clips at the screen edge.
int drawString(uint8_t *frame, int x, int y, const char *s, size_t len);
// Clear the frame to white, then draw the finished lines (newest just above the
// input line) and the input line with a "|" cursor. `scroll` as in visibleLines().
void drawEditor(uint8_t *frame, const TextBuffer &buf, int scroll = 1);
// Redraw only the input line (clears its row band first).
void drawInputLine(uint8_t *frame, const TextBuffer &buf);
}  // namespace textrender
