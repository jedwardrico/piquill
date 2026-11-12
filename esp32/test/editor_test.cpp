// Scenario tests for Editor + DocStore on a real folder.
#include "doc_store.h"
#include "editor.h"
#include "host_storage.h"
#include <stdio.h>
#include <string>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
static std::string hx(const std::string &s) { std::string r; char b[4]; for (unsigned char c : s) { snprintf(b, 4, c < 32 ? "<%d>" : "%c", c); r += b; } return r; }
#define CHECK_EQ(a, b) do { std::string _a = (a), _b = (b); if (_a != _b) { printf("FAIL line %d: %s\n  got      [%s]\n  expected [%s]\n", __LINE__, #a, hx(_a).c_str(), hx(_b).c_str()); fails++; } } while (0)

static uint32_t nowMs = 0;
static KeyEvent K(KeyType t, char c = 0, uint8_t m = 0) { return KeyEvent{t, c, m}; }
static Redraw typeStr(Editor &ed, const char *s) {
  Redraw r = Redraw::None;
  for (; *s; s++) r = ed.onKey(K(KeyType::Char, *s));
  return r;
}
static Redraw ctrl(Editor &ed, char c) { return ed.onKey(K(KeyType::Char, c, MOD_CTRL)); }
static Redraw press(Editor &ed, KeyType t, uint8_t m = 0) { return ed.onKey(K(t, 0, m)); }
static std::string rd(Storage &fs, const char *path) { std::string s; return fs.readFile(path, s) ? s : "<missing>"; }

