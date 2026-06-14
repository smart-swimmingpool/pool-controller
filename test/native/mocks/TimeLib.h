#pragma once

#include <stdint.h>
#include <sys/types.h>  // for time_t

typedef struct {
    uint8_t Second;
    uint8_t Minute;
    uint8_t Hour;
    uint8_t Wday;  // day of week, sunday is day 1
    uint8_t Day;
    uint8_t Month;
    uint8_t Year;
} tmElements_t;

time_t now();
void setTime(time_t t);
void setTime(int hr, int min, int sec, int day, int month, int yr);
time_t makeTime(const tmElements_t &tm);
void breakTime(time_t time, tmElements_t &tm);
int hour(time_t t);
int minute(time_t t);
int second(time_t t);
int day(time_t t);
int month(time_t t);
int year(time_t t);
int weekday(time_t t);
int dayStr(int day);
int monthStr(int month);
time_t previousMidnight(time_t t);

#define SECS_PER_MIN 60
#define SECS_PER_HOUR 3600
#define SECS_PER_DAY 86400
#define SECS_PER_YEAR 31536000L
#define SECS_YR_2000 946684800L

#define LEAP_YEAR(year) (((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0)))

#define weekdaySunday 1
#define weekdayMonday 2
#define weekdayTuesday 3
#define weekdayWednesday 4
#define weekdayThursday 5
#define weekdayFriday 6
#define weekdaySaturday 7
