// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

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

  _poolRelay->setSwitch(checkPoolPumpTimer(getPoolTemperature()));

  if (_solarRelay->getSwitch()) {
    _solarRelay->setSwitch(false);
  }
}
