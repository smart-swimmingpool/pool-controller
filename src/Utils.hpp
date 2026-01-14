#pragma once

/**
 * Utility functions for 24/7 operation optimization
 */

namespace PoolController {
namespace Utils {

    /**
     * Check if enough time has elapsed since last measurement
     * Handles millis() overflow correctly
     * 
     * @param lastMeasurement The last measurement timestamp in milliseconds
     * @param intervalSeconds The interval in seconds
     * @return true if enough time has elapsed
     */
    inline bool shouldMeasure(unsigned long lastMeasurement, unsigned long intervalSeconds) {
        if (lastMeasurement == 0) {
            return true;  // First measurement
        }
        unsigned long currentMillis = millis();
        unsigned long intervalMillis = intervalSeconds * 1000UL;
        
        // This handles overflow correctly
        return (currentMillis - lastMeasurement) >= intervalMillis;
    }

    /**
     * Convert float to string buffer with minimal heap usage
     * 
     * @param value The float value to convert
     * @param buffer The buffer to write to (minimum 16 bytes recommended)
     * @param bufferSize Size of the buffer (must be at least 8 bytes)
     * @param decimals Number of decimal places (default: 2)
     * @note For typical temperature values (-50 to 100), 16 bytes is sufficient
     */
    inline void floatToString(float value, char* buffer, size_t bufferSize, int decimals = 2) {
        // dtostrf needs minimum buffer size to avoid overflow
        if (bufferSize < 8) {
            buffer[0] = '\0';
            return;
        }
        dtostrf(value, 0, decimals, buffer);
    }

    /**
     * Convert int to string buffer with minimal heap usage
     * 
     * @param value The int value to convert
     * @param buffer The buffer to write to
     * @param bufferSize Size of the buffer
     */
    inline void intToString(int value, char* buffer, size_t bufferSize) {
        snprintf(buffer, bufferSize, "%d", value);
    }

} // namespace Utils
} // namespace PoolController
