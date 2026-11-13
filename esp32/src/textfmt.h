// Inline text formatting (bold / italic / underline) and Markdown conversion.
// C++ port of src/textfmt.py. Formatting lives in the text as invisible control
// characters that toggle a style on/off ("plain \x01bold\x01 plain"). They take
// no screen width and are ignored by counts. Every line is self-contained: a
// style still active at a line break is re-opened at the start of the next line.
#pragma once

#include <stddef.h>
#include <string>

namespace textfmt {
constexpr char BOLD = '\x01';
constexpr char ITALIC = '\x02';
constexpr char UNDERLINE = '\x03';

struct State {
  bool bold = false, italic = false, underline = false;
  bool any() const { return bold || italic || underline; }
};

bool isMarker(char c);
std::string stripMarkers(const std::string &s);
size_t visLen(const std::string &s);                 // characters that take width
size_t rawIndex(const std::string &s, size_t nvis);  // index after the first nvis visible chars
State formatState(const std::string &s);             // styles in effect at the end of s
std::string activeMarkers(const State &st);          // markers that turn st on
std::string toggle(const std::string &s, char marker);  // pressing a shortcut twice cancels

// Markdown on disk: bold **x**, italic *x*, underline <u>x</u>. One line at a time.
std::string toMarkdown(const std::string &line);
std::string fromMarkdown(const std::string &line);

// Python-style helpers used by the editor and tests.
size_t wordCount(const std::string &s);  // whitespace-separated words, markers ignored
}  // namespace textfmt
