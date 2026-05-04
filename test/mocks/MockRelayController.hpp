/**
 * Mock implementation of IRelayController for unit testing
 */

#pragma once

#include <cstdint>

class MockRelayController {
public:
    MockRelayController() : _state(false) {}

    void setSwitch(bool state) {
        _state = state;
        onSetSwitchCalled = true;
        lastSetState = state;
    }

    bool getSwitch() const {
        onGetSwitchCalled = true;
        return _state;
    }

    // For test verification
    bool onSetSwitchCalled = false;
    bool onGetSwitchCalled = false;
    bool lastSetState = false;

    void reset() {
        onSetSwitchCalled = false;
        onGetSwitchCalled = false;
        lastSetState = false;
    }

private:
    bool _state;
};
