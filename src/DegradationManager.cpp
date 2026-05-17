// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "DegradationManager.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"

#include <Homie.h>

namespace PoolController {

// Static member definitions
DegradationLevel DegradationManager::currentLevel_ = DegradationLevel::NORMAL;
DegradationLevel DegradationManager::previousLevel_ = DegradationLevel::NORMAL;
bool DegradationManager::sensorValid_ = true;
unsigned long DegradationManager::lastEvaluationMs_ = 0;

// ===========================================================================
// Public API
// ===========================================================================

void DegradationManager::begin() {
  currentLevel_ = DegradationLevel::NORMAL;
  previousLevel_ = DegradationLevel::NORMAL;
  sensorValid_ = true;
  lastEvaluationMs_ = 0;

  Homie.getLogger() << F("✓ DegradationManager initialized") << endl;
}

void DegradationManager::evaluate() {
  unsigned long now = millis();

  // Rate-limit evaluation to EVALUATION_INTERVAL_MS
  if (now - lastEvaluationMs_ < EVALUATION_INTERVAL_MS) {
    return;
  }
  lastEvaluationMs_ = now;

  DegradationLevel newLevel = evaluateLevel();

  if (newLevel != currentLevel_) {
    previousLevel_ = currentLevel_;
    currentLevel_ = newLevel;
    onTransition();
  }
}

DegradationLevel DegradationManager::getLevel() {
  return currentLevel_;
}

bool DegradationManager::isSafe() {
  return currentLevel_ >= DegradationLevel::CRITICAL;
}

const char *DegradationManager::levelToString(DegradationLevel level) {
  switch (level) {
    case DegradationLevel::NORMAL:
      return "normal";
    case DegradationLevel::NO_WIFI:
      return "no-wifi";
    case DegradationLevel::NO_TIME:
      return "no-time";
    case DegradationLevel::NO_SENSOR:
      return "no-sensor";
    case DegradationLevel::CRITICAL:
      return "critical";
    default:
      return "unknown";
  }
}

void DegradationManager::reportSensorStatus(bool valid) {
  sensorValid_ = valid;
}

// ===========================================================================
// Private helpers
// ===========================================================================

void DegradationManager::onTransition() {
  // Log the transition
  Homie.getLogger() << F("⚙ Degradation: ")
                    << levelToString(previousLevel_)
                    << F(" → ")
                    << levelToString(currentLevel_)
                    << endl;

  // Additional per-level actions
  switch (currentLevel_) {
    case DegradationLevel::NORMAL:
      Homie.getLogger() << F("✓ All systems nominal") << endl;
      break;

    case DegradationLevel::NO_WIFI:
      Homie.getLogger() << F("⚠ WiFi/MQTT disconnected — operating offline") << endl;
      Homie.getLogger() << F("  All control rules still active") << endl;
      break;

    case DegradationLevel::NO_TIME:
      Homie.getLogger() << F("⚠ NTP time sync lost — timer scheduling degraded") << endl;
      Homie.getLogger() << F("  Timer mode falls back to auto mode") << endl;
      break;

    case DegradationLevel::NO_SENSOR:
      Homie.getLogger() << F("⚠ Temperature sensor fault — using cautious defaults") << endl;
      Homie.getLogger() << F("  Auto mode may not function correctly") << endl;
      break;

    case DegradationLevel::CRITICAL:
      Homie.getLogger() << F("✖ CRITICAL: Multiple system failures detected!") << endl;
      Homie.getLogger() << F("  Entering safe mode — all relays off") << endl;
      break;
  }

  // MQTT notification — best-effort, no retry
  if (Homie.isConnected()) {
    Homie.getLogger() << F("  Published via MQTT") << endl;
    // Full MQTT publish of degradation state will be handled by
    // OperationModeNode or MqttInterface in P4+
  }
}

DegradationLevel DegradationManager::evaluateLevel() {
  // Gather system health signals
  bool wifiOk = Homie.isConnected();
  bool timeOk = isTimeSyncValid();
  bool memoryOk = SystemMonitor::isHealthy();
  bool sensorOk = sensorValid_;

  // Count active failures (memory failure = CRITICAL immediately)
  uint8_t failureCount = 0;
  if (!wifiOk) failureCount++;
  if (!timeOk) failureCount++;
  if (!sensorOk) failureCount++;

  // --- Decision logic ---

  // Memory critical → always CRITICAL (may reboot soon anyway)
  if (!memoryOk) {
    return DegradationLevel::CRITICAL;
  }

  // Multiple concurrent failures → CRITICAL
  if (failureCount >= 2) {
    return DegradationLevel::CRITICAL;
  }

  // Single failures — return the most specific level
  if (!sensorOk) return DegradationLevel::NO_SENSOR;
  if (!timeOk) return DegradationLevel::NO_TIME;
  if (!wifiOk) return DegradationLevel::NO_WIFI;

  return DegradationLevel::NORMAL;
}

}  // namespace PoolController
