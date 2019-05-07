
#pragma once

#include "Rule.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"

class RuleTimer : public Rule {
public:
  RuleTimer(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay);

  const char* getMode() { return "timer"; };

  void setSolarRelayNode(RelayModuleNode* relay) { _solarRelay = relay; };
  void setPoolRelayNode(RelayModuleNode* relay) { _poolRelay = relay; };

  virtual void loop();

protected:
  bool checkPoolPumpTimer();

private:
  RelayModuleNode* _solarRelay;
  RelayModuleNode* _poolRelay;

  const char* cCaption = "• RuleTimer:";
  const char* cIndent  = "  ◦ ";
};
