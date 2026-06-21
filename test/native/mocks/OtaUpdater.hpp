#pragma once
#include <string>
#include "Arduino.h"

namespace PoolController {

class OtaUpdater {
public:
  static void begin() {}
  static void loop() {}
  static String getCurrentVersion() { return String("3.3.0"); }
  static String getLatestVersion() { return String("3.3.0"); }
  static bool isUpdateAvailable() { return false; }
  static bool isUpdateInProgress() { return false; }
  static bool checkForUpdate() { return false; }
  static int getProgress() { return 0; }
  static String getStatusMessage() { return String(""); }
  static String getReleaseUrl() { return String(""); }
  static void startUpdate() {}

  // Flash space checking (added for security tests)
  static bool hasSufficientSpace(size_t firmwareSize) {
    // Simulate: 4MB available, 15% safety margin, 1MB minimum
    size_t available = 4UL * 1024UL * 1024UL;  // 4MB
    size_t required = static_cast<size_t>(firmwareSize * 1.15f);
    return required <= available && available >= (1024UL * 1024UL);
  }

  static size_t getAvailableFlashSpace() {
    return 4UL * 1024UL * 1024UL;  // 4MB in native tests
  }
};

} // namespace PoolController
