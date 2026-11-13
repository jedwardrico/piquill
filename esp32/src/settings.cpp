#include "settings.h"

#include <ctype.h>

static std::string trim(const std::string &s) {
  size_t a = 0, b = s.size();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) a++;
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) b--;
  return s.substr(a, b - a);
}

Settings parseSettings(const std::string &text) {
  Settings st;
  size_t pos = 0;
  while (pos <= text.size()) {
    size_t nl = text.find('\n', pos);
    std::string line = trim(text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
    pos = nl == std::string::npos ? text.size() + 1 : nl + 1;
    if (line.empty() || line[0] == '#') continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos) continue;
    std::string key = trim(line.substr(0, eq));
    for (char &c : key) c = tolower((unsigned char)c);
    std::string val = trim(line.substr(eq + 1));
    if (val.size() >= 2 && val.front() == '"' && val.back() == '"') val = val.substr(1, val.size() - 2);
    if (key == "wifi_ssid") st.wifiSsid = val;
    else if (key == "wifi_password") st.wifiPassword = val;
    else if (key == "timezone") st.timezone = val.empty() ? "UTC0" : val;
  }
  return st;
}
