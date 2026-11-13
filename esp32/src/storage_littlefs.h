// Storage on the ESP32's flash (LittleFS). UNTESTED: written from memory of the
// Arduino-ESP32 3.x LittleFS API, not built or run.
#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#include "storage.h"

class LittleFsStorage : public Storage {
 public:
  bool begin() { return LittleFS.begin(true); }  // true = format on first use
  bool readFile(const std::string &path, std::string &out) override;
  bool writeFileAtomic(const std::string &path, const std::string &data) override;
  bool removeFile(const std::string &path) override { return LittleFS.remove(path.c_str()); }
  bool exists(const std::string &path) override { return LittleFS.exists(path.c_str()); }
  bool makeDir(const std::string &path) override {
    return LittleFS.exists(path.c_str()) || LittleFS.mkdir(path.c_str());
  }
  std::vector<FileEntry> listDir(const std::string &dir) override;
};
