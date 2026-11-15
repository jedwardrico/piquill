// Draws the Editor's state into a frame: 14 text lines, the input line, then the status panel (page bar, session time, word and
// character counts, file name, battery). Pure drawing: no Arduino calls.
#pragma once

#include <stdint.h>

#include "editor.h"

struct BatteryInfo {
  bool present = false;
  int percent = 0;
  bool charging = false;
};

namespace ui {
constexpr int LOW_BATTERY_WARN = 15;  // percent; below this the panel shows "LOW"
constexpr int MARGIN_X = 10;
constexpr int LINE_H = 36;
constexpr int TEXT_TOP = 4;
constexpr int INPUT_Y = TEXT_TOP + TextBuffer::LINES_ON_SCREEN * LINE_H;  // top of the input line
constexpr int CURSOR_W = 3;                        // the typing cursor's advance, in pixels
constexpr int TEXT_MAX_W = 920 - 2 * MARGIN_X - CURSOR_W;  // widest finished line, in pixels
constexpr int PANEL_Y = INPUT_Y + 40;                                      // top of the status panel

// Width in pixels of a text line (style markers excluded), in the text font.
int lineWidth(const std::string &s);
// Make the editor wrap lines to fit the screen in the text font. Call before begin().
void configure(Editor &ed);

// Redraw the whole screen (clears the frame first). Also draws and clears the
// editor's one-shot console message.
void renderDisplay(uint8_t *frame, Editor &ed, const BatteryInfo &bat);
// Redraw only the bottom part: the input line and the status panel.
void renderInput(uint8_t *frame, Editor &ed, const BatteryInfo &bat);
// A centred one-line message on an otherwise white screen (shutdown notice).
void renderMessage(uint8_t *frame, const char *msg);
std::string batteryLabel(const BatteryInfo &bat);  // "Bat 87%", "Bat 87%+", "LOW 12%" or ""
}  // namespace ui
