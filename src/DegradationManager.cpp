// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#include "DegradationManager.hpp"
#include "SystemMonitor.hpp"
#include "TimeClientHelper.hpp"  // TimeDegradation, getTimeDegradation

#include <Homie.h>

namespace PoolController {

// Static member definitions
DegradationLevel DegradationManager::currentLevel_ = DegradationLevel::NORMAL;
DegradationLevel DegradationManager::previousLevel_ = DegradationLevel::NORMAL;
uint8_t DegradationManager::_sensorHealthCounter_ = 0;
bool DegradationManager::forcedSafeMode_ = false;
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
  _sensorHealthCounter_ = 0;  // Pessimistic start — both probes must report healthy
  forcedSafeMode_ = false;
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

void DegradationManager::reportSensorStatus(bool valid) {
  // Each of the two DallasTemperatureNode instances calls this on every
  // loop pass.  We clamp the counter to [0, 2] so that both probes must
  // report healthy before the sensor subsystem is considered OK.
  if (valid && _sensorHealthCounter_ < 2) _sensorHealthCounter_++;
  else if (!valid && _sensorHealthCounter_ > 0) _sensorHealthCounter_--;
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

  // If time is RED (lost) but WiFi is up, try an NTP sync immediately.
  // The Time library only syncs every SYNC_INTERVAL (3600s), so without
  // this the system could stay in NO_TIME for an hour after NTP recovers.
  if (wifiOk && (getTimeDegradation() == TimeDegradation::RED)) {
    forceNtpUpdate();
  }

  // Time is OK for GREEN + YELLOW (millis() estimate is usable up to 24h)
  bool timeOk = (getTimeDegradation() != TimeDegradation::RED);
  bool memoryOk = SystemMonitor::isHealthy();
  bool sensorOk = (_sensorHealthCounter_ >= 2);  // Both Dallas probes must be healthy

  // Count active failures (memory failure = CRITICAL immediately).
  // Time loss without WiFi is a consequence, not an independent failure —
  // counting both would escalate a plain WiFi outage to CRITICAL.
  uint8_t failureCount = 0;
  if (!wifiOk) failureCount++;
  if (!timeOk && wifiOk) failureCount++;
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
