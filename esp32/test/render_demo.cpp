// Renders sample screens to PBM files (argv[1] = output prefix) for a visual check.
#include "editor.h"
#include "gfx.h"
#include "host_storage.h"
#include "ui.h"
#include <stdio.h>

static uint32_t nowMs = 0;
static void typeStr(Editor &ed, const char *s) { for (; *s; s++) ed.onKey(KeyEvent{KeyType::Char, *s, 0}); }
static void ctrl(Editor &ed, char c) { ed.onKey(KeyEvent{KeyType::Char, c, MOD_CTRL}); }
static void key(Editor &ed, KeyType t, uint8_t m = 0) { ed.onKey(KeyEvent{t, 0, m}); }
static void dump(const uint8_t *frame, const std::string &path) {
  FILE *f = fopen(path.c_str(), "wb");
  fprintf(f, "P4\n920 680\n");
  for (int i = 0; i < gfx::ROW_BYTES * gfx::SCREEN_H; i++) fputc((uint8_t)~frame[i], f);  // PBM: 1 = black
  fclose(f);
}

int main(int, char **argv) {
  std::string prefix = argv[1];
  std::filesystem::remove_all(prefix + "_fs");
  HostStorage fs(prefix + "_fs");
  DocStore docs(fs);
  Editor ed(docs, [] { return nowMs; });
  ed.begin();
  static uint8_t frame[gfx::ROW_BYTES * gfx::SCREEN_H];
  BatteryInfo bat{true, 87, false};

  const char *paras[] = {
      "The quick brown fox jumps over the lazy dog. ",
      "Pack my box with five dozen liquor jugs, then write about it all day long. ",
  };
  for (int p = 0; p < 6; p++) {
    typeStr(ed, paras[p % 2]);
    ctrl(ed, 'b'); typeStr(ed, "bold words "); ctrl(ed, 'b');
    ctrl(ed, 'i'); typeStr(ed, "italic words "); ctrl(ed, 'i');
    ctrl(ed, 'u'); typeStr(ed, "underlined words "); ctrl(ed, 'u');
    typeStr(ed, "and then plain text again. ");
    ctrl(ed, 'b'); ctrl(ed, 'i'); typeStr(ed, "both bold and italic"); ctrl(ed, 'b'); ctrl(ed, 'i');
    key(ed, KeyType::Enter);
  }
  typeStr(ed, "typing now ");
  ctrl(ed, 'b'); typeStr(ed, "in bold");
  nowMs = 3723000;
  ctrl(ed, 's'); typeStr(ed, "Draft one"); key(ed, KeyType::Enter);   // saves; shows [Saved]
  ui::renderDisplay(frame, ed, bat);
  dump(frame, prefix + "_edit.pbm");

  // typing redraws only the bottom strip: must equal a full redraw
  typeStr(ed, " more");
  static uint8_t full[gfx::ROW_BYTES * gfx::SCREEN_H];
  ui::renderInput(frame, ed, bat);
  ui::renderDisplay(full, ed, bat);
  int diff = 0;
  for (int i = 0; i < gfx::ROW_BYTES * gfx::SCREEN_H; i++) diff += frame[i] != full[i];
  printf("strip-vs-full differing bytes: %d\n", diff);

  // a second doc and the browser
  ctrl(ed, 'n'); typeStr(ed, "short"); ctrl(ed, 's'); typeStr(ed, "Notes"); key(ed, KeyType::Enter);
  ctrl(ed, 'o'); key(ed, KeyType::Down);
  ui::renderDisplay(frame, ed, bat);
  dump(frame, prefix + "_browse.pbm");

  // prompt, scrolled page, low battery
  key(ed, KeyType::Escape); ctrl(ed, 'r');
  typeStr(ed, "New name");
  bat = {true, 9, false};
  ui::renderDisplay(frame, ed, bat);
  dump(frame, prefix + "_prompt.pbm");
}
