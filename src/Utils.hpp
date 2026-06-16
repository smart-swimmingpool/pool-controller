// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Utils.hpp
 * @brief Utility functions: measurement timing, float/int-to-string conversion.
 */

#pragma once

#include <cstdio>
#include <cstdint>

// Provide dtostrf for native builds (normally an AVR function from avr-libc)
inline char* dtostrf(double val, int width, int precision, char* buf) {
  // NOLINT: sizeof(buf) in a char* parameter gives pointer size (4 or 8),
  // not the actual buffer. Use 8 as a safe minimum — floatToString() callers
  // ensure bufferSize >= 8 before calling dtostrf, so writing up to 7 chars
  // plus null terminator is always safe and never truncates typical values.
  snprintf(buf, 8, "%*.*f", width, precision, val);  // NOLINT(runtime/printf)
  return buf;
}

namespace Utils {

/**
 * Check if enough time has elapsed since last measurement
 * Handles millis() overflow correctly
 *
 * @param lastMeasurement Last measurement timestamp (milliseconds)
 * @param intervalSeconds The interval in seconds
 * @return true if enough time has elapsed
 */
inline bool shouldMeasure(uint32_t lastMeasurement, uint32_t intervalSeconds) {
  if (lastMeasurement == 0) {
    return true;  // First measurement
  }
  uint32_t currentMillis = millis();
  uint32_t intervalMillis = intervalSeconds * 1000UL;

  // This handles overflow correctly
  return (currentMillis - lastMeasurement) >= intervalMillis;
}

/**
 * Convert float to string buffer with minimal heap usage
 *
 * @param value The float value to convert
 * @param buffer The buffer to write to (min 16 bytes recommended)
 * @param bufferSize Size of the buffer (must be at least 8 bytes)
 * @param decimals Number of decimal places (default: 2)
 * @note For typical temperature values (-50 to 100), 16 bytes is sufficient
 */
inline void floatToString(float value, char *buffer, size_t bufferSize, int decimals = 2) {
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
inline void intToString(int value, char *buffer, size_t bufferSize) {
  snprintf(buffer, bufferSize, "%d", value);
}

}  // namespace Utils
