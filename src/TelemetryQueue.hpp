// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file TelemetryQueue.hpp
 * @brief Bounded queue for deferred MQTT publish requests.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace PoolController {

/** @brief Kinds of publish requests the control loop can enqueue. */
enum class PublishRequestKind : uint8_t {
  STATES = 0,     ///< Publish current telemetry states
  DISCOVERY = 1,  ///< Publish Home Assistant discovery configs
};

/**
 * @brief Fixed-capacity ring buffer of publish requests.
 *
 * Non-blocking: enqueue on a full queue drops the request and returns false
 * (the periodic publish cadence simply skips a beat — safe by design).
 * Publishing is serviced on the Core-1 control-loop task so MqttPublisher never
 * reads mutable controller state concurrently from another core.
 */
class TelemetryQueue {
public:
  static constexpr size_t CAPACITY = 8;  ///< Fixed slots — no dynamic allocation

  /** @brief Construct an empty queue. */
  TelemetryQueue() { reset(); }

  /** @brief Process-wide queue for deferred telemetry requests. */
  static TelemetryQueue &instance() {
    static TelemetryQueue queue;
    return queue;
  }

  /** @brief Enqueue a publish request. @return false if full (dropped). */
  bool enqueue(PublishRequestKind kind);

  /** @brief Dequeue a publish request. @return false if empty. */
  bool dequeue(PublishRequestKind &kind);

  /** @brief Number of requests currently queued. */
  size_t count() const;

  /** @brief Empty the queue (tests only — must not run while in use). */
  void reset();

private:
  /// One slot stays free to tell "full" from "empty", so indices run over
  /// [0, CAPACITY] and the storage needs CAPACITY + 1 slots.
  static constexpr size_t SLOTS = CAPACITY + 1;

  std::atomic<size_t> head_{0};
  std::atomic<size_t> tail_{0};
  PublishRequestKind items_[SLOTS] = {};  ///< Fixed ring storage
};

}  // namespace PoolController
