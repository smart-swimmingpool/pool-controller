/**
 * @file test_telemetry_queue.cpp
 * @brief Unit tests for TelemetryQueue — SPSC ring buffer semantics.
 */

#include <stdio.h>
#include <string.h>

#include "TelemetryQueue.hpp"

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

int run_telemetry_queue_tests() {
  int passed = 0, failed = 0;

  // ── Test: empty on fresh construction ──
  {
    test_begin("TelemetryQueue", "empty after reset");

    TelemetryQueue queue;
    ASSERT_EQ(queue.count(), 0u);

    test_suite_end("TelemetryQueue::empty", 1, 0);
    passed++;
  }

  // ── Test: enqueue/dequeue round-trip ──
  {
    test_begin("TelemetryQueue", "enqueue/dequeue round-trip");

    TelemetryQueue queue;
    ASSERT_TRUE(queue.enqueue(PublishRequestKind::STATES));
    ASSERT_EQ(queue.count(), 1u);

    PublishRequestKind kind = PublishRequestKind::DISCOVERY;
    ASSERT_TRUE(queue.dequeue(kind));
    ASSERT_EQ(static_cast<int>(kind), static_cast<int>(PublishRequestKind::STATES));
    ASSERT_EQ(queue.count(), 0u);

    test_suite_end("TelemetryQueue::roundtrip", 1, 0);
    passed++;
  }

  // ── Test: FIFO ordering ──
  {
    test_begin("TelemetryQueue", "FIFO ordering preserved");

    TelemetryQueue queue;
    queue.enqueue(PublishRequestKind::STATES);
    queue.enqueue(PublishRequestKind::DISCOVERY);

    PublishRequestKind kind;
    ASSERT_TRUE(queue.dequeue(kind));
    ASSERT_EQ(static_cast<int>(kind), static_cast<int>(PublishRequestKind::STATES));
    ASSERT_TRUE(queue.dequeue(kind));
    ASSERT_EQ(static_cast<int>(kind), static_cast<int>(PublishRequestKind::DISCOVERY));

    test_suite_end("TelemetryQueue::fifo", 1, 0);
    passed++;
  }

  // ── Test: dequeue on empty returns false ──
  {
    test_begin("TelemetryQueue", "dequeue on empty returns false");

    TelemetryQueue queue;
    PublishRequestKind kind = PublishRequestKind::STATES;
    ASSERT_FALSE(queue.dequeue(kind));
    ASSERT_EQ(static_cast<int>(kind), static_cast<int>(PublishRequestKind::STATES));

    test_suite_end("TelemetryQueue::empty_dequeue", 1, 0);
    passed++;
  }

  // ── Test: enqueue on full drops and returns false ──
  {
    test_begin("TelemetryQueue", "enqueue on full drops");

    TelemetryQueue queue;
    for (size_t i = 0; i < TelemetryQueue::CAPACITY; i++) {
      ASSERT_TRUE(queue.enqueue(PublishRequestKind::STATES));
    }
    ASSERT_FALSE(queue.enqueue(PublishRequestKind::DISCOVERY));
    ASSERT_EQ(queue.count(), TelemetryQueue::CAPACITY);

    test_suite_end("TelemetryQueue::full_drop", 1, 0);
    passed++;
  }

  // ── Test: reset clears a full queue ──
  {
    test_begin("TelemetryQueue", "reset clears queue");

    TelemetryQueue queue;
    for (size_t i = 0; i < TelemetryQueue::CAPACITY; i++) {
      queue.enqueue(PublishRequestKind::STATES);
    }
    queue.reset();
    ASSERT_EQ(queue.count(), 0u);

    test_suite_end("TelemetryQueue::reset", 1, 0);
    passed++;
  }

  (void)failed;
  return 0;
}
