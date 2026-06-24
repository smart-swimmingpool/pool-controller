#pragma once
#include <ctime>
#include <cstdint>

// TimeDegradation is defined in the production TimeClientHelper.hpp.
// Do NOT redefine here — it's already included via Rule.hpp → Timer.hpp chain.
// When needed in stubs.cpp, define it directly.

// tm conversion helpers — shadow the declarations in TimeLib.h (mock).
// These are NOT declared in the production TimeClientHelper.hpp.
inline int year(time_t t) { return 2026; }
inline int month(time_t t) { return 6; }
inline int day(time_t t) { return 13; }
inline int hour(time_t t) { return 14; }
inline int minute(time_t t) { return 30; }
inline int second(time_t t) { return 0; }
inline int weekday(time_t t) { return 6; }

// Stub for syncSystemClock (used by OtaUpdater.cpp in native tests)
void syncSystemClock();
