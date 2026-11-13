// Battery level from the FeatherS3[D]'s MAX17048 fuel gauge (I2C: SDA GPIO8,
// SCL GPIO9). UNTESTED on hardware. If the gauge does not answer, present() is
// false and the status bar just omits the battery.
#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "battery_math.h"

class Battery {
 public:
  void begin(int sda = 8, int scl = 9);
  void poll();                 // one reading; call every ~30 s
  bool present() const { return present_; }
  int percent() const { return percent_; }
  bool charging() const { return charging_; }
  double voltage() const { return voltage_; }

 private:
  bool read(uint8_t reg, uint8_t &hi, uint8_t &lo);
  bool present_ = false, charging_ = false;
  int percent_ = 0;
  double voltage_ = 0;
  battery_math::Smoother smoother_;
};
