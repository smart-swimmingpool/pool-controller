// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_controller_command_queue.cpp
 * @brief Tests for the bounded typed controller command queue.
 */

#include "ControllerCommandQueue.hpp"

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

using PoolController::CommandSource;
using PoolController::ControllerCommand;
using PoolController::ControllerCommandQueue;
using PoolController::ControllerCommandType;
using PoolController::OperationMode;

static int test_fifo_order() {
  test_begin("ControllerCommandQueue", "commands are returned in FIFO order");
  ControllerCommandQueue queue;

  ControllerCommand first{};
  first.type = ControllerCommandType::SET_MODE;
  first.source = CommandSource::MQTT;
  first.mode = OperationMode::BOOST;

  ControllerCommand second{};
  second.type = ControllerCommandType::SET_POOL_MAX_TEMPERATURE;
  second.source = CommandSource::WEB;
  second.value = 29.5F;

  ASSERT_EQ(queue.push(first), ControllerCommandQueue::PushResult::OK);
  ASSERT_EQ(queue.push(second), ControllerCommandQueue::PushResult::OK);
  ASSERT_EQ(queue.size(), 2U);

  ControllerCommand out{};
  ASSERT_TRUE(queue.pop(out));
  ASSERT_EQ(out.type, ControllerCommandType::SET_MODE);
  ASSERT_EQ(out.source, CommandSource::MQTT);
  ASSERT_EQ(out.mode, OperationMode::BOOST);

  ASSERT_TRUE(queue.pop(out));
  ASSERT_EQ(out.type, ControllerCommandType::SET_POOL_MAX_TEMPERATURE);
  ASSERT_EQ(out.source, CommandSource::WEB);
  ASSERT_EQ(out.value, 29.5F);
  ASSERT_TRUE(!queue.pop(out));
  return 0;
}

static int test_full_queue_and_wraparound() {
  test_begin("ControllerCommandQueue", "full queue rejects and reuses released slots");
  ControllerCommandQueue queue;
  ControllerCommand command{};
  command.type = ControllerCommandType::SET_MODE;

  for (std::size_t i = 0; i < ControllerCommandQueue::kCapacity; ++i) {
    command.value = static_cast<float>(i);
    ASSERT_EQ(queue.push(command), ControllerCommandQueue::PushResult::OK);
  }
  ASSERT_EQ(queue.push(command), ControllerCommandQueue::PushResult::FULL);

  ControllerCommand out{};
  ASSERT_TRUE(queue.pop(out));
  command.value = 99.0F;
  ASSERT_EQ(queue.push(command), ControllerCommandQueue::PushResult::OK);

  std::size_t drained = 0;
  while (queue.pop(out)) {
    ++drained;
  }
  ASSERT_EQ(drained, ControllerCommandQueue::kCapacity);
  return 0;
}

int run_controller_command_queue_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_fifo_order, test_full_queue_and_wraparound};
  for (auto test : tests) {
    if (test() == 0) {
      ++passed;
    } else {
      ++failed;
    }
  }
  test_suite_end("ControllerCommandQueue", passed, failed);
  return failed;
}
