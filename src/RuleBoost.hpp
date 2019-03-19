
#pragma once

#include "Rule.hpp"
#include "RelayModuleNode.hpp"

class RuleBoost : public Rule {
public:
  RuleBoost(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay);

  const char* getMode() { return "boost"; };

  void setSolarRelayNode(RelayModuleNode* relay) { _solarRelay = relay; };
  void setPoolRelayNode(RelayModuleNode* relay) { _poolRelay = relay; };

  virtual void loop();

protected:
  RelayModuleNode* _solarRelay;
  RelayModuleNode* _poolRelay;

private:
  const char* cCaption = "• RuleBoost:";
  const char* cIndent  = "  ◦ ";
};
