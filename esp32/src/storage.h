// Minimal file-system interface so the document code can run on the ESP32
// (LittleFS) and on a PC (for tests). Paths are absolute, like "/documents/a.md".
#pragma once

#include <stdint.h>
#include <string>
#include <vector>

struct FileEntry {
  std::string name;   // base name, no directory
  uint32_t mtime;     // seconds since 1970; only meaningful if the clock was set
};

class Storage {
 public:
  virtual ~Storage() {}
  virtual bool readFile(const std::string &path, std::string &out) = 0;
  // Must be safe against power loss: write a temp file, then rename it over `path`.
  virtual bool writeFileAtomic(const std::string &path, const std::string &data) = 0;
  virtual bool removeFile(const std::string &path) = 0;
  virtual bool exists(const std::string &path) = 0;
  virtual bool makeDir(const std::string &path) = 0;
  virtual std::vector<FileEntry> listDir(const std::string &dir) = 0;
};
