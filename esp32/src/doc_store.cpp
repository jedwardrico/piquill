#include "doc_store.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <algorithm>

#include "textfmt.h"

void DocStore::begin() { fs_.makeDir("/documents"); }

std::string DocStore::sanitizeName(const std::string &in) {
  std::string name;
  for (char c : in) {
    bool ok = isalnum((unsigned char)c) || c == ' ' || c == '_' || c == '.' || c == '-';
    name += ok ? c : '_';
  }
  auto strip = [](std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && s[a] == ' ') a++;
    while (b > a && s[b - 1] == ' ') b--;
    s = s.substr(a, b - a);
  };
  strip(name);
  size_t dots = 0;
  while (dots < name.size() && name[dots] == '.') dots++;
  name = name.substr(dots);
  for (const char *ext : {".md", ".txt"}) {
    size_t n = strlen(ext);
    std::string lower = name;
    for (char &c : lower) c = tolower((unsigned char)c);
    if (lower.size() >= n && lower.compare(lower.size() - n, n, ext) == 0) name = name.substr(0, name.size() - n);
  }
  strip(name);
  if (name.size() > MAX_NAME_LEN) name = name.substr(0, MAX_NAME_LEN);
  return name;
}

static std::vector<std::string> splitLines(const std::string &content) {
  std::vector<std::string> lines;
  size_t pos = 0;
  while (pos < content.size()) {
    size_t nl = content.find('\n', pos);
    if (nl == std::string::npos) {
      lines.push_back(content.substr(pos));
      break;
    }
    lines.push_back(content.substr(pos, nl - pos));
    pos = nl + 1;
  }
  return lines;
}

std::vector<std::string> DocStore::loadLines(const std::string &path) {
  std::string content;
  std::vector<std::string> lines;
  if (!fs_.readFile(path, content)) return lines;
  for (auto &l : splitLines(content)) lines.push_back(textfmt::fromMarkdown(l));
  return lines;
}

bool DocStore::saveLines(const std::string &path, const std::vector<std::string> &lines) {
  std::string data;
  for (auto &l : lines) data += textfmt::toMarkdown(l) + '\n';
  return fs_.writeFileAtomic(path, data);
}

bool DocStore::saveDoc(const std::string &name, const std::vector<std::string> &lines) {
  bool ok = saveLines(docPath(name), lines);
  if (ok) touchRecent(name);
  return ok;
}

bool DocStore::removeDoc(const std::string &name) {
  forgetRecent(name);
  return fs_.removeFile(docPath(name));
}

static std::vector<std::string> readNameList(Storage &fs, const char *path) {
  std::string c;
  std::vector<std::string> v;
  if (fs.readFile(path, c)) v = splitLines(c);
  return v;
}

void DocStore::touchRecent(const std::string &name) {
  auto v = readNameList(fs_, "/recent.txt");
  v.erase(std::remove(v.begin(), v.end(), name), v.end());
  v.insert(v.begin(), name);
  if (v.size() > 200) v.resize(200);
  std::string data;
  for (auto &n : v) data += n + '\n';
  fs_.writeFileAtomic("/recent.txt", data);
}

void DocStore::forgetRecent(const std::string &name) {
  auto v = readNameList(fs_, "/recent.txt");
  auto it = std::remove(v.begin(), v.end(), name);
  if (it == v.end()) return;
  v.erase(it, v.end());
  std::string data;
  for (auto &n : v) data += n + '\n';
  fs_.writeFileAtomic("/recent.txt", data);
}

std::vector<std::string> DocStore::listDocuments() {
  std::vector<std::string> present;
  for (auto &e : fs_.listDir("/documents")) {
    if (e.name.size() > 3 && e.name.compare(e.name.size() - 3, 3, ".md") == 0)
      present.push_back(e.name.substr(0, e.name.size() - 3));
  }
  std::vector<std::string> out;
  for (auto &n : readNameList(fs_, "/recent.txt")) {
    auto it = std::find(present.begin(), present.end(), n);
    if (it != present.end()) {
      out.push_back(n);
      present.erase(it);
    }
  }
  std::sort(present.begin(), present.end());  // files not in the list: alphabetical
  out.insert(out.end(), present.begin(), present.end());
  return out;
}

DocStore::Info DocStore::docInfo(const std::string &name) {
  Info info{"", 0};
  std::string content;
  if (fs_.readFile(docPath(name), content))
    info.words = textfmt::wordCount(textfmt::stripMarkers(textfmt::fromMarkdown(content)));
  for (auto &e : fs_.listDir("/documents")) {
    if (e.name == name + ".md" && e.mtime > 1700000000u) {  // clock was set
      time_t t = e.mtime;
      struct tm tmv;
      localtime_r(&t, &tmv);
      char buf[16];
      strftime(buf, sizeof buf, "%Y-%m-%d", &tmv);
      info.date = buf;
    }
  }
  return info;
}

void DocStore::rememberCurrent(const std::string &name) { fs_.writeFileAtomic("/current_doc.txt", name); }

std::string DocStore::loadCurrentName() {
  std::string c;
  if (!fs_.readFile("/current_doc.txt", c)) return "";
  std::string name = sanitizeName(c);
  return (!name.empty() && docExists(name)) ? name : "";
}

std::string DocStore::newArchiveName() {
  time_t t = clock_ ? clock_() : 0;
  if (t > 1700000000) {  // clock is set: name it by date and time
    struct tm tmv;
    localtime_r(&t, &tmv);
    char buf[32];
    strftime(buf, sizeof buf, "untitled_%Y%m%d_%H%M%S", &tmv);
    std::string base = buf, name = base;
    for (int n = 2; docExists(name); n++) name = base + "_" + std::to_string(n);
    return name;
  }
  for (int i = 1;; i++) {
    std::string n = "untitled_" + std::to_string(i);
    if (!docExists(n)) return n;
  }
}
