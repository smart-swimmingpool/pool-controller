// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file RuleTimer.cpp
 * @brief Timer-only mode — pool pump follows schedule, solar stays off.
 */

#include "RuleTimer.hpp"

RuleTimer::RuleTimer(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

void RuleTimer::loop() {
  Serial.println("§ RuleTimer: loop");

  _poolRelay->setSwitch(checkPoolPumpTimer());

  if (_solarRelay->getSwitch()) {
    _solarRelay->setSwitch(false);
  }
}
