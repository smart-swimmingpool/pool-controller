// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <cstddef>
#include <cstring>

namespace PoolController {

/** @brief Owned NTP server text: up to 127 bytes plus a terminating NUL. */
struct NtpServerValue final {
  std::array<char, 128> text{};

  /**
   * @brief Copy a length-delimited server name without truncation or allocation.
   * Empty, oversized and embedded-NUL input is rejected without changing this
   * value. The caller supplies readable storage for length bytes; no source
   * terminator is required. Default construction represents an unset value.
   */
  bool assign(const char *input, std::size_t length) noexcept {
    if (input == nullptr || length == 0 || length >= text.size() || std::memchr(input, '\0', length) != nullptr) {
      return false;
    }
    // Permit assigning a substring of our own buffer; clear the old suffix.
    std::memmove(text.data(), input, length);
    std::memset(text.data() + length, 0, text.size() - length);
    return true;
  }

  /**
   * @brief Check raw queued values before treating them as C strings.
   *
   * Accepts only the canonical layout produced by assign() or by zero-filled
   * default construction: a nonempty prefix of non-NUL bytes, the first NUL
   * terminator, and only zero bytes after it.
   */
  bool valid() const noexcept {
    if (text[0] == '\0') {
      return false;
    }
    const void *terminator = std::memchr(text.data(), '\0', text.size());
    if (terminator == nullptr) {
      return false;
    }
    const char *firstNul = static_cast<const char *>(terminator);
    return allZero(firstNul + 1, static_cast<std::size_t>(text.data() + text.size() - firstNul - 1));
  }

  /** @brief True when every byte in the given range is NUL. */
  static bool allZero(const char *begin, std::size_t length) noexcept {
    for (std::size_t i = 0; i < length; ++i) {
      if (begin[i] != '\0') {
        return false;
      }
    }
    return true;
  }
};

}  // namespace PoolController
