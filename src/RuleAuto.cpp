
#include "RuleAuto.hpp"

/**
 *
 */
RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
}

/**
 *
 */
void RuleAuto::addPumpSwitchNode(RelayModuleNode* node) {
  _poolRelay = node;
}

/**
 *
 */
void RuleAuto::addSolarSwitchNode(RelayModuleNode* node) {
  _solarRelay = node;
}

/**
 *
 */
void RuleAuto::loop() {
  if (_poolRelay->getSwitch()) {
    if ((!_solarRelay->getSwitch()) && (getPoolTemperature() <= (getPoolMaxTemperature() - getTemperaturHysteresis()))) {
      Homie.getLogger() << cIndent << "RuleAuto: below max. Temperature. Switch solar on" << endl;
      _solarRelay->setSwitch(true);

    } else if ((_solarRelay->getSwitch()) && (getPoolTemperature() > (getPoolMaxTemperature() + getTemperaturHysteresis()))) {
      Homie.getLogger() << cIndent << "RuleAuto: Max. Temperature reached. Switch solar off" << endl;
      _solarRelay->setSwitch(false);

    } else {
      // no change of status
    }
  } else {
    Homie.getLogger() << cIndent << "RuleAuto: pool pump is disabled." << endl;
  }
}

bool RuleAuto::checkPoolPumpTimer() {

}
