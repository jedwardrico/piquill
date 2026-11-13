// Debounce and long-press logic for the single power button. Pure C++ (no
// Arduino), so it can be tested on a PC. Feed it the raw pin state (true =
// pressed) every loop; it returns at most one event per call.
#pragma once

#include <stdint.h>

enum class ButtonEvent { None, Pressed, LongPress, Released };

class PowerButton {
 public:
  static constexpr uint32_t DEBOUNCE_MS = 40;     // raw level must hold this long to count
  static constexpr uint32_t LONG_PRESS_MS = 2000; // held this long (after the debounced press)

  // `pressed` is the raw state now, `now` is millis().
  ButtonEvent update(bool pressed, uint32_t now) {
    if (pressed != raw_) {  // raw level changed: restart the debounce timer
      raw_ = pressed;
      rawSince_ = now;
    }
    if (raw_ != stable_ && now - rawSince_ >= DEBOUNCE_MS) {
      stable_ = raw_;
      if (stable_) {
        pressedAt_ = now;
        longFired_ = false;
        return ButtonEvent::Pressed;
      }
      return ButtonEvent::Released;
    }
    if (stable_ && !longFired_ && now - pressedAt_ >= LONG_PRESS_MS) {
      longFired_ = true;
      return ButtonEvent::LongPress;
    }
    return ButtonEvent::None;
  }

  bool pressed() const { return stable_; }
  bool longFired() const { return longFired_; }

 private:
  bool raw_ = false, stable_ = false, longFired_ = false;
  uint32_t rawSince_ = 0, pressedAt_ = 0;
};
