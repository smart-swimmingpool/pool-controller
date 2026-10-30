// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file test_assert.h
 * @brief Shared assertion macros for the native test runner.
 *
 * Each macro records a pass or a failure in test_main.cpp and returns 1 from
 * the enclosing test function on failure.
 */

#pragma once

#include <cmath>
#include <cstdio>

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

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))

#define ASSERT_NEAR(a, b, eps)                                                                             \
  do {                                                                                                     \
    float _a = (a);                                                                                        \
    float _b = (b);                                                                                        \
    if (std::fabs(_a - _b) > (eps)) {                                                                      \
      char _msg[256];                                                                                      \
      snprintf(_msg, sizeof(_msg), "Expected |%s - %s| < %f: got %f vs %f", #a, #b, (float)(eps), _a, _b); \
      test_fail(__FILE__, __LINE__, _msg);                                                                 \
      return 1;                                                                                            \
    }                                                                                                      \
    test_pass(__FILE__, __LINE__);                                                                         \
  } while (0)
