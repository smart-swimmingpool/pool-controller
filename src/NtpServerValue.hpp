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

  /** @brief Check raw queued values before treating them as C strings. */
  bool valid() const noexcept { return text[0] != '\0' && std::memchr(text.data(), '\0', text.size()) != nullptr; }
};

}  // namespace PoolController
