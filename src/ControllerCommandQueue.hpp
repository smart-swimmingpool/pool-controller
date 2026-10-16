// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerCommandQueue.hpp
 * @brief Bounded hand-over queue for typed controller commands.
 */

#pragma once

#include <cstddef>

#include "ControllerCommand.hpp"

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#else
#include <mutex>
#endif

namespace PoolController {

/**
 * @brief Fixed-capacity queue for commands crossing callback/task boundaries.
 *
 * The queue owns copies of trivially-copyable `ControllerCommand` values and
 * never allocates memory. It is safe for multiple adapter producers and a
 * single Core-1 application consumer.
 */
class ControllerCommandQueue final {
public:
  static constexpr std::size_t kCapacity = 16;

  enum class PushResult : std::uint8_t {
    OK,
    FULL,
  };

  PushResult push(const ControllerCommand &command) {
    lock();
    if (count_ == kCapacity) {
      unlock();
      return PushResult::FULL;
    }
    buffer_[(head_ + count_) % kCapacity] = command;
    ++count_;
    unlock();
    return PushResult::OK;
  }

  bool pop(ControllerCommand &out) {
    lock();
    if (count_ == 0) {
      unlock();
      return false;
    }
    out = buffer_[head_];
    head_ = (head_ + 1) % kCapacity;
    --count_;
    unlock();
    return true;
  }

  std::size_t size() {
    lock();
    const std::size_t result = count_;
    unlock();
    return result;
  }

private:
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  void lock() { portENTER_CRITICAL(&mux_); }
  void unlock() { portEXIT_CRITICAL(&mux_); }
#else
  std::mutex mutex_;
  void lock() { mutex_.lock(); }
  void unlock() { mutex_.unlock(); }
#endif

  ControllerCommand buffer_[kCapacity]{};
  std::size_t head_{0};
  std::size_t count_{0};
};

}  // namespace PoolController
