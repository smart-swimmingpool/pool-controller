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

RuleBoost::RuleBoost(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

void RuleBoost::loop() {
  Serial.print(cIndent);
  Serial.println(F("§ RuleBoost: loop"));

  // Check for invalid temperatures (NaN from sensor disconnect)
  float poolTemp = getPoolTemperature();
  float solarTemp = getSolarTemperature();

  // Safety: Turn off solar if any temperature is invalid
  if (isnan(poolTemp) || isnan(solarTemp)) {
    if (_solarRelay->getSwitch()) {
      Serial.print(cIndent);
      Serial.println(F("§ RuleBoost: Invalid temperature sensor. Switch solar off for safety"));
      _solarRelay->setSwitch(false);
    }
    return;
  }

  if (_poolRelay->getSwitch()) {
    if (_solarRelay->getSwitch()) {
      // solar is ON — check if it should turn OFF
      if (poolTemp > getPoolMaxTemperature()) {
        Serial.print(cIndent);
        Serial.println(F("§ RuleBoost: Maximum pool temp reached. Switch solar off"));
        _solarRelay->setSwitch(false);
      } else if (poolTemp > (solarTemp + getTemperatureHysteresis())) {
        Serial.print(cIndent);
        Serial.println(F("§ RuleBoost: Pool temp reaches solar temp. Switch solar off"));
        _solarRelay->setSwitch(false);
      }
    } else {
      // solar is OFF — check if it should turn ON
      if ((poolTemp < (getPoolMaxTemperature() - getTemperatureHysteresis())) &&
        (poolTemp < (solarTemp - getTemperatureHysteresis()))) {
        Serial.print(cIndent);
        Serial.println(F("§ RuleBoost: below max. Temperature. Switch solar on"));
        _solarRelay->setSwitch(true);
      }
    }
  } else {
    Serial.print(cIndent);
    Serial.println(F("§ RuleBoost: pool pump is disabled."));
    if (_solarRelay->getSwitch()) {
      _solarRelay->setSwitch(false);
    }
  }
}
