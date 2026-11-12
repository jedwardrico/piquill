#include "text_buffer.h"

#include <ctype.h>

bool TextBuffer::insert(char c) {
  input_.insert(input_.begin() + cursor_, c);
  cursor_++;
  return false;
}

bool TextBuffer::typeChar(char c) {
  if (c == ' ') {
    insert(c);
    if (input_.size() > COLS) {
      lines_.push_back(input_);
      input_.clear();
      cursor_ = 0;
      return true;
    }
    return false;
  }
  insert(c);
  if (input_.size() > COLS) {
    // Find the last space before the limit; a long unbroken word is hard-broken.
    size_t limit = COLS;
    size_t lastSpace = limit == 0 ? std::string::npos : input_.rfind(' ', limit - 1);
    std::string sentence;
    if (lastSpace == std::string::npos || lastSpace == 0) {
      sentence = input_.substr(0, limit);
      input_ = input_.substr(limit);
    } else {
      sentence = input_.substr(0, lastSpace);
      input_ = input_.substr(lastSpace + 1);
    }
    lines_.push_back(sentence);
    cursor_ = input_.size();
    return true;
  }
  return false;
}

bool TextBuffer::typeTab() {
  insert(' ');
  insert(' ');
  if (input_.size() > COLS) {
    lines_.push_back(input_);
    input_.clear();
    cursor_ = 0;
    return true;
  }
  return false;
}

bool TextBuffer::enter() {
  lines_.push_back(input_);
  input_.clear();
  cursor_ = 0;
  return true;
}

bool TextBuffer::backspace() {
  if (cursor_ > 0) {
    input_.erase(cursor_ - 1, 1);
    cursor_--;
  }
  return false;
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

size_t TextBuffer::wordCount() const {
  size_t words = 0;
  auto count = [&](const std::string &s) {
    bool in = false;
    for (char c : s) {
      bool sp = isspace((unsigned char)c);
      if (!sp && !in) words++;
      in = !sp;
    }
  };
  for (auto &l : lines_) count(l);
  count(input_);
  return words;
}

size_t TextBuffer::charCount() const {
  size_t n = 0;
  for (auto &l : lines_) n += l.size() + 1;
  return n + input_.size();
}
