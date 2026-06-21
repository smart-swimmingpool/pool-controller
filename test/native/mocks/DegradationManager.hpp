#pragma once
#include <string>

namespace PoolController {

class DegradationManager {
public:
  static void begin() {}
  static void loop() {}
  static void reportSensorDegradation(const std::string &id, bool ok) {}
  static void reportTimeDegradation(int level) {}
  static bool isDegraded() { return false; }
  static int getTimeDegradation() { return 0; }
};

} // namespace PoolController
