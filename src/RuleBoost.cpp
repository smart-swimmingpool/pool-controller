
#include "RuleBoost.hpp"

RuleBoost::RuleBoost(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
}

void RuleBoost::loop() {

  if ((!_solarRelay->getSwitch()) && (getPoolTemperature() <= (getPoolMaxTemperature() - getTemperaturHysteresis()))) {
    _solarRelay->setSwitch(true);

  } else if ((_solarRelay->getSwitch()) && (getPoolTemperature() > (getPoolMaxTemperature() + getTemperaturHysteresis()))) {
    _solarRelay->setSwitch(false);
  } else {
    // no change of status
  }
}
