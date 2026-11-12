#include "hid_keys.h"
#include <stdio.h>
#include <string.h>
#include <string>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static std::vector<KeyEvent> feed(KeyDecoder &d, uint32_t t, uint8_t mod, uint8_t k1 = 0, uint8_t k2 = 0,
                                  uint8_t k3 = 0, uint8_t k4 = 0, uint8_t k5 = 0, uint8_t k6 = 0) {
  uint8_t r[8] = {mod, 0, k1, k2, k3, k4, k5, k6};
  std::vector<KeyEvent> out;
  CHECK(d.onReport(r, 8, t, out));
  return out;
}
static std::string chars(const std::vector<KeyEvent> &v) {
  std::string s;
  for (auto &e : v) s += e.type == KeyType::Char ? e.ch : '?';
  return s;
}

int main() {
  // full US character table, unshifted and shifted
  std::string lower, upper, dig, digS, p, pS;
  for (int k = 0x04; k <= 0x1D; k++) { lower += hidKeycodeToAscii(k, false); upper += hidKeycodeToAscii(k, true); }
  for (int k = 0x1E; k <= 0x27; k++) { dig += hidKeycodeToAscii(k, false); digS += hidKeycodeToAscii(k, true); }
  for (int k = 0x2D; k <= 0x38; k++) { p += hidKeycodeToAscii(k, false); pS += hidKeycodeToAscii(k, true); }
  CHECK(lower == "abcdefghijklmnopqrstuvwxyz");
  CHECK(upper == "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  CHECK(dig == "1234567890" && digS == "!@#$%^&*()");
  CHECK(p == "-=[]\\#;'`,./" && pS == "_+{}|~:\"~<>?");
  CHECK(hidKeycodeToAscii(0x2C, false) == ' ');
  CHECK(hidKeycodeToAscii(0x28, false) == 0);  // Enter is not a character

  // press, release, press; shift; held key is not retyped
  KeyDecoder d;
  CHECK(chars(feed(d, 0, 0, 0x04)) == "a");
  CHECK(chars(feed(d, 10, 0, 0x04)).empty());          // still held
  CHECK(chars(feed(d, 20, 0)).empty());                // released
  CHECK(chars(feed(d, 30, 0x02, 0x04)) == "A");        // left shift + a
  CHECK(chars(feed(d, 40, 0x20, 0x04, 0x05)) == "B");  // right shift, a held, b new
  feed(d, 50, 0);

  // rollover of two new keys in one report keeps report order
  CHECK(chars(feed(d, 60, 0, 0x0B, 0x0C)) == "hi");
  feed(d, 70, 0);

  // rollover error report is ignored and does not clear held keys
  CHECK(chars(feed(d, 80, 0, 0x04)) == "a");
  CHECK(feed(d, 90, 0, 1, 1, 1, 1, 1, 1).empty());
  CHECK(chars(feed(d, 95, 0, 0x04)).empty());
  feed(d, 100, 0);

  // special keys
  auto v = feed(d, 110, 0, 0x28, 0x2A, 0x2B, 0x52);
  CHECK(v.size() == 4 && v[0].type == KeyType::Enter && v[1].type == KeyType::Backspace &&
        v[2].type == KeyType::Tab && v[3].type == KeyType::Up);
  feed(d, 120, 0);

  // ctrl is reported in mods, shift is applied to the char
  v = feed(d, 130, 0x01, 0x16);  // ctrl+s
  CHECK(v.size() == 1 && v[0].ch == 's' && (v[0].mods & MOD_CTRL));
  feed(d, 140, 0);

  // caps lock: toggles letters only, not digits or punctuation
  feed(d, 150, 0, 0x39); feed(d, 160, 0);
  CHECK(d.capsLock());
  CHECK(chars(feed(d, 170, 0, 0x04)) == "A");
  feed(d, 175, 0);
  CHECK(chars(feed(d, 180, 0, 0x1E)) == "1");
  feed(d, 185, 0);
  CHECK(chars(feed(d, 190, 0x02, 0x04)) == "a");  // shift + caps = lower
  feed(d, 195, 0);
  feed(d, 200, 0, 0x39); feed(d, 210, 0);
  CHECK(!d.capsLock());

  // bad length is rejected
  uint8_t bad[9] = {0};
  std::vector<KeyEvent> o;
  CHECK(!d.onReport(bad, 9, 0, o) && o.empty());
  CHECK(!d.onReport(bad, 0, 0, o));

  // repeat: nothing before the delay, then every interval until released
  KeyDecoder r;
  r.repeatDelayMs = 500; r.repeatIntervalMs = 40;
  CHECK(chars(feed(r, 1000, 0, 0x04)) == "a");
  std::vector<KeyEvent> t;
  r.tick(1499, t); CHECK(t.empty());
  r.tick(1500, t); CHECK(t.size() == 1 && t[0].ch == 'a');
  r.tick(1520, t); CHECK(t.size() == 1);
  r.tick(1540, t); CHECK(t.size() == 2);
  feed(r, 1545, 0);  // release
  r.tick(2000, t); CHECK(t.size() == 2);
  // repeat follows the newest key; releasing an older key does not stop it
  t.clear();
  feed(r, 3000, 0, 0x04, 0x05);  // a then b held
  feed(r, 3100, 0, 0x05);        // a released, b still held
  r.tick(3600, t); CHECK(t.size() == 1 && t[0].ch == 'b');
  feed(r, 3700, 0, 0x28);        // Enter: no repeat
  t.clear(); r.tick(5000, t); CHECK(t.empty());
  // releaseAll
  feed(r, 6000, 0, 0x04); r.releaseAll(); t.clear(); r.tick(7000, t); CHECK(t.empty());
  CHECK(chars(feed(r, 7100, 0, 0x04)) == "a");  // after releaseAll the key counts as new

  if (fails) { printf("%d failure(s)\n", fails); return 1; }
  printf("hid keys: OK\n");
  return 0;
}
