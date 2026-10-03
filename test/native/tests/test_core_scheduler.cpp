/**
 * @file test_core_scheduler.cpp
 * @brief Unit tests for CoreScheduler — task parameter assertions via mock.
 */

#include <stdio.h>
#include <string.h>

#include "CoreScheduler.hpp"

using namespace PoolController;  // NOLINT(build/namespaces)

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

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b)                                                                                          \
  do {                                                                                                           \
    auto _a = (a);                                                                                               \
    auto _b = (b);                                                                                               \
    if (_a != _b) {                                                                                              \
      char _msg[256];                                                                                            \
      snprintf(_msg, sizeof(_msg), "Expected %s == %s: got %lld vs %lld", #a, #b, (long long)_a, (long long)_b); \
      test_fail(__FILE__, __LINE__, _msg);                                                                       \
      return 1;                                                                                                  \
    }                                                                                                            \
    test_pass(__FILE__, __LINE__);                                                                               \
  } while (0)

int run_core_scheduler_tests() {
  int passed = 0, failed = 0;

  // ── Test: sensor task priority matches plan ──
  {
    test_begin("CoreScheduler", "sensor task priority matches plan");

    CoreScheduler::begin();
    ASSERT_EQ(CoreScheduler::sensorPriority, CoreScheduler::TASK_PRIORITY_SENSOR);

    test_suite_end("CoreScheduler::sensor_priority", 1, 0);
    passed++;
  }

  // ── Test: sensor task stack matches plan ──
  {
    test_begin("CoreScheduler", "sensor task stack matches plan");

    CoreScheduler::begin();
    ASSERT_EQ(CoreScheduler::sensorStack, CoreScheduler::TASK_STACK_SENSOR);

    test_suite_end("CoreScheduler::sensor_stack", 1, 0);
    passed++;
  }

  (void)failed;
  return 0;
}
