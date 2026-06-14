/**
 * @file test_main.cpp
 * @brief Main test runner — entry point for all native C++ tests.
 *
 * Compiles and runs all pool-controller unit tests natively (x86_64)
 * using mocked ESP32/Arduino headers. Coverage is collected via gcov.
 *
 * Build:
 *   cd test/native && mkdir -p build && cd build
 *   cmake .. && make && ./test_runner
 *
 * Coverage:
 *   gcovr --root ../.. --filter 'src/' .
 *   # or: lcov --capture --directory . --output-file coverage.info
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test counters
static int g_testsPassed = 0;
static int g_testsFailed = 0;
static int g_assertionsPassed = 0;
static int g_assertionsFailed = 0;

void test_begin(const char *suite, const char *name) {
  printf("  TEST  %s :: %s\n", suite, name);
}

void test_pass(const char *file, int line) {
  g_assertionsPassed++;
  printf("    ✓ %s:%d\n", file, line);
}

void test_fail(const char *file, int line, const char *msg) {
  g_assertionsFailed++;
  printf("    ✗ %s:%d: %s\n", file, line, msg);
}

void test_suite_end(const char *name, int passed, int failed) {
  if (failed == 0) {
    printf("  ✓ SUITE %s (%d passed)\n", name, passed);
    g_testsPassed++;
  } else {
    printf("  ✗ SUITE %s (%d passed, %d failed)\n", name, passed, failed);
    g_testsFailed++;
  }
}

// Assertion macros
#define ASSERT_TRUE(cond) do { \
  if (!(cond)) { test_fail(__FILE__, __LINE__, "Expected true: " #cond); return 1; } \
  else { test_pass(__FILE__, __LINE__); } \
} while(0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do { \
  auto _a = (a); auto _b = (b); \
  if (_a != _b) { \
    char _msg[256]; snprintf(_msg, sizeof(_msg), "Expected %s == %s: got %lld vs %lld", #a, #b, (long long)_a, (long long)_b); \
    test_fail(__FILE__, __LINE__, _msg); return 1; \
  } \
  test_pass(__FILE__, __LINE__); \
} while(0)

#define ASSERT_STREQ(a, b) do { \
  const char *_a = (a); const char *_b = (b); \
  if (strcmp(_a, _b) != 0) { \
    char _msg[256]; snprintf(_msg, sizeof(_msg), "Expected strcmp(%s, %s) == 0: got '%s' vs '%s'", #a, #b, _a, _b); \
    test_fail(__FILE__, __LINE__, _msg); return 1; \
  } \
  test_pass(__FILE__, __LINE__); \
} while(0)

#define ASSERT_NEAR(a, b, eps) do { \
  float _a = (a); float _b = (b); \
  if (fabs(_a - _b) > (eps)) { \
    char _msg[256]; snprintf(_msg, sizeof(_msg), "Expected |%s - %s| < %f: got %f vs %f", #a, #b, (float)(eps), _a, _b); \
    test_fail(__FILE__, __LINE__, _msg); return 1; \
  } \
  test_pass(__FILE__, __LINE__); \
} while(0)

// Suite declarations
extern int run_rule_tests();
extern int run_config_manager_tests();
extern int run_webportal_json_tests();
extern int run_mqttpublisher_tests();

int main() {
  printf("\n══════════════════════════════════════════════════\n");
  printf("  Pool Controller — Native Unit Tests\n");
  printf("══════════════════════════════════════════════════\n\n");

  int total = 0;
  total += run_rule_tests();
  total += run_config_manager_tests();
  total += run_webportal_json_tests();
  total += run_mqttpublisher_tests();

  printf("\n══════════════════════════════════════════════════\n");
  printf("  Results: %d suites passed, %d suites failed\n", g_testsPassed, g_testsFailed);
  printf("  Assertions: %d passed, %d failed\n", g_assertionsPassed, g_assertionsFailed);
  printf("══════════════════════════════════════════════════\n\n");

  return g_testsFailed > 0 ? 1 : 0;
}
