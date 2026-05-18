// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

/**
 * Degradation Manager — central system health monitor
 *
 * Tracks the overall system state and provides a single source of truth for
 * degradation-aware decisions across all components (timer fallback,
 * MQTT publish queue, sensor recovery etc).
 *
 * Design: stateless singleton (like SystemMonitor). Components call
 * evaluate() on every loop() — the manager internally rate-limits
 * checks to avoid overhead.
 *
 * Integration points:
 *   - PoolController::loop() calls DegradationManager::evaluate()
 *   - DallasTemperatureNode calls reportSensorStatus()
 *   - RuleAuto / RuleTimer query getLevel() for time degradation
 *   - MqttInterface queries getLevel() for publish decisions
 */

#include <Arduino.h>

namespace PoolController {

/**
 * System degradation levels.
 * Order matters: higher numeric value = worse state.
 */
enum class DegradationLevel : uint8_t {
  NORMAL = 0,    // Everything nominal
  NO_WIFI = 1,   // WiFi/MQTT disconnected — local operation still works
  NO_TIME = 2,   // NTP sync lost — timer-based scheduling degraded
  NO_SENSOR = 3, // One or more temperature sensors failed — cautious defaults
  CRITICAL = 4,  // Multiple failures or critically low memory — safe mode
};

/**
 * Central health monitor.
 *
 * Typical usage in PoolController::loop():
 * @code
 * SystemMonitor::feedWatchdog();
 * SystemMonitor::checkMemory();
 * DegradationManager::evaluate();
 * Homie.loop();
 * @endcode
 */
class DegradationManager {
public:
  /**
   * Initialize with NORMAL level.
   */
  static void begin();

  /**
   * Evaluate current system state and transition if needed.
   * Call this once per loop() iteration — internal rate-limiting
   * prevents excessive checks.
   */
  static void evaluate();

  /**
   * Get current degradation level.
   */
  static DegradationLevel getLevel();

  /**
   * True when level >= CRITICAL — system should enter safe mode
   * (all relays off, minimal operations only).
   */
  static bool isSafe();

  /**
   * Immediately force CRITICAL / safe mode.
   * Used by boot-loop detection (P8) and other pre-initialization checks.
   */
  static void forceSafeMode();

  /**
   * Human-readable level name (for logging / MQTT).
   */
  static const char *levelToString(DegradationLevel level);

  /**
   * Report sensor health status from temperature nodes.
   * Call with false when a sensor read fails, true on successful read.
   */
  static void reportSensorStatus(bool valid);

private:
  static DegradationLevel currentLevel_;
  static DegradationLevel previousLevel_;
  static bool sensorValid_;
  static bool forcedSafeMode_;
  static unsigned long lastEvaluationMs_;

  static constexpr unsigned long EVALUATION_INTERVAL_MS = 5000;

  /**
   * Called on every level transition.
   * Logs the event and, if MQTT is connected, publishes a state update.
   */
  static void onTransition();

  /**
   * Determine the current level from first principles.
   * Called at most once per EVALUATION_INTERVAL_MS.
   */
  static DegradationLevel evaluateLevel();
};

}  // namespace PoolController
