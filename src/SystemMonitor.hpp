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
   */
  static void begin() {
    lastMemoryCheck = 0;
    minFreeHeap = ESP.getFreeHeap();
    lowMemoryWarning = false;

    esp_task_wdt_init(30, true);
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

  /** Minimum uptime (seconds) for a boot to count as "healthy" */
  static constexpr uint32_t BOOT_LOOP_MIN_UPTIME = 300;       // 5 min

  /** Number of consecutive short boots before safe mode activates */
  static constexpr uint8_t BOOT_LOOP_MAX_COUNT = 3;

  /**
   * Detect boot-loop pattern.
   * Call this as early as possible in setup(), before Homie initializes.
   *
   * Uses Preferences (NVS) to persist boot count and last uptime.
   * Returns true if a boot-loop pattern is detected.
   */
  static bool detectBootLoop() {
    Preferences prefs;
    prefs.begin("sysmon", false);

    int bootCount = prefs.getInt("bootCount", 0);
    uint32_t lastUptime = prefs.getUInt("lastUptime", 0);

    // Current uptime is 0 at boot
    uint32_t curUptime = millis() / 1000;

    Serial.printf("  Boot counter: %d, last uptime: %us\n",
                  bootCount + 1, lastUptime);

    bool isBootLoop = false;

    if (lastUptime > 0 && lastUptime < BOOT_LOOP_MIN_UPTIME) {
      // Short boot — potential boot-loop
      if (bootCount >= BOOT_LOOP_MAX_COUNT) {
        // Boot-loop pattern detected: multiple consecutive short boots
        isBootLoop = true;
        Serial.printf("✖ BOOT-LOOP DETECTED (%d boots < %us)\n",
                      bootCount + 1, BOOT_LOOP_MIN_UPTIME);
        Serial.println("  Entering safe mode — all relays OFF");
      }
    } else {
      // Previous boot was long enough → reset boot counter
      bootCount = 0;
    }

    // Persist for next boot
    prefs.putInt("bootCount", bootCount + 1);
    prefs.putUInt("lastUptime", curUptime);
    prefs.end();

    return isBootLoop;
  }
};

}  // namespace PoolController
