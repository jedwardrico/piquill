// Reads hex-encoded strings on stdin, prints textfmt results for each.
#include "textfmt.h"
#include <stdio.h>
#include <string>
static std::string unhex(const std::string &h) {
  std::string s;
  for (size_t i = 0; i + 1 < h.size(); i += 2) s += (char)strtol(h.substr(i, 2).c_str(), nullptr, 16);
  return s;
}
static std::string hex(const std::string &s) {
  std::string h; char b[3];
  for (unsigned char c : s) { snprintf(b, 3, "%02x", c); h += b; }
  return h;
}
int main() {
  char buf[4096];
  while (fgets(buf, sizeof buf, stdin)) {
    std::string h(buf); while (!h.empty() && (h.back() == '\n')) h.pop_back();
    std::string s = unhex(h);
    textfmt::State st = textfmt::formatState(s);
    printf("%s %s %s %s %zu", hex(textfmt::toMarkdown(s)).c_str(), hex(textfmt::fromMarkdown(s)).c_str(),
           hex(textfmt::activeMarkers(st)).c_str(), hex(textfmt::stripMarkers(s)).c_str(), textfmt::visLen(s));
    for (size_t n = 0; n < 6; n++) printf(" %zu", textfmt::rawIndex(s, n));
    printf(" %s %s %s %zu\n", hex(textfmt::toggle(s, textfmt::BOLD)).c_str(), hex(textfmt::toggle(s, textfmt::ITALIC)).c_str(),
           hex(textfmt::toggle(s, textfmt::UNDERLINE)).c_str(), textfmt::wordCount(textfmt::stripMarkers(s)));
  }
}
