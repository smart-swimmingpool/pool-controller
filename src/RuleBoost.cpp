
#include "RuleBoost.hpp"

/**
 *
 */
RuleBoost::RuleBoost(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
}

/**
 *
 */
void RuleBoost::loop() {
  Homie.getLogger() << cIndent << F("§ RuleBoost: loop") << endl;
  if (_poolRelay->getSwitch()) {
    if ((!_solarRelay->getSwitch()) && (getPoolTemperature() < (getPoolMaxTemperature() - getTemperatureHysteresis())) &&
        (getPoolTemperature() < (getSolarTemperature() - getTemperatureHysteresis()))) {
      Homie.getLogger() << cIndent << F("§ RuleBoost: below max. Temperature. Switch solar on") << endl;
      _solarRelay->setSwitch(true);

    } else if ((_solarRelay->getSwitch()) && (getPoolTemperature() > (getPoolMaxTemperature() + getTemperatureHysteresis())) &&
               (getPoolTemperature() > (getSolarTemperature() + getTemperatureHysteresis()))) {
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
