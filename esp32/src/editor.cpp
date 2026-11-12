#include "editor.h"

#include <stdio.h>

#include "textfmt.h"

void Editor::begin() {
  docs_.begin();
  text_.setLines(docs_.loadWorking());
  doc_ = docs_.loadCurrentName();
  scroll_ = 1;
  mode_ = Mode::Edit;
  sessionStartMs_ = nowMs_();
}

int Editor::currentPage() const {
  int p = totalPages() - scroll_ + 1;
  return p < 1 ? 1 : p;
}

std::string Editor::sessionTime() const {
  uint32_t secs = (nowMs_() - sessionStartMs_) / 1000;
  char buf[16];
  snprintf(buf, sizeof buf, "%02u:%02u:%02u", (unsigned)(secs / 3600), (unsigned)(secs % 3600 / 60), (unsigned)(secs % 60));
  return buf;
}

void Editor::currentInput(std::string &text, size_t &cursor) const {
  if (mode_ == Mode::Prompt) {
    text = promptLabel_ + promptText_;
    cursor = text.size();
  } else {
    text = text_.input();
    cursor = text_.cursor();
  }
}

// --- documents -------------------------------------------------------------

void Editor::saveCurrent() {
  docs_.saveWorking(text_.lines());
  if (!doc_.empty()) docs_.saveDoc(doc_, text_.docLines());
}

void Editor::setCurrent(const std::string &name) {
  doc_ = name;
  docs_.rememberCurrent(name);
}

// Save the current text before replacing it. Unnamed text gets a made-up name.
void Editor::archiveCurrent() {
  if (!doc_.empty()) saveCurrent();
  else if (!text_.lines().empty() || !text_.input().empty()) docs_.saveDoc(docs_.newArchiveName(), text_.docLines());
}

Redraw Editor::endModal(const char *msg) {
  mode_ = Mode::Edit;
  console_ = msg;
  return Redraw::Display;
}

Redraw Editor::openDocument(const std::string &name) {
  archiveCurrent();
  text_.setLines(docs_.loadDoc(name));
  scroll_ = 1;
  setCurrent(name);
  docs_.saveWorking(text_.lines());
  sessionStartMs_ = nowMs_();
  mode_ = Mode::Edit;
  console_ = "[Opened]";
  return Redraw::DisplayFull;
}

void Editor::startPrompt(bool rename, const std::string &label, const std::string &initial) {
  mode_ = Mode::Prompt;
  promptRename_ = rename;
  promptLabel_ = label;
  promptText_ = initial;
}

void Editor::startBrowser() {
  browse_.clear();
  for (auto &n : docs_.listDocuments()) {
    DocStore::Info info = docs_.docInfo(n);
    browse_.push_back({n, info.date, info.words});
  }
  browseIndex_ = 0;
  confirmDelete_ = false;
  for (size_t i = 0; i < browse_.size(); i++)
    if (browse_[i].name == doc_) browseIndex_ = (int)i;
  mode_ = Mode::Browse;
}

Redraw Editor::finishPrompt() {
  std::string name = DocStore::sanitizeName(promptText_);
  if (name.empty()) return endModal("[Cancelled]");
  if (name != doc_ && docs_.docExists(name)) {
    console_ = "[Name taken]";
    return Redraw::Display;
  }
  std::string old = doc_;
  setCurrent(name);
  saveCurrent();
  if (!old.empty() && old != name && promptRename_) docs_.removeDoc(old);
  return endModal("[Saved]");
}

// --- keys ------------------------------------------------------------------

Redraw Editor::onKey(const KeyEvent &e) {
  switch (mode_) {
    case Mode::Prompt: return promptKey(e);
    case Mode::Browse: return browseKey(e);
    default: return editKey(e);
  }
}

Redraw Editor::promptKey(const KeyEvent &e) {
  switch (e.type) {
    case KeyType::Enter: return finishPrompt();
    case KeyType::Escape: return endModal("[Cancelled]");
    case KeyType::Backspace:
      if (!promptText_.empty()) promptText_.pop_back();
      return Redraw::Input;
    case KeyType::Char:
      if ((e.mods & (MOD_CTRL | MOD_ALT | MOD_GUI)) == 0 && promptText_.size() < DocStore::MAX_NAME_LEN)
        promptText_ += e.ch;
      return Redraw::Input;
    default: return Redraw::None;
  }
}

