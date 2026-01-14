#pragma once

/**
 * Watchdog and Memory Monitor for 24/7 Operation
 * 
 * Monitors memory usage and automatically reboots if memory gets critically low.
 * Provides watchdog functionality to detect system hangs.
 */

#include <Arduino.h>

#ifdef ESP32
  #include <esp_task_wdt.h>
#elif defined(ESP8266)
  #include <Esp.h>
#endif

namespace PoolController {

/**
 * Memory and Watchdog Monitor
 */
class SystemMonitor {
private:
    static constexpr uint32_t LOW_MEMORY_THRESHOLD = 8192;  // 8KB threshold for ESP8266
    static constexpr uint32_t CRITICAL_MEMORY_THRESHOLD = 4096;  // 4KB critical
    static constexpr uint32_t ESP32_LOW_MEMORY_THRESHOLD = 16384;  // 16KB for ESP32
    static constexpr uint32_t ESP32_CRITICAL_MEMORY_THRESHOLD = 8192;  // 8KB critical
    
    static unsigned long lastMemoryCheck;
    static uint32_t minFreeHeap;
    static bool lowMemoryWarning;

public:
    /**
     * Initialize system monitor and watchdog
     */
    static void begin() {
        lastMemoryCheck = 0;
        minFreeHeap = ESP.getFreeHeap();
        lowMemoryWarning = false;

#ifdef ESP32
        // Enable ESP32 Task Watchdog Timer (TWDT)
        // Default timeout is 5 seconds
        esp_task_wdt_init(30, true);  // 30 second timeout, panic on timeout
        esp_task_wdt_add(NULL);  // Add current thread to WDT watch
#elif defined(ESP8266)
        // ESP8266 has software watchdog, just need to call yield() regularly
        // No explicit initialization needed
#endif
    }

    /**
     * Feed the watchdog - call this regularly in main loop
     */
    static void feedWatchdog() {
#ifdef ESP32
        esp_task_wdt_reset();
#elif defined(ESP8266)
        yield();  // ESP8266 software watchdog
#endif
    }

    /**
     * Check memory status and reboot if critically low
     * Call this periodically (e.g., every 10 seconds)
     */
    static void checkMemory() {
        unsigned long now = millis();
        
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

#ifdef ESP32
        uint32_t lowThreshold = ESP32_LOW_MEMORY_THRESHOLD;
        uint32_t criticalThreshold = ESP32_CRITICAL_MEMORY_THRESHOLD;
#else
        uint32_t lowThreshold = LOW_MEMORY_THRESHOLD;
        uint32_t criticalThreshold = CRITICAL_MEMORY_THRESHOLD;
#endif

        // Critical memory - reboot immediately
        if (freeHeap < criticalThreshold) {
            Serial.printf("CRITICAL: Free heap %d bytes < %d bytes. Rebooting...\n", 
                         freeHeap, criticalThreshold);
            Serial.flush();
            delay(1000);
            ESP.restart();
        }
        
        // Low memory - log warning
        if (freeHeap < lowThreshold && !lowMemoryWarning) {
            Serial.printf("WARNING: Low memory detected. Free heap: %d bytes (min: %d)\n", 
                         freeHeap, minFreeHeap);
            lowMemoryWarning = true;
        } else if (freeHeap >= lowThreshold && lowMemoryWarning) {
            // Memory recovered
            lowMemoryWarning = false;
        }
    }

    /**
     * Get current free heap
     */
    static uint32_t getFreeHeap() {
        return ESP.getFreeHeap();
    }

    /**
     * Get minimum free heap since boot
     */
    static uint32_t getMinFreeHeap() {
        return minFreeHeap;
    }

    /**
     * Get heap fragmentation (ESP8266 only)
     */
    static uint8_t getHeapFragmentation() {
#ifdef ESP8266
        return ESP.getHeapFragmentation();
#else
        return 0;  // Not available on ESP32
#endif
    }

    /**
     * Force a reboot
     */
    static void reboot() {
        Serial.println("System reboot requested");
        Serial.flush();
        delay(1000);
        ESP.restart();
    }

    /**
     * Get uptime in seconds
     */
    static uint32_t getUptimeSeconds() {
        return millis() / 1000;
    }

    /**
     * Check if system is healthy
     */
    static bool isHealthy() {
        uint32_t freeHeap = ESP.getFreeHeap();
#ifdef ESP32
        return freeHeap >= ESP32_LOW_MEMORY_THRESHOLD;
#else
        return freeHeap >= LOW_MEMORY_THRESHOLD;
#endif
    }
};

} // namespace PoolController
