// The typewriter's behaviour: editing, the file-name prompt, the file browser,
// saving and loading. Port of the key handling in src/main.py. Pure C++ (no
// Arduino calls): feed it key events and it tells you what to redraw.
//
// Keys, as on the Pi:
//   typing        text; Enter ends a line (and saves); Backspace deletes
//   Up/Left       scroll back a page      Down/Right  scroll forward a page
//   Ctrl+B/I/U    bold / italic / underline
//   Ctrl+S        save (asks for a name the first time)
//   Ctrl+R        rename / save as        Ctrl+O  open a saved file
//   Ctrl+N        new file                Ctrl+Esc  power down
// In the file browser: Up/Down choose (Shift = a page), Enter open, Delete
// delete (then y/n), Esc cancel.
#pragma once

#include <stdint.h>

#include <functional>
#include <string>
#include <vector>

#include "doc_store.h"
#include "hid_keys.h"
#include "text_buffer.h"

enum class Redraw : uint8_t {
  None,         // nothing changed on screen
  Input,        // only the input line / status strip at the bottom
  Display,      // the whole screen
  DisplayFull,  // the whole screen with a full (flashing) refresh to clear ghosting
};

enum class Mode : uint8_t { Edit, Prompt, Browse };

class Editor {
 public:
  static constexpr int BROWSE_ROWS = TextBuffer::LINES_ON_SCREEN - 2;

  struct BrowseItem {
    std::string name, date;
    size_t words;
  };

  Editor(DocStore &docs, std::function<uint32_t()> nowMs) : docs_(docs), nowMs_(nowMs) {}

  void begin();                      // restore the working copy and current document
  Redraw onKey(const KeyEvent &e);
  Redraw tick();                     // call regularly: clears the "[Saved]" message after a second
  void saveNow() { saveCurrent(); }  // before power-off

  // --- state for drawing ---
  Mode mode() const { return mode_; }
  const TextBuffer &text() const { return text_; }
  int scroll() const { return scroll_; }
  int totalPages() const { return text_.totalPages(); }
  int currentPage() const;
  const std::string &docName() const { return doc_; }   // "" = untitled
  std::string sessionTime() const;                      // HH:MM:SS
  // text and cursor for the input line in the current mode
  void currentInput(std::string &text, size_t &cursor) const;
  const std::vector<BrowseItem> &browseItems() const { return browse_; }
  int browseIndex() const { return browseIndex_; }
  bool browseConfirmDelete() const { return confirmDelete_; }
  // The one-shot message ("[Saved]") to draw once; cleared by this call.
  std::string takeConsole() { std::string m = console_; console_.clear(); return m; }
  bool powerDownRequested() const { return powerDown_; }

 private:
  Redraw editKey(const KeyEvent &e);
  Redraw promptKey(const KeyEvent &e);
  Redraw browseKey(const KeyEvent &e);
  Redraw ctrlKey(const KeyEvent &e);
  Redraw endModal(const char *msg);
  Redraw finishPrompt();
  Redraw openDocument(const std::string &name);
  void startPrompt(bool rename, const std::string &label, const std::string &initial);
  void startBrowser();
  void saveCurrent();
  void archiveCurrent();
  void setCurrent(const std::string &name);
  void armMessageClear() { clearAtMs_ = nowMs_() + 1000; clearArmed_ = true; }

  DocStore &docs_;
  std::function<uint32_t()> nowMs_;
  TextBuffer text_;
  std::string doc_;
  Mode mode_ = Mode::Edit;
  int scroll_ = 1;
  std::string console_;
  uint32_t sessionStartMs_ = 0;
  uint32_t clearAtMs_ = 0;
  bool clearArmed_ = false;
  bool powerDown_ = false;
  // prompt
  bool promptRename_ = false;
  std::string promptLabel_, promptText_;
  // browser
  std::vector<BrowseItem> browse_;
  int browseIndex_ = 0;
  bool confirmDelete_ = false;
};
