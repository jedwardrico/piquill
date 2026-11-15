#include "ui.h"

#include <math.h>
#include <string.h>

#include "gfx.h"

namespace ui {
using namespace gfx;

static const Font &F_TEXT = FONT_28;    // text, input line, browser names
static const Font &F_STATUS = FONT_20;
static const Font &F_SMALL = FONT_18;
static const Font &F_BIG = FONT_32;
static const Font &F_NAME = FONT_26;

static std::string thousands(size_t n) {
  std::string s = std::to_string(n), out;
  for (size_t i = 0; i < s.size(); i++) {
    if (i && (s.size() - i) % 3 == 0) out += ',';
    out += s[i];
  }
  return out;
}

std::string batteryLabel(const BatteryInfo &bat) {
  if (!bat.present) return "";
  if (bat.charging) return "Bat " + std::to_string(bat.percent) + "%+";
  return (bat.percent <= LOW_BATTERY_WARN ? "LOW " : "Bat ") + std::to_string(bat.percent) + "%";
}

static void drawStatus(uint8_t *f, const Editor &ed, const BatteryInfo &bat) {
  fillRect(f, 0, PANEL_Y, SCREEN_W - 1, SCREEN_H - 1, false);
  int page = ed.currentPage(), pages = ed.totalPages();

  // page progress bar: "Pg 2 ------[=]------ of 5"
  int rowY = PANEL_Y + 2;
  drawText(f, F_SMALL, 10, rowY, "Pg " + std::to_string(page));
  drawRightAligned(f, F_SMALL, SCREEN_W - 10, rowY, "of " + std::to_string(pages));
  int x0 = 90, x1 = SCREEN_W - 90, mid = rowY + 10, markerW = 40;
  hline(f, x0, x1, mid);
  double frac = pages > 1 ? (page - 1.0) / (pages - 1) : 0;
  int mx = (int)(x0 + (x1 - x0 - markerW) * frac);
  fillRect(f, mx, mid - 2, mx + markerW, mid + 2, true);

  // the divider between the text and the stats
  int divY = PANEL_Y + 28;
  hline(f, 10, SCREEN_W - 10, divY, 2);

  // stats row
  int valY = divY + 8, labelY = divY + 44;
  float leftX = SCREEN_W * 0.17f, rightX = SCREEN_W * 0.83f;
  drawCentered(f, F_STATUS, leftX, valY + 4, ed.sessionTime());
  drawCentered(f, F_SMALL, leftX, labelY, "Session Time");
  drawCentered(f, F_BIG, SCREEN_W / 2.0f, valY, "Words: " + thousands(ed.text().wordCount()));
  drawCentered(f, F_STATUS, rightX, valY + 4, thousands(ed.text().charCount()));
  drawCentered(f, F_SMALL, rightX, labelY, "Characters");

  // file name
  std::string name = ed.docName().empty() ? "untitled" : ed.docName();
  if (name.size() > DocStore::MAX_NAME_LEN) name = name.substr(0, DocStore::MAX_NAME_LEN - 1) + "~";
  drawCentered(f, F_NAME, SCREEN_W / 2.0f, labelY + 20, name);

  // battery, bottom right
  std::string label = batteryLabel(bat);
  if (!label.empty()) drawRightAligned(f, F_SMALL, SCREEN_W - 10, labelY + 28, label);
}

static void drawBrowser(uint8_t *f, const Editor &ed) {
  const int visible = Editor::BROWSE_ROWS;
  drawCentered(f, F_SMALL, SCREEN_W / 2.0f, TEXT_TOP,
               "Up/Down: choose  Shift: page  Enter: open  Del: delete  Esc: cancel");
  const auto &items = ed.browseItems();
  if (items.empty()) {
    drawText(f, F_TEXT, 10, LINE_H * 2, "(no saved files yet - Ctrl+S to save)");
    return;
  }
  int idx = ed.browseIndex();
  int top = (idx / visible) * visible;  // the window moves a page at a time
  int y = TEXT_TOP + LINE_H;
  for (int i = top; i < (int)items.size() && i < top + visible; i++, y += LINE_H) {
    const auto &it = items[i];
    std::string marker = i == idx ? ">" : " ";
    if (i == idx && ed.browseConfirmDelete()) {
      drawText(f, F_TEXT, 10, y, marker + " " + it.name + "   delete? (y/n)");
    } else {
      drawText(f, F_TEXT, 10, y, marker + " " + it.name);
      std::string w = thousands(it.words) + (it.words == 1 ? " word" : " words");
      drawText(f, F_STATUS, 580, y + 5, it.date);
      drawRightAligned(f, F_STATUS, SCREEN_W - 10, y + 5, w);
    }
  }
  if ((int)items.size() > visible) {
    int pages = ((int)items.size() + visible - 1) / visible;
    drawRightAligned(f, F_SMALL, SCREEN_W - 10, INPUT_Y + 8, std::to_string(idx / visible + 1) + "/" + std::to_string(pages));
  }
}

// The input line with styles; the cursor itself is always drawn plain.
static void drawInputLine(uint8_t *f, const Editor &ed) {
  std::string text;
  size_t cur;
  ed.currentInput(text, cur);
  if (cur > text.size()) cur = text.size();
  textfmt::State st;
  int x = drawFormatted(f, F_TEXT, MARGIN_X, INPUT_Y, text.substr(0, cur), st);
  drawRun(f, F_TEXT, x, INPUT_Y, "|", 1);
  x += F_TEXT.w;
  drawFormatted(f, F_TEXT, x, INPUT_Y, text.substr(cur), st);
}

static bool showsInputLine(const Editor &ed) {
  return ed.mode() == Mode::Prompt || (ed.mode() == Mode::Edit && ed.scroll() == 1);
}

void renderDisplay(uint8_t *f, Editor &ed, const BatteryInfo &bat) {
  clear(f);
  if (ed.mode() == Mode::Browse) {
    drawBrowser(f, ed);
  } else {
    // newest visible line sits just above the input line
    auto vis = ed.text().visibleLines(ed.scroll());
    int y = INPUT_Y - LINE_H;
    for (auto it = vis.rbegin(); it != vis.rend(); ++it, y -= LINE_H) {
      textfmt::State st;
      drawFormatted(f, F_TEXT, MARGIN_X, y, it->substr(0, textfmt::rawIndex(*it, TextBuffer::COLS)), st);
    }
  }
  if (showsInputLine(ed)) drawInputLine(f, ed);
  drawStatus(f, ed, bat);

  std::string msg = ed.takeConsole();
  if (!msg.empty()) {
    // right-aligned on the input line's row; clear only its own box so the
    // status panel below is left alone
    int w = textWidth(F_TEXT, msg.size());
    fillRect(f, SCREEN_W - 10 - w, INPUT_Y, SCREEN_W - 1, INPUT_Y + F_TEXT.h - 1, false);
    drawText(f, F_TEXT, SCREEN_W - 10 - w, INPUT_Y, msg);
  }
}

void renderInput(uint8_t *f, Editor &ed, const BatteryInfo &bat) {
  fillRect(f, 0, INPUT_Y, SCREEN_W - 1, SCREEN_H - 1, false);
  drawInputLine(f, ed);
  drawStatus(f, ed, bat);  // the word count changes as you type
}

void renderMessage(uint8_t *f, const char *msg) {
  clear(f);
  std::string m = msg;
  drawText(f, F_TEXT, (SCREEN_W - textWidth(F_TEXT, m.size())) / 2, SCREEN_H / 2, m);
}

}  // namespace ui
