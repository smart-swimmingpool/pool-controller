/**
 * @file test_main.cpp
 * @brief Main test runner — entry point for all native C++ tests.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

extern int run_rule_tests();
extern int run_config_manager_tests();
extern int run_webportal_json_tests();
extern int run_mqttpublisher_tests();
extern int run_security_tests();
extern int run_state_manager_tests();
extern int run_timer_tests();
extern int run_logcapture_tests();
extern int run_webportal_logs_tests();
extern int run_local_settings_menu_tests();
extern int run_ky040_decoder_tests();
extern int run_calibration_manager_tests();
extern int run_telemetry_queue_tests();
extern int run_sensor_slots_tests();
extern int run_core_scheduler_tests();
extern int run_degradation_manager_tests();

int main() {
  printf("\n══════════════════════════════════════════════════\n");
  printf("  Pool Controller — Native Unit Tests\n");
  printf("══════════════════════════════════════════════════\n\n");

  int total = 0;
  total += run_rule_tests();
  total += run_config_manager_tests();
  total += run_webportal_json_tests();
  total += run_mqttpublisher_tests();
  total += run_security_tests();
  total += run_state_manager_tests();
  total += run_timer_tests();
  total += run_logcapture_tests();
  total += run_webportal_logs_tests();
  total += run_local_settings_menu_tests();
  total += run_ky040_decoder_tests();
  total += run_calibration_manager_tests();
  total += run_telemetry_queue_tests();
  total += run_sensor_slots_tests();
  total += run_core_scheduler_tests();
  total += run_degradation_manager_tests();
  (void)total;

  printf("\n══════════════════════════════════════════════════\n");
  printf("  Results: %d suites passed, %d suites failed\n", g_testsPassed, g_testsFailed);
  printf("  Assertions: %d passed, %d failed\n", g_assertionsPassed, g_assertionsFailed);
  printf("══════════════════════════════════════════════════\n\n");

  return g_testsFailed > 0 ? 1 : 0;
}
