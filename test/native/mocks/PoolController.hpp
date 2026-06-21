#pragma once
#include <stdint.h>

namespace PoolController {

class Utils {
public:
  static bool shouldMeasure(unsigned long &last, unsigned long interval) {
    unsigned long now = millis();
    if (now - last >= interval) {
      last = now;
      return true;
    }
    return false;
  }
};

} // namespace PoolController
