// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerSnapshotStore.hpp
 * @brief Consistent publication of immutable controller read models.
 */

#pragma once

#include "ControllerSnapshot.hpp"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#else
#include <mutex>
#endif

namespace PoolController {

/**
 * @brief Small synchronized store for complete `SystemSnapshot` values.
 *
 * The application owner publishes one complete snapshot after updating state.
 * Adapters copy the latest snapshot under a short critical section and never
 * dereference mutable controller nodes directly.
 */
class ControllerSnapshotStore final {
public:
  void publish(const SystemSnapshot &snapshot) {
    lock();
    snapshot_ = snapshot;
    hasSnapshot_ = true;
    unlock();
  }

  bool read(SystemSnapshot &out) const {
    lock();
    if (!hasSnapshot_) {
      unlock();
      return false;
    }
    out = snapshot_;
    unlock();
    return true;
  }

  bool hasSnapshot() const {
    lock();
    const bool result = hasSnapshot_;
    unlock();
    return result;
  }

private:
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  void lock() const { portENTER_CRITICAL(&mux_); }
  void unlock() const { portEXIT_CRITICAL(&mux_); }
#else
  mutable std::mutex mutex_;
  void lock() const { mutex_.lock(); }
  void unlock() const { mutex_.unlock(); }
#endif

  SystemSnapshot snapshot_{};
  bool hasSnapshot_{false};
};

}  // namespace PoolController
