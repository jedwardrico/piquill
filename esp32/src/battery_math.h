// Battery gauge arithmetic (MAX17048), port of the decode functions in
// src/battery.py. Pure C++ so it can be tested on a PC.
#pragma once

#include <stdint.h>

#include <math.h>

#include <algorithm>

namespace battery_math {
constexpr uint8_t ADDRESS = 0x36;
constexpr uint8_t REG_VCELL = 0x02;  // cell voltage, 78.125 uV per bit
constexpr uint8_t REG_SOC = 0x04;    // state of charge: high byte = whole %, low byte = 1/256 %
constexpr uint8_t REG_CRATE = 0x16;  // charge/discharge rate, signed, 0.208 %/hour per bit
constexpr double CHARGING_RATE = 0.5;  // %/hour above which the cell counts as charging

inline double decodeSoc(uint8_t hi, uint8_t lo) {
  double v = hi + lo / 256.0;
  return v < 0 ? 0 : (v > 100 ? 100 : v);
}
inline double decodeVoltage(uint8_t hi, uint8_t lo) { return ((hi << 8) | lo) * 78.125e-6; }
inline double decodeRate(uint8_t hi, uint8_t lo) { return (int16_t)((hi << 8) | lo) * 0.208; }

// Median of the last 5 readings, as a whole percent (smooths jitter).
class Smoother {
 public:
  int add(double soc) {
    if (n_ < 5) vals_[n_++] = soc;
    else { for (int i = 0; i < 4; i++) vals_[i] = vals_[i + 1]; vals_[4] = soc; }
    double tmp[5];
    for (int i = 0; i < n_; i++) tmp[i] = vals_[i];
    std::sort(tmp, tmp + n_);
    double median = (n_ % 2) ? tmp[n_ / 2] : (tmp[n_ / 2 - 1] + tmp[n_ / 2]) / 2;
    return (int)nearbyint(median);  // half to even, like Python's round() in battery.py
  }
 private:
  double vals_[5];
  int n_ = 0;
};
}  // namespace battery_math
