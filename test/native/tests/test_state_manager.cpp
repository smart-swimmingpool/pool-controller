/**
 * @file test_state_manager.cpp
 * @brief Unit tests for StateManager — save/load round-trip for all types.
 */

#include <stdio.h>
#include <string.h>
#include <cmath>

#include "Arduino.h"
#include "StateManager.hpp"

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

#define ASSERT_STREQ(a, b)                                                                     \
  do {                                                                                         \
    const char *_a = (a);                                                                      \
    const char *_b = (b);                                                                      \
    if (strcmp(_a, _b) != 0) {                                                                 \
      char _msg[256];                                                                          \
      snprintf(_msg, sizeof(_msg), "Expected '%s' == '%s': got '%s' vs '%s'", #a, #b, _a, _b); \
      test_fail(__FILE__, __LINE__, _msg);                                                     \
      return 1;                                                                                \
    }                                                                                          \
    test_pass(__FILE__, __LINE__);                                                             \
  } while (0)

int run_state_manager_tests() {
  int passed = 0, failed = 0;
  int rc;

  // ── Test: String save/load round-trip ──
  {
    test_begin("StateManager", "string save/load round-trip");

    StateManager::saveString("test_key", "hello_world");
    String result = StateManager::loadString("test_key", "default");

    ASSERT_STREQ(result.c_str(), "hello_world");

    test_suite_end("StateManager::string_rw", 1, 0);
    passed++;
  }

  // ── Test: String returns default for missing key ──
  {
    test_begin("StateManager", "string default for missing key");

    String result = StateManager::loadString("nonexistent", "fallback");

    ASSERT_STREQ(result.c_str(), "fallback");

    test_suite_end("StateManager::string_default", 1, 0);
    passed++;
  }

  // ── Test: Float save/load round-trip ──
  {
    test_begin("StateManager", "float save/load round-trip");

    StateManager::saveFloat("temp_key", 28.5f);
    float result = StateManager::loadFloat("temp_key", 0.0f);

    // Use epsilon comparison
    ASSERT_TRUE(fabs(result - 28.5f) < 0.001f);

    test_suite_end("StateManager::float_rw", 1, 0);
    passed++;
  }

  // ── Test: Float returns default for missing key ──
  {
    test_begin("StateManager", "float default for missing key");

    float result = StateManager::loadFloat("no_float", 15.0f);

    ASSERT_TRUE(fabs(result - 15.0f) < 0.001f);

    test_suite_end("StateManager::float_default", 1, 0);
    passed++;
  }

  // ── Test: Float rejects NaN ──
  {
    test_begin("StateManager", "float rejects NaN from storage");

    // Write NaN directly through Preferences
    Preferences prefs;
    prefs.begin("pool-controller", false);
    prefs.putFloat("nan_key", NAN);
    prefs.end();

    float result = StateManager::loadFloat("nan_key", 42.0f);

    // Should return default value (42.0) because NaN is invalid
    ASSERT_TRUE(fabs(result - 42.0f) < 0.001f);

    test_suite_end("StateManager::float_nan", 1, 0);
    passed++;
  }

  // ── Test: Float rejects out-of-range values ──
  {
    test_begin("StateManager", "float rejects out-of-range");

    Preferences prefs;
    prefs.begin("pool-controller", false);
    prefs.putFloat("big_float", 9999.0f);
    prefs.end();

    float result = StateManager::loadFloat("big_float", 10.0f);

    // Should return default because 9999 > 1000 range
    ASSERT_TRUE(fabs(result - 10.0f) < 0.001f);

    test_suite_end("StateManager::float_range", 1, 0);
    passed++;
  }

  // ── Test: Int save/load round-trip ──
  {
    test_begin("StateManager", "int save/load round-trip");

    StateManager::saveInt("count_key", 42);
    int result = StateManager::loadInt("count_key", 0);

    ASSERT_EQ(result, 42);

    test_suite_end("StateManager::int_rw", 1, 0);
    passed++;
  }

  // ── Test: Int returns default for missing key ──
  {
    test_begin("StateManager", "int default for missing key");

    int result = StateManager::loadInt("no_int", -1);

    ASSERT_EQ(result, -1);

    test_suite_end("StateManager::int_default", 1, 0);
    passed++;
  }

  // ── Test: Int rejects out-of-range values ──
  {
    test_begin("StateManager", "int rejects out-of-range");

    Preferences prefs;
    prefs.begin("pool-controller", false);
    prefs.putInt("big_int", 99999);
    prefs.end();

    int result = StateManager::loadInt("big_int", 0);

    // Should return default because 99999 > 10000 range
    ASSERT_EQ(result, 0);

    test_suite_end("StateManager::int_range", 1, 0);
    passed++;
  }

  // ── Test: Bool save/load round-trip ──
  {
    test_begin("StateManager", "bool save/load round-trip");

    StateManager::saveBool("flag_key", true);
    bool result = StateManager::loadBool("flag_key", false);

    ASSERT_TRUE(result);

    test_suite_end("StateManager::bool_rw", 1, 0);
    passed++;
  }

  // ── Test: Bool returns default for missing key ──
  {
    test_begin("StateManager", "bool default for missing key");

    bool result = StateManager::loadBool("no_bool", true);

    ASSERT_TRUE(result);

    test_suite_end("StateManager::bool_default", 1, 0);
    passed++;
  }

  // ── Test: Clear removes all keys ──
  {
    test_begin("StateManager", "clear removes all keys");

    StateManager::saveString("temp_key", "value");
    StateManager::saveInt("count", 123);
    StateManager::clear();

    String strResult = StateManager::loadString("temp_key", "cleared");
    int intResult = StateManager::loadInt("count", -99);

    ASSERT_STREQ(strResult.c_str(), "cleared");
    ASSERT_EQ(intResult, -99);

    test_suite_end("StateManager::clear", 1, 0);
    passed++;
  }

  printf("\n  StateManager Tests: %d passed, %d failed\n", passed, failed);
  return passed + failed;
}
