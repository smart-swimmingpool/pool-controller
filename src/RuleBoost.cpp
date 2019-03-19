
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

  if (_poolRelay->getSwitch()) {
    if ((!_solarRelay->getSwitch()) && (getPoolTemperature() <= (getPoolMaxTemperature() - getTemperaturHysteresis()))) {
      Homie.getLogger() << cIndent << "Boost: below max. Temperature. Switch solar on" << endl;
      _solarRelay->setSwitch(true);

    } else if ((_solarRelay->getSwitch()) && (getPoolTemperature() > (getPoolMaxTemperature() + getTemperaturHysteresis()))) {
      Homie.getLogger() << cIndent << "Boost: Max. Temperature reached. Switch solar off" << endl;
      _solarRelay->setSwitch(false);

    } else {
      // no change of status
    }
  } else {
    Homie.getLogger() << cIndent << "pool pump is disabled." << endl;
  }
}
