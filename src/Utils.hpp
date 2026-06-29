// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file Utils.hpp
 * @brief Utility functions: measurement timing, float/int-to-string conversion.
 */

#pragma once

#include <cstdio>
#include <cstdint>

// Provide dtostrf for native builds (normally an AVR function from avr-libc)
inline char *dtostrf(double val, int width, int precision, char *buf) {
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

/**
 * Safely concatenate strings with reserved capacity to minimize heap fragmentation.
 * This function helps reduce heap fragmentation by reserving capacity upfront.
 *
 * @param result The String to append to (will reserve capacity if needed)
 * @param toAppend The string to append
 * @param reserveExtra Extra capacity to reserve beyond current needs
 * @note Uses reserve() directly since capacity() is protected in Arduino String class
 */
inline void safeStringConcat(String &result, const String &toAppend, size_t reserveExtra = 32) {
  // Reserve additional capacity to minimize reallocations
  // We can't check current capacity as it's protected, so we always reserve
  // the total needed size to ensure we don't fragment the heap
  result.reserve(result.length() + toAppend.length() + reserveExtra);
  result += toAppend;
}

/**
 * Create a String with reserved capacity to minimize heap fragmentation.
 *
 * @param initialValue Initial string value
 * @param reserveSize Capacity to reserve
 * @return String with reserved capacity
 */
inline String createReservedString(const char *initialValue, size_t reserveSize) {
  String result(initialValue);
  result.reserve(reserveSize);
  return result;
}

}  // namespace Utils
