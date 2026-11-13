#include "battery.h"

void Battery::begin(int sda, int scl) {
  Wire.begin(sda, scl);
  poll();
}

bool Battery::read(uint8_t reg, uint8_t &hi, uint8_t &lo) {
  Wire.beginTransmission(battery_math::ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)battery_math::ADDRESS, 2) != 2) return false;
  hi = Wire.read();
  lo = Wire.read();
  return true;
}

void Battery::poll() {
  uint8_t hi, lo;
  if (!read(battery_math::REG_SOC, hi, lo)) return;  // a failed read keeps the last good value
  percent_ = smoother_.add(battery_math::decodeSoc(hi, lo));
  present_ = true;
  if (read(battery_math::REG_VCELL, hi, lo)) voltage_ = battery_math::decodeVoltage(hi, lo);
  charging_ = read(battery_math::REG_CRATE, hi, lo) && battery_math::decodeRate(hi, lo) > battery_math::CHARGING_RATE;
}
