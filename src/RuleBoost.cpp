
#include "RuleBoost.hpp"
#include <cmath>  // For isnan()

/**
 *
 */
RuleBoost::RuleBoost(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay = poolRelay;
}

/**
 *
 */
void RuleBoost::loop() {
  Homie.getLogger() << cIndent << F("§ RuleBoost: loop") << endl;

  // Check for invalid temperatures (NaN from sensor disconnect)
  float poolTemp = getPoolTemperature();
  float solarTemp = getSolarTemperature();

  // Safety: Turn off solar if any temperature is invalid
  if (isnan(poolTemp) || isnan(solarTemp)) {
    if (_solarRelay->getSwitch()) {
      Homie.getLogger() << cIndent << F("§ RuleBoost: Invalid temperature sensor. Switch solar off for safety") << endl;
      _solarRelay->setSwitch(false);
    }
    return;
  }

  if (_poolRelay->getSwitch()) {
    if ((!_solarRelay->getSwitch()) && (poolTemp < (getPoolMaxTemperature() - getTemperatureHysteresis())) &&
      (poolTemp < (solarTemp - getTemperatureHysteresis()))) {
      Homie.getLogger() << cIndent << F("§ RuleBoost: below max. Temperature. Switch solar on") << endl;
      _solarRelay->setSwitch(true);

    } else if ((_solarRelay->getSwitch()) && (poolTemp > (getPoolMaxTemperature() + getTemperatureHysteresis())) &&
      (poolTemp > (solarTemp + getTemperatureHysteresis()))) {
      Homie.getLogger() << cIndent << F("§ RuleBoost: Max. Temperature reached. Switch solar off") << endl;
      _solarRelay->setSwitch(false);

    } else {
      // no change of status
    }
  } else {
    Homie.getLogger() << cIndent << F("§ RuleBoost: pool pump is disabled.") << endl;
    if (_solarRelay->getSwitch()) {
      _solarRelay->setSwitch(false);
    }
  }
}
