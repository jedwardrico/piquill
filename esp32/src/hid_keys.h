// Turns USB/BLE HID keyboard reports into key events. Pure C++ (no Arduino or
// BLE calls) so it can be tested on a PC.
//
// Input is the 8-byte "boot keyboard" input report that BLE (HID over GATT) and
// USB keyboards send: [modifiers, reserved, key1..key6], where the keys are USB
// HID usage codes. BLE keyboards do not auto-repeat, so tick() generates the
// repeats for a held key.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

enum class KeyType : uint8_t {
  Char, Enter, Backspace, Tab, Escape, Delete,
  Left, Right, Up, Down, Home, End, PageUp, PageDown,
};

// Modifier bits in KeyEvent::mods (left and right keys are merged).
constexpr uint8_t MOD_CTRL = 1, MOD_ALT = 2, MOD_GUI = 4, MOD_SHIFT = 8;

struct KeyEvent {
  KeyType type;
  char ch;        // for Char: the character, already shifted / caps-locked
  uint8_t mods;   // MOD_* held when the key went down
};

class KeyDecoder {
 public:
  uint32_t repeatDelayMs = 500;
  uint32_t repeatIntervalMs = 40;

  // Feed one input report. Appends events for keys that were newly pressed.
  // Returns false if the report is not a usable 8-byte keyboard report.
  bool onReport(const uint8_t *data, size_t len, uint32_t nowMs, std::vector<KeyEvent> &out);
  // Call regularly (e.g. every loop). Appends one repeat when one is due.
  void tick(uint32_t nowMs, std::vector<KeyEvent> &out);
  // Forget all held keys (call when the keyboard disconnects).
  void releaseAll();

  bool capsLock() const { return caps_; }

 private:
  bool translate(uint8_t keycode, uint8_t mods, KeyEvent &ev);
  uint8_t prev_[6] = {0};
  bool caps_ = false;
  bool repeating_ = false;
  uint8_t repeatKey_ = 0;
  KeyEvent repeatEvent_{KeyType::Char, 0, 0};
  uint32_t nextRepeatMs_ = 0;
};

// US-layout character for a HID usage code, or 0 if it is not a printable key.
// `shift` selects the shifted symbol; letters are lower case here (caps lock
// is applied by KeyDecoder).
char hidKeycodeToAscii(uint8_t keycode, bool shift);
