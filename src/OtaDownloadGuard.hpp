// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file OtaDownloadGuard.hpp
 * @brief Stall and overall timeout detection for the OTA download loop.
 *
 * The download loop feeds the task watchdog while it waits for data, so the
 * watchdog cannot detect a TCP connection that stalls without being closed.
 * This guard aborts such a download after a period without data or after an
 * overall time limit. Header-only and hardware-independent for native tests;
 * unsigned arithmetic keeps it correct across millis() wrap-around.
 */

#pragma once

#include <cstdint>

namespace PoolController {

class OtaDownloadGuard {
public:
  enum class Status : uint8_t {
    OK,         ///< Download may continue
    STALLED,    ///< No data received for longer than the stall timeout
    TIMED_OUT,  ///< Overall download time exceeded
  };

  OtaDownloadGuard(uint32_t startMs, uint32_t stallTimeoutMs, uint32_t totalTimeoutMs)
      : startMs_(startMs), lastDataMs_(startMs), stallTimeoutMs_(stallTimeoutMs), totalTimeoutMs_(totalTimeoutMs) {}

  /// Call whenever bytes were received.
  void onData(uint32_t nowMs) { lastDataMs_ = nowMs; }

  /// Check whether the download has to be aborted.
  Status check(uint32_t nowMs) const {
    if ((nowMs - startMs_) >= totalTimeoutMs_) {
      return Status::TIMED_OUT;
    }
    if ((nowMs - lastDataMs_) >= stallTimeoutMs_) {
      return Status::STALLED;
    }
    return Status::OK;
  }

private:
  uint32_t startMs_;
  uint32_t lastDataMs_;
  uint32_t stallTimeoutMs_;
  uint32_t totalTimeoutMs_;
};

}  // namespace PoolController
