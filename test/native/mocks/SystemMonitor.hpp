#pragma once

namespace PoolController {

class SystemMonitor {
public:
  static void begin() {}
  static void loop() {}
  static uint32_t getBootCount() { return 1; }
  static void reportBoot() {}
  static void clearBootLoopCounter() {}
  static bool isInSafeMode() { return false; }
};

} // namespace PoolController
