// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file RuleAuto.cpp
 * @brief Automatic solar-optimised mode implementation with smart heating logic.
 */

#include "RuleAuto.hpp"
#include <cmath>  // for isnan()

RuleAuto::RuleAuto(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

void RuleAuto::loop() {
  Serial.println("§ RuleAuto: loop");

  // Validate temperature readings before making decisions
  float poolTemp = getPoolTemperature();
  float solarTemp = getSolarTemperature();

  if (std::isnan(poolTemp) || std::isnan(solarTemp)) {
    Serial.println("  ⚠ RuleAuto: Invalid temperature readings detected");
    Serial.printf("  Pool temp: %f\n", poolTemp);
    Serial.printf("  Solar temp: %f\n", solarTemp);
    Serial.println("  Turning off solar pump for safety");

    // Turn off solar pump for safety, but keep pool pump running on timer
    _solarRelay->setSwitch(false);
    _poolRelay->setSwitch(checkPoolPumpTimer());
    return;
  }

  _poolRelay->setSwitch(checkPoolPumpTimer(poolTemp));

  if (_poolRelay->getSwitch()) {
    // pool pump is running

    if (_solarRelay->getSwitch()) {
      // solar is on
      float hyst = getTemperatureHysteresis();
      if (getSolarTemperature() < (getSolarMinTemperature() - hyst)) {
        Serial.printf("  § RuleAuto: Solar below min. required solar temp. (%f). Switch solar off\n", getSolarMinTemperature());
        _solarRelay->setSwitch(false);
      } else if (getPoolTemperature() >= (getSolarTemperature() + hyst)) {
        Serial.printf("  § RuleAuto: Pool temp. (%f) reaches solar temp (%f). Switch solar off\n", getPoolTemperature(),
          getSolarTemperature());
        _solarRelay->setSwitch(false);
      } else if (getPoolTemperature() >= (getPoolMaxTemperature() + hyst)) {
        Serial.printf("  § RuleAuto: Pool temp. (%f) above max. temperature (%f). Switch solar off\n", getPoolTemperature(),
          getPoolMaxTemperature());
        _solarRelay->setSwitch(false);
      } else {
        Serial.println("  § RuleAuto: Solar on -> no change");
      }
    } else {
      // solar is off
      if ((getPoolTemperature() <= getPoolMaxTemperature()) && (getPoolTemperature() <= getSolarTemperature()) &&
        (getSolarMinTemperature() <= getSolarTemperature())) {
        Serial.printf("  § RuleAuto: below max. Temperature (%f). Switch solar on\n", getPoolMaxTemperature());
        _solarRelay->setSwitch(true);
      } else {
        Serial.println("  § RuleAuto: Solar off -> no change");
      }
    }
  } else {
    if (_solarRelay->getSwitch()) {
      Serial.println("  § RuleAuto: pool pump is disabled. Switch solar off");
      _solarRelay->setSwitch(false);
    }
  }

  Serial.printf("  § RuleAuto: Pool temp. :     %f\n", getPoolTemperature());
  Serial.printf("  § RuleAuto: max. Pool temp.: %f\n", getPoolMaxTemperature());
  Serial.printf("  § RuleAuto: Solar temp. :     %f\n", getSolarTemperature());
  Serial.printf("  § RuleAuto: min. Solar temp.: %f\n", getSolarMinTemperature());
}
