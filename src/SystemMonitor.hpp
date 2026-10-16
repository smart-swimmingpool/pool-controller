// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file SystemMonitor.hpp
 * @brief Watchdog, memory monitor, and min/max tracking for 24/7 operation.
 */

#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>

#include "LogCapture.hpp"

namespace PoolController {

class SystemMonitor {
private:
  static constexpr uint32_t LOW_MEMORY_THRESHOLD = 16384;
  static constexpr uint32_t CRITICAL_MEMORY_THRESHOLD = 8192;

  static uint32_t lastMemoryCheck;
  static uint32_t minFreeHeap;
  static bool lowMemoryWarning;

public:
  static void begin() {
    lastMemoryCheck = 0;
    minFreeHeap = ESP.getFreeHeap();
    lowMemoryWarning = false;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    const esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 30000,
      .idle_core_mask = 0,
      .trigger_panic = true,
    };
    esp_task_wdt_reconfigure(&wdt_config);
#else
    esp_task_wdt_init(30, true);
#endif
    registerCurrentTaskWithWatchdog();
  }

  /** Subscribe the calling FreeRTOS task to the task watchdog. */
  static bool registerCurrentTaskWithWatchdog() {
    const esp_err_t err = esp_task_wdt_add(NULL);
    if (err == ESP_OK) {
      return true;
    }
    LOG_ERROR("Task watchdog registration failed: 0x%x\n", static_cast<unsigned>(err));
    return false;
  }

  static void feedWatchdog() { esp_task_wdt_reset(); }

  /** Feed the watchdog from a task that registered itself successfully. */
  static void feedWatchdogFromTask() { esp_task_wdt_reset(); }

  static void checkMemory() {
    uint32_t now = millis();
    if (now - lastMemoryCheck < 10000) {
      return;
    }
    lastMemoryCheck = now;

    uint32_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < minFreeHeap) {
      minFreeHeap = freeHeap;
    }

    if (freeHeap < CRITICAL_MEMORY_THRESHOLD) {
      LOG_ERROR("CRITICAL: Free heap %d bytes < %d bytes. Rebooting...\n", freeHeap, CRITICAL_MEMORY_THRESHOLD);
      Serial.flush();
      delay(1000);
      ESP.restart();
    }

    if (freeHeap < LOW_MEMORY_THRESHOLD && !lowMemoryWarning) {
      LOG_WARN("WARNING: Low memory detected. Free heap: %d bytes (min: %d)\n", freeHeap, minFreeHeap);
      lowMemoryWarning = true;
    } else if (freeHeap >= LOW_MEMORY_THRESHOLD && lowMemoryWarning) {
      lowMemoryWarning = false;
    }
  }

  static uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
  static uint32_t getMinFreeHeap() { return minFreeHeap; }

  static void reboot() {
    LOG_INFO("System reboot requested\n");
    Serial.flush();
    delay(1000);
    ESP.restart();
  }

  static uint32_t getUptimeSeconds() { return millis() / 1000; }
  static bool isHealthy() { return ESP.getFreeHeap() >= LOW_MEMORY_THRESHOLD; }

  static constexpr uint8_t BOOT_LOOP_MAX_COUNT = 3;
  static constexpr uint32_t BOOT_LOOP_CLEAR_AFTER_SEC = 300;

  static bool detectBootLoop() {
    Preferences prefs;
    prefs.begin("sysmon", false);
    int bootCount = prefs.getInt("bootCount", 0) + 1;
    LOG_INFO("  Boot counter: %d\n", bootCount);

    bool isBootLoop = (bootCount >= BOOT_LOOP_MAX_COUNT);
    if (isBootLoop) {
      LOG_ERROR("✖ BOOT-LOOP DETECTED (%d consecutive boots)\n", bootCount);
      LOG_ERROR("  Entering safe mode — all relays OFF\n");
    }

    prefs.putInt("bootCount", bootCount);
    prefs.end();
    return isBootLoop;
  }

  static void clearBootLoopCounter() {
    Preferences prefs;
    prefs.begin("sysmon", false);
    prefs.putInt("bootCount", 0);
    prefs.end();
  }
};

}  // namespace PoolController
