#include "text_buffer.h"

#include "textfmt.h"

void TextBuffer::insert(char c) {
  input_.insert(input_.begin() + cursor_, c);
  cursor_++;
}

void TextBuffer::pushInputAndRestart() {
  lines_.push_back(input_);
  input_ = textfmt::activeMarkers(textfmt::formatState(input_));  // open styles carry over
  cursor_ = input_.size();
}

bool TextBuffer::typeChar(char c) {
  if (c == ' ') {
    insert(c);
    if (textfmt::visLen(input_) > COLS) {
      pushInputAndRestart();
      return true;
    }
    return false;
  }
  insert(c);
  if (textfmt::visLen(input_) > COLS) {
    // Style markers take no width, so work in visible characters. Find the last
    // space before the limit; a long unbroken word is hard-broken at the limit.
    size_t limit = textfmt::rawIndex(input_, COLS);
    size_t lastSpace = limit == 0 ? std::string::npos : input_.rfind(' ', limit - 1);
    std::string sentence;
    std::string rest;
    if (lastSpace == std::string::npos || lastSpace == 0) {
      sentence = input_.substr(0, limit);
      rest = input_.substr(limit);
    } else {
      sentence = input_.substr(0, lastSpace);
      rest = input_.substr(lastSpace + 1);
    }
    input_ = textfmt::activeMarkers(textfmt::formatState(sentence)) + rest;
    lines_.push_back(sentence);
    cursor_ = input_.size();
    return true;
  }
  return false;
}

bool TextBuffer::typeTab() {
  insert(' ');
  insert(' ');
  bool pushed = false;
  if (textfmt::visLen(input_) > COLS) {
    pushInputAndRestart();
    pushed = true;
  }
  cursor_ = input_.size();
  return pushed;
}

bool TextBuffer::enter() {
  pushInputAndRestart();
  return true;
}

bool TextBuffer::backspace() {
  if (cursor_ > 0) {
    input_.erase(cursor_ - 1, 1);
    cursor_--;
  }
  return false;
}

void TextBuffer::toggleFormat(char marker) {
  input_ = textfmt::toggle(input_, marker);
  cursor_ = input_.size();
}

void TextBuffer::setLines(std::vector<std::string> l) {
  lines_ = std::move(l);
  input_.clear();
  cursor_ = 0;
}

void TextBuffer::clear() { setLines({}); }

std::vector<std::string> TextBuffer::docLines() const {
  std::vector<std::string> d = lines_;
  if (!textfmt::stripMarkers(input_).empty()) d.push_back(input_);
  return d;
}

std::vector<std::string> TextBuffer::visibleLines(int scroll) const {
  if (scroll < 1) scroll = 1;
  long n = (long)lines_.size();
  long start = n - (long)LINES_ON_SCREEN * scroll;
  if (start < 0) start = 0;
  long end = start + LINES_ON_SCREEN;
  if (end > n) end = n;
  return std::vector<std::string>(lines_.begin() + start, lines_.begin() + end);
}

int TextBuffer::totalPages() const {
  int p = ((int)lines_.size() + LINES_ON_SCREEN - 1) / LINES_ON_SCREEN;
  return p < 1 ? 1 : p;
}

size_t TextBuffer::wordCount() const {
  size_t words = 0;
  for (auto &l : lines_) words += textfmt::wordCount(textfmt::stripMarkers(l));
  return words + textfmt::wordCount(textfmt::stripMarkers(input_));
}

size_t TextBuffer::charCount() const {
  size_t n = 0;
  for (auto &l : lines_) n += textfmt::visLen(l) + 1;  // +1 for the wrap point
  return n + textfmt::visLen(input_);
}
