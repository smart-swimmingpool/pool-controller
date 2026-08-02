// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file RuleBoost.cpp
 * @brief Boost mode — both pumps run continuously.
 */

#include "RuleBoost.hpp"
#include <Arduino.h>
#include <cmath>  // For isnan()

#include "LogCapture.hpp"

RuleBoost::RuleBoost(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

void RuleBoost::loop() {
  LOG_INFO("%s§ RuleBoost: loop\n", cIndent);

  // Check for invalid temperatures (NaN from sensor disconnect)
  float poolTemp = getPoolTemperature();
  float solarTemp = getSolarTemperature();

  // Safety: Turn off solar if any temperature is invalid
  if (isnan(poolTemp) || isnan(solarTemp)) {
    if (_solarRelay->getSwitch()) {
      LOG_INFO("%s§ RuleBoost: Invalid temperature sensor. Switch solar off for safety\n", cIndent);
      _solarRelay->setSwitch(false);
    }
    return;
  }

  if (_poolRelay->getSwitch()) {
    if (_solarRelay->getSwitch()) {
      // solar is ON — check if it should turn OFF
      if (poolTemp > getPoolMaxTemperature()) {
        LOG_INFO("%s§ RuleBoost: Maximum pool temp reached. Switch solar off\n", cIndent);
        _solarRelay->setSwitch(false);
      } else if (poolTemp > (solarTemp + getTemperatureHysteresis())) {
        LOG_INFO("%s§ RuleBoost: Pool temp reaches solar temp. Switch solar off\n", cIndent);
        _solarRelay->setSwitch(false);
      }
    } else {
      // solar is OFF — check if it should turn ON
      if ((poolTemp < (getPoolMaxTemperature() - getTemperatureHysteresis())) &&
        (poolTemp < (solarTemp - getTemperatureHysteresis()))) {
        LOG_INFO("%s§ RuleBoost: below max. Temperature. Switch solar on\n", cIndent);
        _solarRelay->setSwitch(true);
      }
    }
  } else {
    LOG_INFO("%s§ RuleBoost: pool pump is disabled.\n", cIndent);
    if (_solarRelay->getSwitch()) {
      _solarRelay->setSwitch(false);
    }
  }
}
