#pragma once
#include "Arduino.h"
#include "DegradationPolicy.hpp"

namespace PoolController {

/**
 * @brief Minimal mock DegradationManager for native tests.
 *
 * Tests that need the real DegradationManager logic should compile
 * src/DegradationManager.cpp from the wrappers directory (so the mock
 * below is used only by other test code that includes this header).
 *
 * The real DegradationManager has the same API — this mock compiles
 * without pulling in NetworkManager/SystemMonitor/TimeClientHelper.
 */
class DegradationManager {
public:
  static void begin() {}
  static void evaluate() {}
  static DegradationLevel getLevel() { return DegradationLevel::NORMAL; }
  static bool isSafe() { return false; }
  static void forceSafeMode() {}
  static const char *levelToString(DegradationLevel level) { return "normal"; }
  static void reportSensorStatus(const char *nodeId, bool valid) {}
  static void unforceSafeMode() {}
};

} // namespace PoolController
