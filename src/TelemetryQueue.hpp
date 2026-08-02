// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file TelemetryQueue.hpp
 * @brief Lock-free single-producer/single-consumer queue for MQTT publish requests.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace PoolController {

/** @brief Kinds of publish requests the control loop can enqueue. */
enum class PublishRequestKind : uint8_t {
  STATES = 0,    ///< Publish current telemetry states
  DISCOVERY = 1, ///< Publish Home Assistant discovery configs
};

/**
 * @brief SPSC (single-producer, single-consumer) ring buffer of publish requests.
 *
 * Non-blocking: enqueue on a full queue drops the request and returns false
 * (the periodic publish cadence simply skips a beat — safe by design).
 * Uses a classic atomic head/tail lock-free ring; safe with one writer
 * (control loop) and one reader (PublishTask).
 */
class TelemetryQueue {
public:
  static constexpr size_t CAPACITY = 8;  ///< Fixed slots — no dynamic allocation

  /** @brief Construct an empty queue. */
  TelemetryQueue() { reset(); }

  /**
   * @brief Process-wide singleton used by the control loop and PublishTask.
   * @note Static local is inline (C++17) — one instance across translation units.
   */
  static TelemetryQueue &instance() {
    static TelemetryQueue queue;
    return queue;
  }

  /** @brief Producer side: enqueue a publish request. @return false if full (dropped). */
  bool enqueue(PublishRequestKind kind);

  /** @brief Consumer side: dequeue a publish request. @return false if empty. */
  bool dequeue(PublishRequestKind &kind);

  /** @brief Number of requests currently queued. */
  size_t count() const;

  /** @brief Empty the queue (tests only — must not run while tasks are active). */
  void reset();

private:
  std::atomic<size_t> head_{0};           ///< Consumer index (only consumer writes)
  std::atomic<size_t> tail_{0};           ///< Producer index (only producer writes)
  PublishRequestKind items_[CAPACITY];    ///< Fixed ring storage
};

}  // namespace PoolController
