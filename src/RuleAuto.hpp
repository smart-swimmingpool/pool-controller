
#pragma once

#include <time.h>
#include "simpleDSTadjust.h"
#include "Rule.hpp"
#include "RelayModuleNode.hpp"

class RuleAuto : public Rule {
public:
  RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay);

  const char* getMode() { return "auto"; };

  void setSolarRelayNode(RelayModuleNode* relay) { _solarRelay = relay; };
  void setPoolRelayNode(RelayModuleNode* relay) { _poolRelay = relay; };

  virtual void loop();

  virtual void addPumpSwitchNode(RelayModuleNode* node);
  virtual void addSolarSwitchNode(RelayModuleNode* node);

protected:
  RelayModuleNode* _solarRelay;
  RelayModuleNode* _poolRelay;

  bool checkPoolPumpTimer();
  tm*  getCurrentDateTime();
  void computeTimeDifference(struct tm t1, struct tm t2, struct tm* difference);

private:
  const char* cCaption = "• RuleAuto:";
  const char* cIndent  = "  ◦ ";

  String poolPumpStart = "10:30";
  String poolPumpEnd   = "16:45";

  simpleDSTadjust* _dstAdjusted;
};
