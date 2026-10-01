// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file RuleAuto.cpp
 * @brief Automatic solar-optimised mode implementation with smart heating logic.
 */

#include "RuleAuto.hpp"
#include <cmath>  // for isnan()

#include "LogCapture.hpp"

RuleAuto::RuleAuto(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

void RuleAuto::loop() {
  LOG_INFO("§ RuleAuto: loop\n");

  // Validate temperature readings before making decisions
  float poolTemp = getPoolTemperature();
  float solarTemp = getSolarTemperature();

  if (std::isnan(poolTemp) || std::isnan(solarTemp)) {
    LOG_WARN("  ⚠ RuleAuto: Invalid temperature readings detected\n");
    LOG_WARN("  Pool temp: %f\n", poolTemp);
    LOG_WARN("  Solar temp: %f\n", solarTemp);
    LOG_WARN("  Turning off solar pump for safety\n");

    // Turn off solar pump for safety, but keep pool pump running on timer
    _solarRelay->setSwitch(false);
    _poolRelay->setSwitch(checkPoolPumpTimer());
    return;
  }

  _poolRelay->setSwitch(checkPoolPumpTimer(poolTemp));

  float hyst = getTemperatureHysteresis();

  if (_poolRelay->getSwitch()) {
    // pool pump is running

    if (_solarRelay->getSwitch()) {
      // solar is on
      if (getSolarTemperature() < (getSolarMinTemperature() - hyst)) {
        LOG_INFO("  § RuleAuto: Solar below min. required solar temp. (%f). Switch solar off\n", getSolarMinTemperature());
        _solarRelay->setSwitch(false);
      } else if (getPoolTemperature() >= (getSolarTemperature() + hyst)) {
        LOG_INFO("  § RuleAuto: Pool temp. (%f) reaches solar temp (%f). Switch solar off\n", getPoolTemperature(),
          getSolarTemperature());
        _solarRelay->setSwitch(false);
      } else if (getPoolTemperature() >= getPoolMaxTemperature()) {
        LOG_INFO("  § RuleAuto: Pool temp. (%f) reached max. temperature (%f). Switch solar off\n", getPoolTemperature(),
          getPoolMaxTemperature());
        _solarRelay->setSwitch(false);
      } else {
        LOG_INFO("  § RuleAuto: Solar on -> no change\n");
      }
    } else {
      // solar is off
      if ((getPoolTemperature() <= (getPoolMaxTemperature() - hyst)) &&
        (getPoolTemperature() <= (getSolarTemperature() - hyst)) &&
        ((getSolarMinTemperature() + hyst) <= getSolarTemperature())) {
        LOG_INFO("  § RuleAuto: Pool temp (%f) below max temp minus hysteresis (%f). Switch solar on\n", getPoolTemperature(),
          getPoolMaxTemperature() - hyst);
        _solarRelay->setSwitch(true);
      } else {
        LOG_INFO("  § RuleAuto: Solar off -> no change\n");
      }
    }
  } else {
    if (_solarRelay->getSwitch()) {
      LOG_INFO("  § RuleAuto: pool pump is disabled. Switch solar off\n");
      _solarRelay->setSwitch(false);
    }
  }

  LOG_INFO("  § RuleAuto: Pool temp. :     %f\n", getPoolTemperature());
  LOG_INFO("  § RuleAuto: max. Pool temp.: %f\n", getPoolMaxTemperature());
  LOG_INFO("  § RuleAuto: Solar temp. :     %f\n", getSolarTemperature());
  LOG_INFO("  § RuleAuto: min. Solar temp.: %f\n", getSolarMinTemperature());
}