int main() {
  std::string root = std::string(getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp") + "/piquill_editor_test";
  std::filesystem::remove_all(root);
  HostStorage fs(root);
  DocStore docs(fs);
  Editor ed(docs, [] { return nowMs; });
  ed.begin();

  // typing: Input redraw, Enter saves the working copy
  CHECK(typeStr(ed, "hello world") == Redraw::Input);
  CHECK(press(ed, KeyType::Enter) == Redraw::Display);
  CHECK_EQ(rd(fs, "/cache.txt"), "hello world\n");
  typeStr(ed, "second");
  CHECK(ed.text().lines().size() == 1 && ed.text().input() == "second");

  // first Ctrl+S asks for a name; the name is cleaned up; the open line is included
  CHECK(ctrl(ed, 's') == Redraw::Display && ed.mode() == Mode::Prompt);
  std::string t; size_t c;
  ed.currentInput(t, c);
  CHECK_EQ(t, "Save as: ");
  typeStr(ed, "My Doc!");
  CHECK(press(ed, KeyType::Backspace) == Redraw::Input);  // delete the '!'
  typeStr(ed, "?");
  CHECK(press(ed, KeyType::Enter) == Redraw::Display);
  CHECK(ed.mode() == Mode::Edit);
  CHECK_EQ(ed.takeConsole(), "[Saved]");
  CHECK_EQ(ed.docName(), "My Doc_");
  CHECK_EQ(rd(fs, "/documents/My Doc_.md"), "hello world\nsecond\n");
  CHECK_EQ(rd(fs, "/current_doc.txt"), "My Doc_");

  // formatting is stored as Markdown
  ctrl(ed, 'b'); typeStr(ed, " bold"); ctrl(ed, 'b'); typeStr(ed, " x"); ctrl(ed, 'i'); typeStr(ed, "it");
  press(ed, KeyType::Enter);
  CHECK_EQ(rd(fs, "/documents/My Doc_.md"), "hello world\nsecond **bold** x*it*\n");

  // Ctrl+S on a saved document: message, then a redraw a second later removes it
  nowMs = 5000;
  CHECK(ctrl(ed, 's') == Redraw::Display);
  CHECK_EQ(ed.takeConsole(), "[Saved]");
  CHECK(ed.tick() == Redraw::None);
  nowMs = 6001;
  CHECK(ed.tick() == Redraw::Display);
  CHECK(ed.tick() == Redraw::None);

  // rename removes the old file
  ctrl(ed, 'r');
  ed.currentInput(t, c);
  CHECK_EQ(t, "Rename to: My Doc_");
  for (int i = 0; i < 6; i++) press(ed, KeyType::Backspace);  // "My Doc_" -> "M" ... then type
  typeStr(ed, "ew name");
  press(ed, KeyType::Enter);
  CHECK_EQ(ed.docName(), "Mew name");
  CHECK(!docs.docExists("My Doc_") && docs.docExists("Mew name"));
  ed.takeConsole();

  // a second document, then "name taken" and cancelling
  ctrl(ed, 'n');
  CHECK_EQ(ed.takeConsole(), "[New]");
  CHECK(ed.docName().empty() && ed.text().lines().empty());
  typeStr(ed, "other"); press(ed, KeyType::Enter);
  ctrl(ed, 's'); typeStr(ed, "Mew name");
  CHECK(press(ed, KeyType::Enter) == Redraw::Display);
  CHECK(ed.mode() == Mode::Prompt);
  CHECK_EQ(ed.takeConsole(), "[Name taken]");
  press(ed, KeyType::Escape);
  CHECK(ed.mode() == Mode::Edit);
  CHECK_EQ(ed.takeConsole(), "[Cancelled]");
  ctrl(ed, 's'); typeStr(ed, "...");  // sanitises to an empty name
  press(ed, KeyType::Enter);
  CHECK_EQ(ed.takeConsole(), "[Cancelled]");
  ctrl(ed, 's'); typeStr(ed, "Second"); press(ed, KeyType::Enter); ed.takeConsole();
  CHECK(docs.docExists("Second"));

  // browser: most recently saved first, open with Enter
  CHECK(ctrl(ed, 'o') == Redraw::Display && ed.mode() == Mode::Browse);
  CHECK(ed.browseItems().size() == 2);
  CHECK_EQ(ed.browseItems()[0].name, "Second");
  CHECK_EQ(ed.browseItems()[1].name, "Mew name");
  CHECK(ed.browseItems()[1].words == 5);  // hello world / second bold xit = 5 words
  CHECK(ed.browseIndex() == 0);  // current doc is selected
  CHECK(press(ed, KeyType::Down) == Redraw::Display && ed.browseIndex() == 1);
  CHECK(press(ed, KeyType::Down) == Redraw::Display && ed.browseIndex() == 1);  // clamped
  CHECK(press(ed, KeyType::Enter) == Redraw::DisplayFull);
  CHECK(ed.mode() == Mode::Edit && ed.docName() == "Mew name");
  CHECK_EQ(ed.takeConsole(), "[Opened]");
  CHECK(ed.text().lines().size() == 2);
  CHECK_EQ(ed.text().lines()[1], std::string("second \x01") + "bold\x01 x\x02" + "it\x02");

  // delete from the browser: confirm with y, cancel with anything else
  ctrl(ed, 'o');
  CHECK(press(ed, KeyType::Delete) == Redraw::Display && ed.browseConfirmDelete());
  ed.onKey(K(KeyType::Char, 'n'));
  CHECK(!ed.browseConfirmDelete() && ed.browseItems().size() == 2);
  press(ed, KeyType::Delete); ed.onKey(K(KeyType::Char, 'y'));
  CHECK(ed.browseItems().size() == 1 && !docs.docExists("Mew name"));
  CHECK(ed.docName().empty());  // deleting the open doc makes the text unsaved again
  press(ed, KeyType::Escape);
  CHECK(ed.mode() == Mode::Edit);

  // new on unsaved text archives it under a made-up name
  ctrl(ed, 'n');
  CHECK(docs.docExists("untitled_1"));

  // pages: 30 lines = 3 pages; arrows scroll, typing while scrolled draws nothing
  for (int i = 0; i < 30; i++) { typeStr(ed, "line"); press(ed, KeyType::Enter); }
  CHECK(ed.totalPages() == 3 && ed.currentPage() == 3 && ed.scroll() == 1);
  CHECK(press(ed, KeyType::Up) == Redraw::Display && ed.scroll() == 2 && ed.currentPage() == 2);
  press(ed, KeyType::Left); press(ed, KeyType::Left);
  CHECK(ed.scroll() == 3 && ed.currentPage() == 1);  // capped at the first page
  CHECK(typeStr(ed, "x") == Redraw::None);
  CHECK(press(ed, KeyType::Enter) == Redraw::Display && ed.scroll() == 1);  // Enter jumps back, no new line
  CHECK(ed.text().lines().size() == 30);
  press(ed, KeyType::Down);
  CHECK(ed.scroll() == 1);

  // everything comes back after a restart
  ed.saveNow();
  Editor ed2(docs, [] { return nowMs; });
  ed2.begin();
  CHECK(ed2.text().lines().size() == 30);

  // session time and power down
  nowMs = 100000; Editor ed3(docs, [] { return nowMs; }); ed3.begin();
  nowMs = 100000 + 3723000;
  CHECK_EQ(ed3.sessionTime(), "01:02:03");
  CHECK(!ed3.powerDownRequested());
  press(ed3, KeyType::Escape, MOD_CTRL);
  CHECK(ed3.powerDownRequested());

  if (fails) { printf("%d failure(s)\n", fails); return 1; }
  printf("editor: OK\n");
  return 0;
}
