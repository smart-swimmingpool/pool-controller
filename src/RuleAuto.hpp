
#pragma once


#include "TimeLib.h"
#include "Timezone.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

#include "Rule.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"

class RuleAuto : public Rule {
public:
  RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay);

  const char* getMode() { return "auto"; };

  void setSolarRelayNode(RelayModuleNode* relay) { _solarRelay = relay; };
  void setPoolRelayNode(RelayModuleNode* relay) { _poolRelay = relay; };

  virtual void loop();

protected:
  bool checkPoolPumpTimer();

private:
  RelayModuleNode* _solarRelay;
  RelayModuleNode* _poolRelay;

  const char* cCaption = "• RuleAuto:";
  const char* cIndent  = "  ◦ ";
};
