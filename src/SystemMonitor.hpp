// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

/**
 * Watchdog and Memory Monitor for 24/7 Operation
 *
 * Monitors memory usage and automatically reboots if memory gets critically low.
 * Provides watchdog functionality to detect system hangs.
 *
 * ESP8266 support was removed in v3.2.0.
 */

#include <Arduino.h>
#include <Preferences.h>
#include <esp_idf_version.h>
#include <esp_task_wdt.h>

namespace PoolController {

/**
 * Memory and Watchdog Monitor
 */
class SystemMonitor {
private:
  static constexpr uint32_t LOW_MEMORY_THRESHOLD = 16384;
  static constexpr uint32_t CRITICAL_MEMORY_THRESHOLD = 8192;

  static uint32_t lastMemoryCheck;
  static uint32_t minFreeHeap;
  static bool lowMemoryWarning;

public:
  /**
   * Initialize system monitor and watchdog.
   * ESP32 TWDT: 30-second timeout, panic on timeout.
   *
   * ESP-IDF 5.x changed the WDT API to use a config struct, and the Arduino
   * framework pre-initialises the TWDT before setup() runs.  Use
   * esp_task_wdt_reconfigure() on ESP-IDF ≥ 5 so we can adjust the timeout
   * without failing with ESP_ERR_INVALID_STATE.
   */
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
    esp_task_wdt_add(NULL);
  }

  /**
   * Feed the watchdog — call this regularly in main loop
   */
  static void feedWatchdog() {
    esp_task_wdt_reset();
  }

  /**
   * Check memory status and reboot if critically low.
   * Call this periodically (e.g., every 10 seconds).
   */
  static void checkMemory() {
    uint32_t now = millis();

    // Check every 10 seconds
    if (now - lastMemoryCheck < 10000) {
      return;
    }
    lastMemoryCheck = now;

    uint32_t freeHeap = ESP.getFreeHeap();

    // Track minimum heap
    if (freeHeap < minFreeHeap) {
      minFreeHeap = freeHeap;
    }

    // Critical memory — reboot immediately
    if (freeHeap < CRITICAL_MEMORY_THRESHOLD) {
      Serial.printf("CRITICAL: Free heap %d bytes < %d bytes. Rebooting...\n", freeHeap, CRITICAL_MEMORY_THRESHOLD);
      Serial.flush();
      delay(1000);
      ESP.restart();
    }

    // Low memory — log warning
    if (freeHeap < LOW_MEMORY_THRESHOLD && !lowMemoryWarning) {
      Serial.printf("WARNING: Low memory detected. Free heap: %d bytes "
                    "(min: %d)\n",
        freeHeap, minFreeHeap);
      lowMemoryWarning = true;
    } else if (freeHeap >= LOW_MEMORY_THRESHOLD && lowMemoryWarning) {
      lowMemoryWarning = false;
    }
  }

  /** Get current free heap */
  static uint32_t getFreeHeap() { return ESP.getFreeHeap(); }

  /** Get minimum free heap since boot */
  static uint32_t getMinFreeHeap() { return minFreeHeap; }

  /** Force a reboot */
  static void reboot() {
    Serial.println("System reboot requested");
    Serial.flush();
    delay(1000);
    ESP.restart();
  }

  /** Get uptime in seconds */
  static uint32_t getUptimeSeconds() { return millis() / 1000; }

  /** Check if system is healthy */
  static bool isHealthy() {
    return ESP.getFreeHeap() >= LOW_MEMORY_THRESHOLD;
  }

  // --- Boot-loop detection (P8) ---

  /** Number of consecutive boots before safe mode activates */
  static constexpr uint8_t BOOT_LOOP_MAX_COUNT = 3;

  /** Minimum uptime (seconds) before clearing the boot-loop counter */
  static constexpr uint32_t BOOT_LOOP_CLEAR_AFTER_SEC = 300;  // 5 min

  /**
   * Detect boot-loop pattern.
   * Call this as early as possible in setup(), before Homie initializes.
   *
   * Increments a persistent boot counter in NVS on every boot.
   * Returns true when BOOT_LOOP_MAX_COUNT consecutive boots have occurred
   * without a reset (which happens after stable uptime in loop()).
   *
   * The counter is reset to 0 by clearBootLoopCounter(), called from
   * PoolController::loop() after BOOT_LOOP_CLEAR_AFTER_SEC of stable operation.
   */
  static bool detectBootLoop() {
    Preferences prefs;
    prefs.begin("sysmon", false);

    int bootCount = prefs.getInt("bootCount", 0) + 1;

    Serial.printf("  Boot counter: %d\n", bootCount);

    bool isBootLoop = (bootCount >= BOOT_LOOP_MAX_COUNT);
    if (isBootLoop) {
      Serial.printf("✖ BOOT-LOOP DETECTED (%d consecutive boots)\n", bootCount);
      Serial.println("  Entering safe mode — all relays OFF");
    }

    prefs.putInt("bootCount", bootCount);
    prefs.end();

    return isBootLoop;
  }

  /**
   * Clear the boot-loop counter.
   * Called from PoolController::loop() after BOOT_LOOP_CLEAR_AFTER_SEC seconds
   * of stable operation to indicate a healthy boot.
   */
  static void clearBootLoopCounter() {
    Preferences prefs;
    prefs.begin("sysmon", false);
    prefs.putInt("bootCount", 0);
    prefs.end();
  }
};

}  // namespace PoolController
