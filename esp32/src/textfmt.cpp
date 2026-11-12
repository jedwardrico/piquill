#include "textfmt.h"

#include <ctype.h>
#include <string.h>
#include <vector>

namespace textfmt {

static int order(char c) { return c == BOLD ? 0 : c == ITALIC ? 1 : c == UNDERLINE ? 2 : -1; }

bool isMarker(char c) { return order(c) >= 0; }

std::string stripMarkers(const std::string &s) {
  std::string r;
  for (char c : s) if (!isMarker(c)) r += c;
  return r;
}

size_t visLen(const std::string &s) {
  size_t n = 0;
  for (char c : s) if (!isMarker(c)) n++;
  return n;
}

size_t rawIndex(const std::string &s, size_t nvis) {
  size_t seen = 0;
  for (size_t i = 0; i < s.size(); i++) {
    if (isMarker(s[i])) continue;
    if (seen == nvis) return i;
    seen++;
  }
  return s.size();
}

State formatState(const std::string &s) {
  State st;
  for (char c : s) {
    if (c == BOLD) st.bold = !st.bold;
    else if (c == ITALIC) st.italic = !st.italic;
    else if (c == UNDERLINE) st.underline = !st.underline;
  }
  return st;
}

std::string activeMarkers(const State &st) {
  std::string r;
  if (st.bold) r += BOLD;
  if (st.italic) r += ITALIC;
  if (st.underline) r += UNDERLINE;
  return r;
}

std::string toggle(const std::string &s, char marker) {
  if (!s.empty() && s.back() == marker) return s.substr(0, s.size() - 1);
  return s + marker;
}

// ---- Markdown -----------------------------------------------------------

static bool isEscapeChar(char c) { return c == '\\' || c == '*' || c == '_' || c == '<' || c == '`'; }

static std::string escapeMd(const std::string &s) {
  std::string r;
  for (char c : s) {
    if (isEscapeChar(c)) r += '\\';
    r += c;
  }
  return r;
}

static std::string wrapMd(const std::string &text, bool bold, bool italic, bool underline) {
  std::string stars((bold ? 2 : 0) + (italic ? 1 : 0), '*');
  std::string t = stars + escapeMd(text) + stars;
  return underline ? "<u>" + t + "</u>" : t;
}

static size_t firstNotSpace(const std::string &s) {
  size_t i = 0;
  while (i < s.size() && s[i] == ' ') i++;
  return i;
}

std::string toMarkdown(const std::string &line) {
  struct Run { std::string text; State st; };
  std::vector<Run> runs;
  std::string buf;
  State st;
  for (char c : line) {
    if (isMarker(c)) {
      if (!buf.empty()) runs.push_back({buf, st});
      buf.clear();
      if (c == BOLD) st.bold = !st.bold;
      else if (c == ITALIC) st.italic = !st.italic;
      else st.underline = !st.underline;
    } else {
      buf += c;
    }
  }
  if (!buf.empty()) runs.push_back({buf, st});

  std::string out;
  for (auto &r : runs) {
    if (!r.st.any()) {
      out += escapeMd(r.text);
      continue;
    }
    // Markdown won't close emphasis after whitespace, so keep spaces outside
    size_t lead = firstNotSpace(r.text);
    if (lead == r.text.size()) {  // only spaces
      out += r.text;
      continue;
    }
    size_t end = r.text.size();
    while (end > 0 && r.text[end - 1] == ' ') end--;
    out += r.text.substr(0, lead) + wrapMd(r.text.substr(lead, end - lead), r.st.bold, r.st.italic, r.st.underline) +
           r.text.substr(end);
  }
  return out;
}

std::string fromMarkdown(const std::string &line) {
  std::string out;
  size_t pos = 0, i = 0, n = line.size();
  auto startsWith = [&](size_t at, const char *lit) { return line.compare(at, strlen(lit), lit) == 0; };
  while (i < n) {
    size_t len = 0;
    int kind = 0;  // 1 escape, 2 underline, 3 emphasis
    if (line[i] == '\\' && i + 1 < n && isEscapeChar(line[i + 1])) { len = 2; kind = 1; }
    else if (startsWith(i, "<u>")) { len = 3; kind = 2; }
    else if (startsWith(i, "</u>")) { len = 4; kind = 2; }
    else if (startsWith(i, "***")) { len = 3; kind = 3; }
    else if (startsWith(i, "**")) { len = 2; kind = 3; }
    else if (line[i] == '*') { len = 1; kind = 3; }
    if (!kind) { i++; continue; }
    out += line.substr(pos, i - pos);
    if (kind == 1) out += line[i + 1];
    else if (kind == 2) out += UNDERLINE;
    else {
      if (len >= 2) out += BOLD;
      if (len % 2 == 1) out += ITALIC;
    }
    i += len;
    pos = i;
  }
  out += line.substr(pos);
  return out;
}

size_t wordCount(const std::string &s) {
  size_t words = 0;
  bool in = false;
  for (char c : s) {
    bool sp = isspace((unsigned char)c);
    if (!sp && !in) words++;
    in = !sp;
  }
  return words;
}

}  // namespace textfmt
