// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Utils.hpp
 * @brief Utility functions: measurement timing, float/int-to-string conversion.
 */

#pragma once

#include <cstdio>

// Provide dtostrf for native builds (normally an AVR function from avr-libc)
inline char* dtostrf(double val, int width, int precision, char* buf) {
  snprintf(buf, sizeof(buf), "%*.*f", width, precision, val);
  return buf;
}

// ---------------------------------------------------------------------------
// Timing helpers — keeps measurement timestamps in a uniform place
// ---------------------------------------------------------------------------

/**
 * @brief Number of microseconds since the last "mark" event.
 *
 * In normal code this uses micros() (ESP32).  In tests it's stubbed.
 */
inline unsigned long markMicros() {
  extern unsigned long fakeMicros;
  return fakeMicros;
}

/**
 * @brief Record the current microsecond timestamp and return it.
 */
inline unsigned long markTime() {
  unsigned long now = markMicros();
  return now;
}

/**
 * @brief Return elapsed microseconds since @p last, handling wrap-around.
 */
inline unsigned long elapsedMicros(unsigned long last) {
  unsigned long now = markMicros();
  if (now >= last) return now - last;
  return (0xFFFFFFFF - last + 1) + now;
}

/**
 * @brief Return elapsed milliseconds since @p last.
 *
 * Converts from micros() to avoid an extra millis() call.
 */
inline unsigned long elapsedMillisFromMark(unsigned long last) {
  return elapsedMicros(last) / 1000;
}

// ---------------------------------------------------------------------------
// Conversion helpers
// ---------------------------------------------------------------------------

/**
 * @brief Convert an unsigned long to a C-string (base 10).
 *
 * Uses snprintf internally.  The output buffer must be at least 11 bytes
 * (10 digits + NUL, enough for uint32_t).
 *
 * @param[in]  val   Value to convert.
 * @param[out] buf   Output buffer (size >= 11).
 * @return Pointer to @p buf.
 */
inline char* ultostr(unsigned long val, char* buf) {
  snprintf(buf, sizeof(buf), "%lu", val);
  return buf;
}

/**
 * @brief Convert a float to a C-string with @p precision decimal places.
 *
 * Thin wrapper around dtostrf().
 *
 * @param[in]  val       Value to convert.
 * @param[in]  precision Number of decimal places (0-9).
 * @param[out] buf       Output buffer (must be large enough for the result).
 * @return Pointer to @p buf.
 */
inline char* ftostr(float val, int precision, char* buf) {
  return dtostrf(val, 1, precision, buf);
}
