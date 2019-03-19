
#include "RuleAuto.hpp"

RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
}

void RuleAuto::addPumpSwitchNode(RelayModuleNode* node) {
  _poolRelay = node;
}
void RuleAuto::addSolarSwitchNode(RelayModuleNode* node) {
  _solarRelay = node;
}

void RuleAuto::loop() {

  if (getSolarTemperature() > 50) {
  }

  if (getSolarTemperature() > getPoolTemperature()) {
  }
}
