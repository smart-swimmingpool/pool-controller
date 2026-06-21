#pragma once

#include "Arduino.h"

class MDNSResponder {
public:
    bool begin(const char *) { return true; }
    void addService(const char *, const char *, uint16_t) {}
    void addServiceTxt(const char *, const char *, const char *) {}
    void update() {}
    void end() {}
};
static MDNSResponder MDNS;
