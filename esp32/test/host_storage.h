// Storage backed by a folder on the PC, for tests.
#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>

#include "storage.h"

class HostStorage : public Storage {
 public:
  explicit HostStorage(const std::string &root) : root_(root) { std::filesystem::create_directories(root_); }
  std::string real(const std::string &p) const { return root_ + p; }
  bool readFile(const std::string &path, std::string &out) override {
    std::ifstream f(real(path), std::ios::binary);
    if (!f) return false;
    std::stringstream ss; ss << f.rdbuf(); out = ss.str();
    return true;
  }
  bool writeFileAtomic(const std::string &path, const std::string &data) override {
    std::string tmp = real(path) + ".tmp";
    { std::ofstream f(tmp, std::ios::binary); if (!f) return false; f << data; }
    std::error_code ec;
    std::filesystem::rename(tmp, real(path), ec);
    return !ec;
  }
  bool removeFile(const std::string &path) override { return std::filesystem::remove(real(path)); }
  bool exists(const std::string &path) override { return std::filesystem::exists(real(path)); }
  bool makeDir(const std::string &path) override { std::error_code ec; std::filesystem::create_directories(real(path), ec); return !ec; }
  std::vector<FileEntry> listDir(const std::string &dir) override {
    std::vector<FileEntry> v;
    std::error_code ec;
    for (auto &e : std::filesystem::directory_iterator(real(dir), ec))
      if (e.is_regular_file()) v.push_back({e.path().filename().string(), 0});
    return v;
  }
 private:
  std::string root_;
};
