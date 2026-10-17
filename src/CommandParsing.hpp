// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file CommandParsing.hpp
 * @brief Strict text parsers shared by the MQTT and Web command adapters.
 *
 * Each parser accepts the whole input or nothing: "abc" or "28abc" must not
 * silently become 0 or 28 and then pass range validation.
 */

#pragma once

#include <array>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace PoolController {

namespace CommandParsing {

inline bool onlyWhitespaceFollows(const char *end) {
  while (std::isspace(static_cast<unsigned char>(*end))) {
    ++end;
  }
  return *end == '\0';
}

/** @brief Parse a finite float; surrounding whitespace is allowed. */
inline bool parseFloatStrict(const char *value, float &out) {
  char *end = nullptr;
  const float parsed = std::strtof(value, &end);
  if (end == value || !onlyWhitespaceFollows(end) || !std::isfinite(parsed)) {
    return false;
  }
  out = parsed;
  return true;
}

/** @brief Parse a base-10 int32; surrounding whitespace is allowed. */
inline bool parseIntStrict(const char *value, std::int32_t &out) {
  char *end = nullptr;
  const long long parsed = std::strtoll(value, &end, 10);
  if (end == value || !onlyWhitespaceFollows(end) || parsed < INT32_MIN || parsed > INT32_MAX) {
    return false;
  }
  out = static_cast<std::int32_t>(parsed);
  return true;
}

inline int hexDigitValue(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return 10 + c - 'a';
  }
  if (c >= 'A' && c <= 'F') {
    return 10 + c - 'A';
  }
  return -1;
}

/** @brief Parse a DS18B20 ROM address written as exactly 16 hex digits. */
inline bool parseSensorAddress(const char *value, std::size_t length, std::array<std::uint8_t, 8> &address) {
  if (length != address.size() * 2) {
    return false;
  }
  for (std::size_t i = 0; i < address.size(); ++i) {
    const int high = hexDigitValue(value[i * 2]);
    const int low = hexDigitValue(value[i * 2 + 1]);
    if (high < 0 || low < 0) {
      return false;
    }
    address[i] = static_cast<std::uint8_t>((high << 4) | low);
  }
  return true;
}

}  // namespace CommandParsing

}  // namespace PoolController
