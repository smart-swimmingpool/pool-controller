// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file MqttCommandQueue.hpp
 * @brief Fixed-size, thread-safe hand-over of MQTT commands from the
 *        AsyncTCP task to the loop task.
 *
 * AsyncMqttClient delivers messages on the AsyncTCP task. Handling them there
 * changed controller state (operation mode String, settings, relays, NVS)
 * concurrently with the loop task. The MQTT callback therefore only copies
 * topic and payload into this queue; the loop task processes them.
 *
 * No heap allocation: messages are copied into a static ring buffer.
 * On ESP32 the buffer is guarded by a portMUX critical section; native tests
 * use std::mutex.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#else
#include <mutex>
#endif

namespace PoolController {

class MqttCommandQueue {
public:
  static constexpr size_t kCapacity = 8;
  static constexpr size_t kTopicSize = 128;    ///< incl. terminating NUL
  static constexpr size_t kPayloadSize = 128;  ///< incl. terminating NUL

  struct Message {
    char topic[kTopicSize];
    char payload[kPayloadSize];
    size_t payloadLen;
  };

  enum class PushResult : uint8_t {
    OK,
    FULL,       ///< Queue full — message dropped
    TOO_LARGE,  ///< Topic or payload does not fit — message dropped
  };

  /// Copy a message into the queue (safe to call from any task).
  PushResult push(const char *topic, const char *payload, size_t payloadLen) {
    size_t topicLen = strnlen(topic, kTopicSize);
    if (topicLen >= kTopicSize || payloadLen >= kPayloadSize) {
      return PushResult::TOO_LARGE;
    }
    lock();
    if (count_ == kCapacity) {
      unlock();
      return PushResult::FULL;
    }
    Message &slot = buffer_[(head_ + count_) % kCapacity];
    memcpy(slot.topic, topic, topicLen);
    slot.topic[topicLen] = '\0';
    if (payloadLen > 0) {
      memcpy(slot.payload, payload, payloadLen);
    }
    slot.payload[payloadLen] = '\0';
    slot.payloadLen = payloadLen;
    count_++;
    unlock();
    return PushResult::OK;
  }

  /// Take the oldest message out of the queue. Returns false if empty.
  bool pop(Message &out) {
    lock();
    if (count_ == 0) {
      unlock();
      return false;
    }
    out = buffer_[head_];
    head_ = (head_ + 1) % kCapacity;
    count_--;
    unlock();
    return true;
  }

  size_t size() {
    lock();
    size_t n = count_;
    unlock();
    return n;
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

  Message buffer_[kCapacity] = {};
  size_t head_ = 0;
  size_t count_ = 0;
};

}  // namespace PoolController
