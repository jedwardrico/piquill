#include "settings.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main() {
  // the example file shipped in the repo parses, with its placeholders
  Settings a = parseSettings(
      "# comment\n"
      "wifi_ssid=Home Net\n"
      "wifi_password=pa=ss word\n"
      "timezone=EST5EDT,M3.2.0,M11.1.0\n");
  CHECK(a.wifiSsid == "Home Net" && a.wifiPassword == "pa=ss word");
  CHECK(a.timezone == "EST5EDT,M3.2.0,M11.1.0" && a.hasWifi());

  // Windows line endings, spaces around =, upper-case keys, quotes keep spaces
  Settings b = parseSettings("  WIFI_SSID = Cafe \r\nWifi_Password = \" spaced \" \r\n  TimeZone=JST-9\r\n");
  CHECK(b.wifiSsid == "Cafe" && b.wifiPassword == " spaced " && b.timezone == "JST-9");

  // missing / empty values: no wifi, timezone defaults to UTC; junk is ignored
  Settings c = parseSettings("nonsense\n=novalue\nunknown=1\n\n   \n#wifi_ssid=commented\ntimezone=\n");
  CHECK(!c.hasWifi() && c.timezone == "UTC0");
  Settings d = parseSettings("");
  CHECK(!d.hasWifi() && d.timezone == "UTC0");

  // last one wins, no trailing newline needed
  Settings e = parseSettings("wifi_ssid=a\nwifi_ssid=b");
  CHECK(e.wifiSsid == "b");

  if (fails) { printf("%d failure(s)\n", fails); return 1; }
  printf("settings: OK\n");
  return 0;
}