Redraw Editor::browseKey(const KeyEvent &e) {
  int last = browse_.empty() ? 0 : (int)browse_.size() - 1;
  if (confirmDelete_) {
    if (e.type == KeyType::Char && e.ch == 'y' && !browse_.empty()) {
      std::string name = browse_[browseIndex_].name;
      browse_.erase(browse_.begin() + browseIndex_);
      docs_.removeDoc(name);
      if (name == doc_) setCurrent("");  // text stays on screen, but is unsaved again
      last = browse_.empty() ? 0 : (int)browse_.size() - 1;
      browseIndex_ = browseIndex_ > last ? last : browseIndex_;
      if (browseIndex_ < 0) browseIndex_ = 0;
    }
    confirmDelete_ = false;
    return Redraw::Display;
  }
  int step = (e.mods & MOD_SHIFT) ? BROWSE_ROWS : 1;  // shift = a page at a time
  switch (e.type) {
    case KeyType::Up:
      browseIndex_ = browseIndex_ - step < 0 ? 0 : browseIndex_ - step;
      return Redraw::Display;
    case KeyType::Down:
      browseIndex_ = browseIndex_ + step > last ? last : browseIndex_ + step;
      return Redraw::Display;
    case KeyType::Delete:
      if (browse_.empty()) return Redraw::None;
      confirmDelete_ = true;
      return Redraw::Display;
    case KeyType::Enter:
      if (browse_.empty()) return Redraw::None;
      return openDocument(browse_[browseIndex_].name);
    case KeyType::Escape:
      mode_ = Mode::Edit;
      return Redraw::Display;
    default: return Redraw::None;
  }
}

Redraw Editor::ctrlKey(const KeyEvent &e) {
  if (e.type == KeyType::Escape) {
    powerDown_ = true;
    return Redraw::None;
  }
  if (e.type != KeyType::Char) return Redraw::None;
  switch (e.ch) {
    case 's':
      if (!doc_.empty()) {
        saveCurrent();
        console_ = "[Saved]";
        armMessageClear();
      } else {
        startPrompt(false, "Save as: ", "");
      }
      return Redraw::Display;
    case 'r':
      startPrompt(true, "Rename to: ", doc_);
      return Redraw::Display;
    case 'o':
      startBrowser();
      return Redraw::Display;
    case 'b': text_.toggleFormat(textfmt::BOLD); return Redraw::Input;
    case 'i': text_.toggleFormat(textfmt::ITALIC); return Redraw::Input;
    case 'u': text_.toggleFormat(textfmt::UNDERLINE); return Redraw::Input;
    case 'n':
      archiveCurrent();
      text_.clear();
      scroll_ = 1;
      setCurrent("");
      docs_.saveWorking(text_.lines());
      sessionStartMs_ = nowMs_();
      console_ = "[New]";
      armMessageClear();
      return Redraw::DisplayFull;  // clean slate: a full refresh clears the old page
    default: return Redraw::None;
  }
}

Redraw Editor::editKey(const KeyEvent &e) {
  if (e.mods & MOD_CTRL) return ctrlKey(e);
  if (e.mods & (MOD_ALT | MOD_GUI)) return Redraw::None;
  bool linesChanged = false;
  switch (e.type) {
    case KeyType::Char: linesChanged = text_.typeChar(e.ch); break;
    case KeyType::Tab: linesChanged = text_.typeTab(); break;
    case KeyType::Backspace: text_.backspace(); break;
    case KeyType::Enter:
      if (scroll_ > 1) {  // reviewing older text: jump back to the newest page
        scroll_ = 1;
        return Redraw::Display;
      }
      text_.enter();
      saveCurrent();
      return Redraw::Display;
    case KeyType::Down:
    case KeyType::Right:
      scroll_ = scroll_ - 1 < 1 ? 1 : scroll_ - 1;
      return Redraw::Display;
    case KeyType::Up:
    case KeyType::Left:
      scroll_ = scroll_ + 1 > totalPages() ? totalPages() : scroll_ + 1;
      return Redraw::Display;
    default: return Redraw::None;
  }
  if (linesChanged) return Redraw::Display;
  return scroll_ == 1 ? Redraw::Input : Redraw::None;  // no input line while reviewing older pages
}

Redraw Editor::tick() {
  if (clearArmed_ && (int32_t)(nowMs_() - clearAtMs_) >= 0) {
    clearArmed_ = false;
    return Redraw::Display;  // redraw without the message
  }
  return Redraw::None;
}
