/**
 * Mock implementation of temperature sensor for unit testing
 */

#pragma once

#include <cstdint>

class MockTemperatureSensor {
public:
    MockTemperatureSensor() : _temperature(20.0f) {}

    void setTemperature(float temp) {
        _temperature = temp;
    }

    float getTemperature() const {
        return _temperature;
    }

    void setPin(uint8_t pin) {
        _pin = pin;
    }

    uint8_t getPin() const {
        return _pin;
    }

private:
    float _temperature;
    uint8_t _pin = 0;
};
