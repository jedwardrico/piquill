#include "power_button.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

// Run the button with a constant raw level from t0 to t1 (1 ms steps); collect events.
struct Run { int pressed = 0, longp = 0, released = 0; };
static void hold(PowerButton &b, bool level, uint32_t t0, uint32_t t1, Run &r) {
  for (uint32_t t = t0; t < t1; t++) {
    switch (b.update(level, t)) {
      case ButtonEvent::Pressed: r.pressed++; break;
      case ButtonEvent::LongPress: r.longp++; break;
      case ButtonEvent::Released: r.released++; break;
      default: break;
    }
  }
}

int main() {
  { // clean short press
    PowerButton b; Run r;
    hold(b, false, 0, 100, r);
    hold(b, true, 100, 400, r);
    CHECK(r.pressed == 1 && b.pressed());
    hold(b, false, 400, 600, r);
    CHECK(r.pressed == 1 && r.released == 1 && r.longp == 0 && !b.pressed());
  }
  { // contact bounce on press and release is one press
    PowerButton b; Run r;
    hold(b, false, 0, 100, r);
    for (int i = 0; i < 5; i++) { hold(b, true, 100 + i * 10, 105 + i * 10, r); hold(b, false, 105 + i * 10, 110 + i * 10, r); }
    hold(b, true, 150, 500, r);
    for (int i = 0; i < 5; i++) { hold(b, false, 500 + i * 10, 505 + i * 10, r); hold(b, true, 505 + i * 10, 510 + i * 10, r); }
    hold(b, false, 550, 800, r);
    CHECK(r.pressed == 1 && r.released == 1 && r.longp == 0);
  }
  { // glitch shorter than the debounce time is ignored
    PowerButton b; Run r;
    hold(b, false, 0, 100, r);
    hold(b, true, 100, 120, r);
    hold(b, false, 120, 300, r);
    CHECK(r.pressed == 0 && r.released == 0);
  }
  { // long press fires once, then release
    PowerButton b; Run r;
    hold(b, true, 0, 3000, r);
    CHECK(r.pressed == 1 && r.longp == 1 && b.longFired());
    hold(b, false, 3000, 3200, r);
    CHECK(r.released == 1 && r.longp == 1);
  }
  { // just under the long-press time is not a long press
    PowerButton b; Run r;
    hold(b, true, 0, 1900, r);
    hold(b, false, 1900, 2100, r);
    CHECK(r.longp == 0 && r.pressed == 1 && r.released == 1);
  }
  { // second long press after releasing fires again
    PowerButton b; Run r;
    hold(b, true, 0, 2500, r); hold(b, false, 2500, 2700, r);
    hold(b, true, 2700, 5500, r);
    CHECK(r.longp == 2 && r.pressed == 2);
  }
  { // works across millis() wrap-around
    PowerButton b; Run r;
    uint32_t t0 = 0xFFFFFF00u;
    for (uint32_t i = 0; i < 3000; i++) {
      ButtonEvent e = b.update(i >= 200, t0 + i);
      if (e == ButtonEvent::Pressed) r.pressed++;
      if (e == ButtonEvent::LongPress) r.longp++;
    }
    CHECK(r.pressed == 1 && r.longp == 1);
  }
  if (fails == 0) printf("power button: OK\n");
  return fails ? 1 : 0;
}
