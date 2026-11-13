// Saving and loading documents, port of the file handling in src/main.py.
//
//   /cache.txt          working copy of the current text (restored at start-up)
//   /current_doc.txt    name of the current document ("" = not saved yet)
//   /recent.txt         document names, most recently saved first
//   /documents/*.md     saved documents (Markdown: **bold**, *italic*, <u>underline</u>)
//
// The ESP32 has no real-time clock, so "most recently modified" is tracked in
// recent.txt instead of using file times, and a file's date is only shown once
// the clock has been set.
#pragma once

#include <functional>
#include <string>
#include <time.h>
#include <vector>

#include "storage.h"

class DocStore {
 public:
  static constexpr size_t MAX_NAME_LEN = 32;

  explicit DocStore(Storage &fs) : fs_(fs) {}
  void begin();  // creates /documents

  static std::string sanitizeName(const std::string &name);
  static std::string docPath(const std::string &name) { return "/documents/" + name + ".md"; }

  std::vector<std::string> loadLines(const std::string &path);
  bool saveLines(const std::string &path, const std::vector<std::string> &lines);

  std::vector<std::string> loadWorking() { return loadLines("/cache.txt"); }
  bool saveWorking(const std::vector<std::string> &lines) { return saveLines("/cache.txt", lines); }

  std::vector<std::string> loadDoc(const std::string &name) { return loadLines(docPath(name)); }
  bool saveDoc(const std::string &name, const std::vector<std::string> &lines);
  bool docExists(const std::string &name) { return fs_.exists(docPath(name)); }
  bool removeDoc(const std::string &name);

  std::vector<std::string> listDocuments();  // most recently saved first

  struct Info { std::string date; size_t words; };
  Info docInfo(const std::string &name);

  void rememberCurrent(const std::string &name);  // "" = none
  std::string loadCurrentName();                  // "" if none / file missing

  // The clock, used to name archived text by date. Without one (or while it is
  // unset) names are untitled_1, untitled_2, ...
  void setClock(std::function<time_t()> clock) { clock_ = clock; }
  // A free name for text that was never saved: untitled_20251112_134501 when the
  // clock is set, otherwise untitled_1, untitled_2, ...
  std::string newArchiveName();

 private:
  void touchRecent(const std::string &name);
  void forgetRecent(const std::string &name);
  Storage &fs_;
  std::function<time_t()> clock_;
};
