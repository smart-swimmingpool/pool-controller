/**
 * @file test_sensor_slots.cpp
 * @brief Unit tests for SensorSlots — lock-free temperature slots.
 */

#include <stdio.h>
#include <string.h>
#include <atomic>
#include <cmath>
#include <thread>

#include "SensorSlots.hpp"

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

#define ASSERT_NEAR(a, b, eps)                                                                             \
  do {                                                                                                     \
    float _a = (a);                                                                                        \
    float _b = (b);                                                                                        \
    if (fabs(_a - _b) > (eps)) {                                                                           \
      char _msg[256];                                                                                      \
      snprintf(_msg, sizeof(_msg), "Expected |%s - %s| < %f: got %f vs %f", #a, #b, (float)(eps), _a, _b); \
      test_fail(__FILE__, __LINE__, _msg);                                                                 \
      return 1;                                                                                            \
    }                                                                                                      \
    test_pass(__FILE__, __LINE__);                                                                         \
  } while (0)

int run_sensor_slots_tests() {
  int passed = 0, failed = 0;

  // ── Test: defaults are NaN and not found ──
  {
    test_begin("SensorSlots", "defaults are NaN and not found");

    SensorSlots::reset();
    ASSERT_TRUE(std::isnan(SensorSlots::read(SensorId::SOLAR)));
    ASSERT_FALSE(SensorSlots::isFound(SensorId::SOLAR));

    test_suite_end("SensorSlots::defaults", 1, 0);
    passed++;
  }

  // ── Test: write/read round-trip ──
  {
    test_begin("SensorSlots", "write/read round-trip");

    SensorSlots::reset();
    SensorSlots::write(SensorId::POOL, 26.5f, true);
    ASSERT_TRUE(SensorSlots::isFound(SensorId::POOL));
    ASSERT_NEAR(SensorSlots::read(SensorId::POOL), 26.5f, 0.01f);

    test_suite_end("SensorSlots::roundtrip", 1, 0);
    passed++;
  }

  // ── Test: write NaN marks not found ──
  {
    test_begin("SensorSlots", "write NaN marks not found");

    SensorSlots::reset();
    SensorSlots::write(SensorId::SOLAR, NAN, false);
    ASSERT_FALSE(SensorSlots::isFound(SensorId::SOLAR));
    ASSERT_TRUE(std::isnan(SensorSlots::read(SensorId::SOLAR)));

    test_suite_end("SensorSlots::nan", 1, 0);
    passed++;
  }

  // ── Test: slots are independent ──
  {
    test_begin("SensorSlots", "slots are independent");

    SensorSlots::reset();
    SensorSlots::write(SensorId::SOLAR, 30.0f, true);
    SensorSlots::write(SensorId::CONTROLLER, 41.2f, true);
    ASSERT_NEAR(SensorSlots::read(SensorId::SOLAR), 30.0f, 0.01f);
    ASSERT_NEAR(SensorSlots::read(SensorId::CONTROLLER), 41.2f, 0.01f);
    ASSERT_FALSE(SensorSlots::isFound(SensorId::POOL));

    test_suite_end("SensorSlots::independent", 1, 0);
    passed++;
  }

  // ── Test: concurrent reader never sees a mixed value/found pair (review #170) ──
  {
    test_begin("SensorSlots", "snapshot is consistent under concurrent writes");

    SensorSlots::reset();
    std::atomic<bool> stop{false};
    std::thread writer([&stop] {
      bool valid = true;
      while (!stop.load()) {
        if (valid) {
          SensorSlots::write(SensorId::POOL, 25.0f, true);
        } else {
          SensorSlots::write(SensorId::POOL, NAN, false);
        }
        valid = !valid;
      }
    });
    int mixed = 0;
    for (int i = 0; i < 200000; i++) {
      SensorSlots::Reading r = SensorSlots::snapshot(SensorId::POOL);
      if (r.found == std::isnan(r.value)) {
        mixed++;
      }
    }
    stop = true;
    writer.join();
    ASSERT_EQ(mixed, 0);

    test_suite_end("SensorSlots::snapshot_consistent", 1, 0);
    passed++;
  }

  (void)failed;
  return 0;
}
