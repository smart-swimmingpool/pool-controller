// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DegradationManager.hpp
 * @brief Central system health monitor — degradation detection and level tracking.
 */

#pragma once

#include <Arduino.h>

namespace PoolController {

enum class DegradationLevel : uint8_t {
  NORMAL = 0,
  NO_WIFI = 1,
  NO_TIME = 2,
  NO_SENSOR = 3,
  CRITICAL = 4,
};

class DegradationManager {
public:
  static void begin();
  static void evaluate();
  static DegradationLevel getLevel();
  static bool isSafe();
  static void forceSafeMode();
  static const char *levelToString(DegradationLevel level);
  static void reportSensorStatus(const char *nodeId, bool valid);
  static void unforceSafeMode();

private:
  // Cross-core status: SensorTask writes on Core 0, control loop reads on Core 1.
  // Kept volatile here to preserve PR #170 semantics during the main merge;
  // the review finding to replace this with proper synchronization remains open.
  static volatile bool sensorsEverReported_;
  static DegradationLevel currentLevel_;
  static DegradationLevel previousLevel_;
  static volatile bool poolSensorOk_;
  static volatile bool solarSensorOk_;
  static bool forcedSafeMode_;
  static unsigned long lastEvaluationMs_;

  static constexpr unsigned long EVALUATION_INTERVAL_MS = 5000;

  static void onTransition();
  static DegradationLevel evaluateLevel();
};

}  // namespace PoolController
