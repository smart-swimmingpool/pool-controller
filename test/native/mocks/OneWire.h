#pragma once

#include <stdint.h>

typedef uint8_t DeviceAddress[8];

class OneWire {
public:
    OneWire() : _pin(0) {}
    OneWire(uint8_t pin) : _pin(pin) {}
    void begin() { _pin = _pin; }
    void begin(uint8_t pin) { _pin = pin; }
    bool reset() { return false; }
    void select(const uint8_t *) {}
    void skip() {}
    void write(uint8_t, uint8_t = 0) {}
    uint8_t read() { return 0; }
    void reset_search() {}
    bool search(uint8_t *) { return false; }
    uint8_t crc8(const uint8_t *, uint8_t) { return 0; }
private:
    uint8_t _pin;
};
