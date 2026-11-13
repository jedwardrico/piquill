#include "storage_littlefs.h"

bool LittleFsStorage::readFile(const std::string &path, std::string &out) {
  File f = LittleFS.open(path.c_str(), "r");
  if (!f || f.isDirectory()) return false;
  out.resize(f.size());
  size_t n = out.empty() ? 0 : f.read((uint8_t *)&out[0], out.size());
  out.resize(n);
  f.close();
  return true;
}

bool LittleFsStorage::writeFileAtomic(const std::string &path, const std::string &data) {
  std::string tmp = path + ".tmp";
  File f = LittleFS.open(tmp.c_str(), "w");
  if (!f) return false;
  size_t n = data.empty() ? 0 : f.write((const uint8_t *)data.data(), data.size());
  f.close();
  if (n != data.size()) {
    LittleFS.remove(tmp.c_str());
    return false;
  }
  // LittleFS rename replaces an existing destination atomically
  return LittleFS.rename(tmp.c_str(), path.c_str());
}

std::vector<FileEntry> LittleFsStorage::listDir(const std::string &dir) {
  std::vector<FileEntry> out;
  File d = LittleFS.open(dir.c_str());
  if (!d || !d.isDirectory()) return out;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    if (f.isDirectory()) continue;
    std::string name = f.name();
    size_t slash = name.rfind('/');
    if (slash != std::string::npos) name = name.substr(slash + 1);
    out.push_back({name, (uint32_t)f.getLastWrite()});
  }
  return out;
}
