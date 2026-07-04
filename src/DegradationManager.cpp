// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DegradationManager.cpp
 * @brief Degradation detection — sensor failure tracking, time degradation,
 *        level computation, and system health aggregation.
 */

#include "DegradationManager.hpp"
#include "NetworkManager.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"  // TimeDegradation, getTimeDegradation
#include "RelayModuleNode.hpp"

namespace PoolController {

// Nodes declared in PoolController.cpp
extern RelayModuleNode poolPumpNode;
extern RelayModuleNode solarPumpNode;

// Static member definitions
DegradationLevel DegradationManager::currentLevel_ = DegradationLevel::NORMAL;
DegradationLevel DegradationManager::previousLevel_ = DegradationLevel::NORMAL;
bool DegradationManager::poolSensorOk_ = false;
bool DegradationManager::solarSensorOk_ = false;
bool DegradationManager::forcedSafeMode_ = false;
bool DegradationManager::sensorsEverReported_ = false;
unsigned long DegradationManager::lastEvaluationMs_ = 0;

// ===========================================================================
// Public API
// ===========================================================================

void DegradationManager::begin() {
  // Don't reset if already forced into safe mode by boot-loop detection
  if (currentLevel_ == DegradationLevel::CRITICAL) {
    previousLevel_ = DegradationLevel::CRITICAL;
    // forcedSafeMode_ is preserved; evaluate() will not downgrade below CRITICAL
    return;
  }
  currentLevel_ = DegradationLevel::NORMAL;
  previousLevel_ = DegradationLevel::NORMAL;
  poolSensorOk_ = false;  // Pessimistic — both probes must report healthy
  solarSensorOk_ = false;
  forcedSafeMode_ = false;
  lastEvaluationMs_ = 0;

  Serial.println(F("✓ DegradationManager initialized"));
}

void DegradationManager::evaluate() {
  unsigned long now = millis();

  // Rate-limit evaluation to EVALUATION_INTERVAL_MS
  if (now - lastEvaluationMs_ < EVALUATION_INTERVAL_MS) {
    return;
  }
  lastEvaluationMs_ = now;

  // If force-safe-mode is active, never auto-downgrade below CRITICAL.
  // Only an explicit disable or reboot can clear forced safe mode.
  if (forcedSafeMode_) {
    if (currentLevel_ != DegradationLevel::CRITICAL) {
      previousLevel_ = currentLevel_;
      currentLevel_ = DegradationLevel::CRITICAL;
      onTransition();
    }
    return;
  }

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

void DegradationManager::forceSafeMode() {
  forcedSafeMode_ = true;
  if (currentLevel_ != DegradationLevel::CRITICAL) {
    previousLevel_ = currentLevel_;
    currentLevel_ = DegradationLevel::CRITICAL;
    onTransition();
  }
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

void DegradationManager::reportSensorStatus(const char *nodeId, bool valid) {
  sensorsEverReported_ = true;  // Mark that at least one sensor has been reported
  if (strcmp(nodeId, "pool-temp") == 0) {
    poolSensorOk_ = valid;
  } else if (strcmp(nodeId, "solar-temp") == 0) {
    solarSensorOk_ = valid;
  }
}

void DegradationManager::unforceSafeMode() {
  forcedSafeMode_ = false;
  if (currentLevel_ == DegradationLevel::CRITICAL) {
    previousLevel_ = currentLevel_;
    currentLevel_ = DegradationLevel::NORMAL;
    onTransition();
  }
}

// ===========================================================================
// Private helpers
// ===========================================================================

void DegradationManager::onTransition() {
  // Log the transition
  Serial.print(F("⚙ Degradation: "));
  Serial.print(levelToString(previousLevel_));
  Serial.print(F(" → "));
  Serial.println(levelToString(currentLevel_));

  // Additional per-level actions
  switch (currentLevel_) {
  case DegradationLevel::NORMAL:
    Serial.println(F("✓ All systems nominal"));
    break;

  case DegradationLevel::NO_WIFI:
    Serial.println(F("⚠ WiFi/MQTT disconnected — operating offline"));
    Serial.println(F("  All control rules still active"));
    break;

  case DegradationLevel::NO_TIME:
    Serial.println(F("⚠ NTP time sync lost — timer scheduling degraded"));
    Serial.println(F("  Timer mode falls back to auto mode"));
    break;

  case DegradationLevel::NO_SENSOR:
    Serial.println(F("⚠ Temperature sensor fault — using cautious defaults"));
    Serial.println(F("  Auto mode may not function correctly"));
    break;

  case DegradationLevel::CRITICAL:
    Serial.println(F("✖ CRITICAL: Multiple system failures detected!"));
    Serial.println(F("  Entering safe mode — all relays off"));
    // De-energize both relays immediately (P1 review fix)
    poolPumpNode.setSwitch(false);
    solarPumpNode.setSwitch(false);
    break;
  }

  // MQTT notification — best-effort, no retry.
  // Full publish is handled by MqttPublisher on its next publish cycle.
  if (NetworkManager::isMqttConnected()) {
    Serial.println(F("  Degradation state will be published via MQTT"));
  }
}

DegradationLevel DegradationManager::evaluateLevel() {
  // Gather system health signals
  bool wifiOk = NetworkManager::isWiFiConnected();

  // If time is RED (lost) but WiFi is up, try an NTP sync immediately.
  // The Time library only syncs every SYNC_INTERVAL (3600s), so without
  // this the system could stay in NO_TIME for an hour after NTP recovers.
  if (wifiOk && (getTimeDegradation() == TimeDegradation::RED)) {
    forceNtpUpdate();
  }

  // Time is OK for GREEN + YELLOW (millis() estimate is usable up to 24h)
  bool timeOk = (getTimeDegradation() != TimeDegradation::RED);
  bool memoryOk = SystemMonitor::isHealthy();
  bool sensorOk = sensorsEverReported_ ? (poolSensorOk_ && solarSensorOk_) : true;  // Both probes must be healthy

  // Count active failures (memory failure = CRITICAL immediately).
  // Time loss without WiFi is a consequence, not an independent failure —
  // counting both would escalate a plain WiFi outage to CRITICAL.
  uint8_t failureCount = 0;
  if (!wifiOk)
    failureCount++;
  if (!timeOk && wifiOk)
    failureCount++;
  if (!sensorOk)
    failureCount++;

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
  if (!sensorOk)
    return DegradationLevel::NO_SENSOR;
  if (!timeOk)
    return DegradationLevel::NO_TIME;
  if (!wifiOk)
    return DegradationLevel::NO_WIFI;

  return DegradationLevel::NORMAL;
}

}  // namespace PoolController
