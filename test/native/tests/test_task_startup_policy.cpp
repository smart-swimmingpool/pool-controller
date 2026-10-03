// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_task_startup_policy.cpp
 * @brief Tests for decideTaskStartupAction() — a Core-0 task that cannot be
 *        created must lead to a defined action, never to silent operation
 *        without fresh sensor data (review #170).
 */

#include <cstdio>

#include "TaskStartupPolicy.hpp"

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

using PoolController::decideTaskStartupAction;
using PoolController::TaskStartupAction;

static int test_all_started() {
  test_begin("TaskStartupPolicy", "all tasks started → continue");
  ASSERT_EQ(decideTaskStartupAction(true, true, true), TaskStartupAction::CONTINUE);
  return 0;
}

static int test_sensor_task_missing() {
  test_begin("TaskStartupPolicy", "missing SensorTask → restart");
  ASSERT_EQ(decideTaskStartupAction(false, true, true), TaskStartupAction::RESTART);
  ASSERT_EQ(decideTaskStartupAction(false, true, false), TaskStartupAction::RESTART);
  return 0;
}

static int test_publish_task_missing() {
  test_begin("TaskStartupPolicy", "missing PublishTask → restart");
  ASSERT_EQ(decideTaskStartupAction(true, false, true), TaskStartupAction::RESTART);
  ASSERT_EQ(decideTaskStartupAction(false, false, false), TaskStartupAction::RESTART);
  return 0;
}

static int test_display_task_missing() {
  test_begin("TaskStartupPolicy", "missing DisplayTask → continue degraded");
  ASSERT_EQ(decideTaskStartupAction(true, true, false), TaskStartupAction::CONTINUE_DEGRADED);
  return 0;
}

int run_task_startup_policy_tests() {
  int passed = 0;
  int failed = 0;
  int (*tests[])() = {test_all_started, test_sensor_task_missing, test_publish_task_missing, test_display_task_missing};
  for (auto test : tests) {
    if (test() == 0) {
      passed++;
    } else {
      failed++;
    }
  }
  test_suite_end("TaskStartupPolicy", passed, failed);
  return failed;
}
