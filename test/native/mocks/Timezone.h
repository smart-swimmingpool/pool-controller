#pragma once
#include <stdint.h>
#include "TimeLib.h"

#ifndef TIMECHANGERULE_DEFINED
#define TIMECHANGERULE_DEFINED
struct TimeChangeRule {
    char abbrev[6] = "UTC";
    uint8_t week = 0;
    uint8_t dow = 0;
    uint8_t month = 0;
    uint8_t hour = 0;
    int offset = 0;
};
#endif

class Timezone {
public:
    Timezone() {}
    Timezone(const TimeChangeRule &dst, const TimeChangeRule &std) : _dst(dst), _std(std) {}
    time_t toLocal(time_t utc, TimeChangeRule **tcr = nullptr) {
        if (tcr) *tcr = &_std;
        return utc + _std.offset * 60;
    }
    time_t toUTC(time_t local, TimeChangeRule **tcr = nullptr) {
        if (tcr) *tcr = &_std;
        return local - _std.offset * 60;
    }
    bool utcIsDST(time_t utc) { return false; }
    bool locIsDST(time_t local) { return false; }
    void setRules(const TimeChangeRule &dst, const TimeChangeRule &std) {
        _dst = dst; _std = std;
    }
private:
    TimeChangeRule _dst;
    TimeChangeRule _std;
};
