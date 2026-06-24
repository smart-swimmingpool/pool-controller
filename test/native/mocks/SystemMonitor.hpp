#pragma once

#include <cstdint>

namespace PoolController {

class SystemMonitor {
public:
  static void begin() {}
  static void loop() {}
  static void checkMemory() {}
  static void feedWatchdog() {}
  static uint32_t getBootCount() { return 1; }
  static void reportBoot() {}
  static void clearBootLoopCounter() {}
  static bool isInSafeMode() { return false; }
  static bool isHealthy() { return true; }
  static uint32_t getFreeHeap() { return 180000; }
  static uint32_t getMinFreeHeap() { return 0; }
  static uint32_t getUptimeSeconds() { return 3600; }
  static void reboot() {}
  static bool detectBootLoop() { return false; }
};

} // namespace PoolController
