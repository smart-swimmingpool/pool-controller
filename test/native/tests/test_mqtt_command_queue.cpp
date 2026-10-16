// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file test_mqtt_command_queue.cpp
 * @brief Tests for MqttCommandQueue — hand-over of MQTT commands from the
 *        AsyncTCP task to the loop task (#195).
 */

#include <cstdio>
#include <cstring>
#include <string>

#include "MqttCommandQueue.hpp"

extern void test_begin(const char *suite, const char *name);
extern void test_pass(const char *file, int line);
extern void test_fail(const char *file, int line, const char *msg);
extern void test_suite_end(const char *name, int passed, int failed);

#define ASSERT_TRUE(cond)                                     \
  do {                                                        \
    if (!(cond)) {                                            \
      test_fail(__FILE__, __LINE__, "Expected true: " #cond); \
      return 1;                                               \
    }                                                         \
    test_pass(__FILE__, __LINE__);                            \
  } while (0)

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

using PoolController::MqttCommandQueue;
using PushResult = PoolController::MqttCommandQueue::PushResult;

static int test_fifo_order() {
  test_begin("MqttCommandQueue", "messages are returned in FIFO order");
  static MqttCommandQueue queue;
  ASSERT_TRUE(queue.push("a/set", "1", 1) == PushResult::OK);
  ASSERT_TRUE(queue.push("b/set", "22", 2) == PushResult::OK);
  ASSERT_EQ(queue.size(), static_cast<size_t>(2));

  MqttCommandQueue::Message msg;
  ASSERT_TRUE(queue.pop(msg));
  ASSERT_EQ(strcmp(msg.topic, "a/set"), 0);
  ASSERT_TRUE(strcmp(msg.payload, "1") == 0 && msg.payloadLen == 1);
  ASSERT_TRUE(queue.pop(msg));
  ASSERT_EQ(strcmp(msg.topic, "b/set"), 0);
  ASSERT_TRUE(strcmp(msg.payload, "22") == 0 && msg.payloadLen == 2);
  ASSERT_TRUE(!queue.pop(msg));
  return 0;
}

static int test_payload_without_terminator() {
  test_begin("MqttCommandQueue", "payload is copied by length and NUL-terminated");
  static MqttCommandQueue queue;
  const char raw[] = {'O', 'N', 'X', 'X'};  // only the first two bytes belong to the payload
  ASSERT_TRUE(queue.push("pump/set", raw, 2) == PushResult::OK);
  MqttCommandQueue::Message msg;
  ASSERT_TRUE(queue.pop(msg));
  ASSERT_TRUE(strcmp(msg.payload, "ON") == 0 && msg.payloadLen == 2);
  return 0;
}

static int test_empty_payload() {
  test_begin("MqttCommandQueue", "empty payload is allowed");
  static MqttCommandQueue queue;
  ASSERT_TRUE(queue.push("x/set", "", 0) == PushResult::OK);
  MqttCommandQueue::Message msg;
  ASSERT_TRUE(queue.pop(msg));
  ASSERT_TRUE(msg.payloadLen == 0 && msg.payload[0] == '\0');
  return 0;
}

static int test_full_queue_drops() {
  test_begin("MqttCommandQueue", "full queue rejects further messages");
  static MqttCommandQueue queue;
  for (size_t i = 0; i < MqttCommandQueue::kCapacity; i++) {
    ASSERT_TRUE(queue.push("t", "p", 1) == PushResult::OK);
  }
  ASSERT_TRUE(queue.push("t", "p", 1) == PushResult::FULL);
  MqttCommandQueue::Message msg;
  ASSERT_TRUE(queue.pop(msg));
  ASSERT_TRUE(queue.push("t", "p", 1) == PushResult::OK);  // space again (wrap-around)
  size_t n = 0;
  while (queue.pop(msg)) {
    n++;
  }
  ASSERT_TRUE(n == MqttCommandQueue::kCapacity);
  return 0;
}

static int test_too_large_dropped() {
  test_begin("MqttCommandQueue", "oversized topic or payload is rejected");
  static MqttCommandQueue queue;
  std::string longTopic(MqttCommandQueue::kTopicSize, 't');
  std::string longPayload(MqttCommandQueue::kPayloadSize, 'p');
  ASSERT_TRUE(queue.push(longTopic.c_str(), "1", 1) == PushResult::TOO_LARGE);
  ASSERT_TRUE(queue.push("t", longPayload.c_str(), longPayload.size()) == PushResult::TOO_LARGE);
  std::string maxPayload(MqttCommandQueue::kPayloadSize - 1, 'p');
  ASSERT_TRUE(queue.push("t", maxPayload.c_str(), maxPayload.size()) == PushResult::OK);
  ASSERT_EQ(queue.size(), static_cast<size_t>(1));
  return 0;
}

int run_mqtt_command_queue_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {
    test_fifo_order, test_payload_without_terminator, test_empty_payload, test_full_queue_drops, test_too_large_dropped};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("MqttCommandQueue", passed, failed);
  return failed;
}
