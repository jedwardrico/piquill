// User settings read from /config.txt on the board (see config.example.txt).
// Pure C++ so the parser can be tested on a PC.
#pragma once

#include <string>

struct Settings {
  std::string wifiSsid;
  std::string wifiPassword;
  std::string timezone = "UTC0";  // POSIX TZ string, e.g. "EST5EDT,M3.2.0,M11.1.0"
  bool hasWifi() const { return !wifiSsid.empty(); }
};

// One "key=value" per line; blank lines and lines starting with # are ignored;
// keys are case-insensitive; the value may be wrapped in double quotes (to keep
// leading/trailing spaces) and may itself contain '='. Unknown keys are ignored.
// Keys: wifi_ssid, wifi_password, timezone.
Settings parseSettings(const std::string &text);
