// Text model for the typewriter: finished lines plus the line being typed.
//
// C++ port of the typing logic in src/main.py (without bold/italic markers,
// which are not ported yet). Behaviour matches the Pi version:
//  - a line that grows past COLS visible characters is word-wrapped at the last
//    space, or hard-broken if the word has no space;
//  - a space/tab that overflows pushes the whole line and starts an empty one;
//  - Enter pushes the line (even an empty one);
//  - after a wrap the cursor goes to the end of the new input line.
// Pure C++ (no Arduino calls) so it can be tested on a PC.
#pragma once

#include <stddef.h>
#include <string>
#include <vector>

class TextBuffer {
 public:
  static constexpr size_t COLS = 52;        // characters per line
  static constexpr int LINES_ON_SCREEN = 14;  // finished lines shown above the input

  // Returns true if the finished lines changed (a line was pushed or removed),
  // which means the whole text area needs a redraw, not just the input line.
  bool typeChar(char c);   // printable ASCII; space handled like the Pi
  bool typeTab();          // two spaces
  bool enter();
  bool backspace();

  const std::vector<std::string> &lines() const { return lines_; }
  const std::string &input() const { return input_; }
  size_t cursor() const { return cursor_; }
  void setLines(std::vector<std::string> l) { lines_ = std::move(l); }

  // Finished lines visible on a page: scroll = 1 is the newest page, 2 the page
  // before it, and so on (same as the Pi's scrollindex). Oldest first.
  std::vector<std::string> visibleLines(int scroll = 1) const;

  size_t wordCount() const;
  size_t charCount() const;

 private:
  bool insert(char c);
  std::vector<std::string> lines_;
  std::string input_;
  size_t cursor_ = 0;
};
