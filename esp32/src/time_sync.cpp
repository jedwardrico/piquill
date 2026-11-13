#include "time_sync.h"

#include <WiFi.h>
#include <time.h>

static const time_t VALID_AFTER = 1700000000;  // Nov 2023: anything earlier is the unset clock

bool clockIsSet() { return time(nullptr) > VALID_AFTER; }

bool syncTime(const Settings &s, uint32_t timeoutMs) {
  // Local time for dates, whether or not we sync (the clock may survive a restart).
  setenv("TZ", s.timezone.c_str(), 1);
  tzset();

  if (clockIsSet()) {
    Serial.println("Time: clock already set");
    return true;
  }
  if (!s.hasWifi()) {
    Serial.println("Time: no wifi_ssid in /config.txt, clock not set");
    return false;
  }

  Serial.printf("Time: joining Wi-Fi '%s'...\n", s.wifiSsid.c_str());
  uint32_t start = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(s.wifiSsid.c_str(), s.wifiPassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeoutMs) {
      Serial.println("Time: could not join Wi-Fi (wrong name/password, or out of range)");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      return false;
    }
    delay(100);
  }

  configTzTime(s.timezone.c_str(), "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  while (!clockIsSet()) {
    if (millis() - start > timeoutMs) {
      Serial.println("Time: no answer from the time servers (no internet?)");
      break;
    }
    delay(100);
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  bool ok = clockIsSet();
  if (ok) {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    Serial.printf("Time: set, %04d-%02d-%02d %02d:%02d:%02d (%s)\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec, s.timezone.c_str());
  }
  return ok;
}
