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
};

} // namespace PoolController
