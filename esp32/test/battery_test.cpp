// Reads "hi lo" byte pairs on stdin; prints soc, voltage, rate for each, plus the
// smoothed percent for the running sequence of soc values.
#include "battery_math.h"
#include <stdio.h>
int main() {
  int hi, lo;
  battery_math::Smoother sm;
  while (scanf("%d %d", &hi, &lo) == 2)
    printf("%.6f %.9f %.6f %d\n", battery_math::decodeSoc(hi, lo), battery_math::decodeVoltage(hi, lo),
           battery_math::decodeRate(hi, lo), sm.add(battery_math::decodeSoc(hi, lo)));
}
