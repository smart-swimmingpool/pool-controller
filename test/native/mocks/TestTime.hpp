#pragma once
#include <ctime>

/**
 * @brief Test-only hook to control the mock clock used by getCurrentDateTime().
 *
 * The native test harness stubs getTimeFor() (see stubs.cpp) to return a
 * controllable epoch. Tests that need a specific wall-clock time build the
 * epoch via mktime() from a tm struct, so the localtime_r() round-trip inside
 * getCurrentDateTime() is independent of the host timezone.
 */
void setMockTime(time_t t);