
#pragma once

#include "Rule.hpp"
#include "RelayModuleNode.hpp"

class RuleAuto : public Rule {
public:
  RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay);

  const char* getMode() { return "auto"; };
  void        setSolarRelayNode(RelayModuleNode* relay) { _solarRelay = relay; };
  void        setPoolRelayNode(RelayModuleNode* relay) { _poolRelay = relay; };

  virtual void loop();

  virtual void addPumpSwitchNode(RelayModuleNode* node);
  virtual void addSolarSwitchNode(RelayModuleNode* node);

protected:
  RelayModuleNode* _solarRelay;
  RelayModuleNode* _poolRelay;
};
