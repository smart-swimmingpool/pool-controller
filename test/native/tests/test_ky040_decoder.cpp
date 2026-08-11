#include "Ky040Decoder.hpp"

#include <cstdio>

#define ASSERT_EQ(a, b)                                                                                    \
  do {                                                                                                     \
    auto _a = (a);                                                                                         \
    auto _b = (b);                                                                                         \
    if (_a != _b) {                                                                                        \
      printf("    ✗ %s:%d expected equality\n", __FILE__, __LINE__);                                     \
      return 1;                                                                                            \
    }                                                                                                      \
  } while (0)

using PoolController::Ky040Decoder;
using PoolController::LocalUiEvent;

static int test_clockwise_rotation() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, false, 0), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, true, false, 5), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, false, false, 10), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, false, false, 15), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 20), LocalUiEvent::ROTATE_CLOCKWISE);
  return 0;
}

static int test_counter_clockwise_rotation() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, false, 0), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, false, false, 5), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, false, false, 10), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(false, true, false, 15), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 20), LocalUiEvent::ROTATE_COUNTER_CLOCKWISE);
  return 0;
}

static int test_short_press_on_release() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, true, 100), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, true, 300), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 350), LocalUiEvent::SHORT_PRESS);
  return 0;
}

static int test_long_press_on_release() {
  Ky040Decoder decoder;
  ASSERT_EQ(decoder.update(true, true, true, 100), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, true, 2300), LocalUiEvent::NONE);
  ASSERT_EQ(decoder.update(true, true, false, 2350), LocalUiEvent::LONG_PRESS);
  return 0;
}

int run_ky040_decoder_tests() {
  int failures = 0;
  failures += test_clockwise_rotation();
  failures += test_counter_clockwise_rotation();
  failures += test_short_press_on_release();
  failures += test_long_press_on_release();
  if (failures == 0) {
    printf("  KY-040 Decoder Tests: 4 passed, 0 failed\n");
  }
  return failures;
}
