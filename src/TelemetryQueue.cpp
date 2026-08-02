// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file TelemetryQueue.cpp
 * @brief SPSC publish-request ring buffer implementation.
 */

#include "TelemetryQueue.hpp"

namespace PoolController {

bool TelemetryQueue::enqueue(PublishRequestKind kind) {
  const size_t tail = tail_.load(std::memory_order_relaxed);
  const size_t next = (tail + 1) % (CAPACITY + 1);
  if (next == head_.load(std::memory_order_acquire)) {
    return false;  // full
  }
  items_[tail] = kind;
  tail_.store(next, std::memory_order_release);
  return true;
}

bool TelemetryQueue::dequeue(PublishRequestKind &kind) {
  const size_t head = head_.load(std::memory_order_relaxed);
  if (head == tail_.load(std::memory_order_acquire)) {
    return false;  // empty
  }
  kind = items_[head];
  head_.store((head + 1) % (CAPACITY + 1), std::memory_order_release);
  return true;
}

size_t TelemetryQueue::count() const {
  const size_t head = head_.load(std::memory_order_acquire);
  const size_t tail = tail_.load(std::memory_order_acquire);
  return (tail + CAPACITY + 1 - head) % (CAPACITY + 1);
}

void TelemetryQueue::reset() {
  head_.store(0, std::memory_order_relaxed);
  tail_.store(0, std::memory_order_relaxed);
}

}  // namespace PoolController
