#include "hid_keys.h"

#include <string.h>

namespace {
// Usage 0x04..0x1D = a..z, 0x1E..0x27 = 1..9,0
const char *DIGITS = "1234567890";
const char *DIGITS_SHIFT = "!@#$%^&*()";
// Usage 0x2D..0x38: - = [ ] \ (non-US #) ; ' ` , . /
const char *PUNCT = "-=[]\\#;'`,./";
const char *PUNCT_SHIFT = "_+{}|~:\"~<>?";
}  // namespace

char hidKeycodeToAscii(uint8_t kc, bool shift) {
  if (kc >= 0x04 && kc <= 0x1D) return (char)((shift ? 'A' : 'a') + (kc - 0x04));
  if (kc >= 0x1E && kc <= 0x27) return (shift ? DIGITS_SHIFT : DIGITS)[kc - 0x1E];
  if (kc == 0x2C) return ' ';
  if (kc >= 0x2D && kc <= 0x38) return (shift ? PUNCT_SHIFT : PUNCT)[kc - 0x2D];
  return 0;
}

bool KeyDecoder::translate(uint8_t kc, uint8_t mods, KeyEvent &ev) {
  ev.mods = mods;
  ev.ch = 0;
  switch (kc) {
    case 0x28: case 0x58: ev.type = KeyType::Enter; return true;  // Enter, keypad Enter
    case 0x29: ev.type = KeyType::Escape; return true;
    case 0x2A: ev.type = KeyType::Backspace; return true;
    case 0x2B: ev.type = KeyType::Tab; return true;
    case 0x4C: ev.type = KeyType::Delete; return true;
    case 0x4A: ev.type = KeyType::Home; return true;
    case 0x4B: ev.type = KeyType::PageUp; return true;
    case 0x4D: ev.type = KeyType::End; return true;
    case 0x4E: ev.type = KeyType::PageDown; return true;
    case 0x4F: ev.type = KeyType::Right; return true;
    case 0x50: ev.type = KeyType::Left; return true;
    case 0x51: ev.type = KeyType::Down; return true;
    case 0x52: ev.type = KeyType::Up; return true;
    default: break;
  }
  bool shift = (mods & MOD_SHIFT) != 0;
  bool isLetter = kc >= 0x04 && kc <= 0x1D;
  // caps lock flips the case of letters only
  char c = hidKeycodeToAscii(kc, isLetter ? (shift != caps_) : shift);
  if (!c) return false;
  ev.type = KeyType::Char;
  ev.ch = c;
  return true;
}

bool KeyDecoder::onReport(const uint8_t *d, size_t len, uint32_t nowMs, std::vector<KeyEvent> &out) {
  if (!d || len != 8) return false;
  const uint8_t *keys = d + 2;
  // All six slots = 0x01 means "too many keys pressed" (rollover error): ignore.
  bool rollover = true;
  for (int i = 0; i < 6; i++) if (keys[i] != 0x01) rollover = false;
  if (rollover) return true;

  uint8_t m = d[0];
  uint8_t mods = ((m & 0x11) ? MOD_CTRL : 0) | ((m & 0x44) ? MOD_ALT : 0) |
                 ((m & 0x88) ? MOD_GUI : 0) | ((m & 0x22) ? MOD_SHIFT : 0);

  uint8_t cur[6] = {0};
  int n = 0;
  for (int i = 0; i < 6; i++) if (keys[i] > 0x03) cur[n++] = keys[i];  // skip 0 and error codes

  for (int i = 0; i < n; i++) {
    uint8_t kc = cur[i];
    bool wasDown = false;
    for (int j = 0; j < 6; j++) if (prev_[j] == kc) wasDown = true;
    if (wasDown) continue;
    if (kc == 0x39) {  // caps lock toggles on press, makes no event
      caps_ = !caps_;
      continue;
    }
    KeyEvent ev;
    if (!translate(kc, mods, ev)) continue;
    out.push_back(ev);
    if (ev.type != KeyType::Enter && ev.type != KeyType::Escape) {
      repeating_ = true;
      repeatKey_ = kc;
      repeatEvent_ = ev;
      nextRepeatMs_ = nowMs + repeatDelayMs;
    } else {
      repeating_ = false;
    }
  }

  // stop repeating once the repeated key is no longer held
  if (repeating_) {
    bool held = false;
    for (int i = 0; i < n; i++) if (cur[i] == repeatKey_) held = true;
    if (!held) repeating_ = false;
  }
  memset(prev_, 0, sizeof prev_);
  memcpy(prev_, cur, n);
  return true;
}

void KeyDecoder::tick(uint32_t nowMs, std::vector<KeyEvent> &out) {
  if (!repeating_) return;
  if ((int32_t)(nowMs - nextRepeatMs_) >= 0) {
    out.push_back(repeatEvent_);
    nextRepeatMs_ = nowMs + repeatIntervalMs;
  }
}

void KeyDecoder::releaseAll() {
  memset(prev_, 0, sizeof prev_);
  repeating_ = false;
}
