#pragma once

#include "Arduino.h"
#include "WiFiUdp.h"

class NTPClient {
public:
    explicit NTPClient(WiFiUDP &udp, const char *poolServerName = "pool.ntp.org", int timeOffset = 0, int updateInterval = 60000)
        : _udp(&udp) {}

    void begin() {}
    void end() {}
    void forceUpdate() { _lastUpdate = millis() / 1000; _lastEpoch = _lastUpdate; }
    bool update() { return true; }
    bool isTimeSet() const { return _lastEpoch > 0; }
    time_t getEpochTime() const { return _lastEpoch; }
    int getDay() const { return 1; }
    int getHours() const { return 12; }
    int getMinutes() const { return 0; }
    int getSeconds() const { return 0; }
    String getFormattedTime() const { return String("12:00:00"); }
    void setTimeOffset(int offset) { _timeOffset = offset; }
    int getTimeOffset() const { return _timeOffset; }
    void setUpdateInterval(int interval) { _updateInterval = interval; }
    void setRandomPort(bool) {}

private:
    WiFiUDP *_udp;
    time_t _lastEpoch = 1000000;
    time_t _lastUpdate = 0;
    int _timeOffset = 0;
    int _updateInterval = 60000;
};
