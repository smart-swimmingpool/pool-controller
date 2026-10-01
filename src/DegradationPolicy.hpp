// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DegradationPolicy.hpp
 * @brief Pure, hardware-independent degradation rules (level classification
 *        and persistent low-heap detection), shared by DegradationManager
 *        and SystemMonitor and covered by native unit tests.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * System degradation levels.
 * Order matters: higher numeric value = worse state.
 */
enum class DegradationLevel : uint8_t {
  NORMAL = 0,     // Everything nominal
  NO_WIFI = 1,    // WiFi/MQTT disconnected — local operation still works
  NO_TIME = 2,    // NTP sync lost — timer-based scheduling degraded
  NO_SENSOR = 3,  // One or more temperature sensors failed — cautious defaults
  CRITICAL = 4,   // Critically low memory or forced (boot-loop) — safe mode
};

/**
 * @brief Classify the system state into a degradation level.
 *
 * Only conditions that make relay operation itself unsafe lead to CRITICAL
 * (safe mode, all relays off). Connectivity, time and sensor problems — also
 * in combination — never stop the filter pump: the rules already switch the
 * solar pump off on invalid temperatures, and the filter pump keeps following
 * the timer (or runs continuously when the time is unknown) for water hygiene.
 *
 * @param wifiOk   WiFi connected
 * @param timeOk   time degradation is not RED
 * @param sensorOk both temperature sensors healthy (or not yet reported)
 * @param memoryOk free heap above the low-memory threshold
 */
inline DegradationLevel classifyDegradation(bool wifiOk, bool timeOk, bool sensorOk, bool memoryOk) {
  if (!memoryOk) {
    return DegradationLevel::CRITICAL;
  }
  if (!sensorOk) {
    return DegradationLevel::NO_SENSOR;
  }
  if (!timeOk) {
    return DegradationLevel::NO_TIME;
  }
  if (!wifiOk) {
    return DegradationLevel::NO_WIFI;
  }
  return DegradationLevel::NORMAL;
}

/**
 * @brief Detects a low-memory condition that persists for too long.
 *
 * Feed it periodically with the current low-memory state. update() returns
 * true once the condition has been active without interruption for at least
 * @p limitMs. Uses unsigned arithmetic, so millis() wrap-around is handled.
 */
class PersistentConditionTimer {
public:
  bool update(uint32_t nowMs, bool conditionActive, uint32_t limitMs) {
    if (!conditionActive) {
      active_ = false;
      return false;
    }
    if (!active_) {
      active_ = true;
      sinceMs_ = nowMs;
      return false;
    }
    return (nowMs - sinceMs_) >= limitMs;
  }

  void reset() { active_ = false; }

private:
  bool active_ = false;
  uint32_t sinceMs_ = 0;
};

}  // namespace PoolController
