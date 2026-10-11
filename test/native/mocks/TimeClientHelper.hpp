#pragma once
#include <ctime>
#include <cstdint>

#include "Arduino.h"
#include "TimeLib.h"

// tm conversion helpers — shadow the declarations in TimeLib.h (mock).
inline int year(time_t t) { return 2026; }
inline int month(time_t t) { return 6; }
inline int day(time_t t) { return 13; }
inline int hour(time_t t) { return 14; }
inline int minute(time_t t) { return 30; }
inline int second(time_t t) { return 0; }
inline int weekday(time_t t) { return 6; }

bool isTimeSyncValid();
int getTzCount();
int getTimezoneLabelCount();
const char *const *getTimezoneLabelList();
int getTimezoneIndexFromLabel(const char *label);
String getFormattedTime(time_t rawTime);
time_t getUtcTime();
int getTimezoneIndex();
time_t getLastValidSyncTime();
bool forceNtpUpdate();
void setTimeDegradationGreenHours(uint8_t hours);
uint8_t getTimeDegradationGreenHours();
void setTimeDegradationRedHours(uint8_t hours);
uint8_t getTimeDegradationRedHours();
time_t getTimeFor(int tzIndex, TimeChangeRule **tcr);
String getTimeInfoFor(int index);
void setTimezoneIndex(int index);
void timeClientSetup(const char *ntpServer);
void syncSystemClock();
