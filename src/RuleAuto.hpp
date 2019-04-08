
#pragma once


#include "TimeLib.h"
#include "Timezone.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

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
  tm  getCurrentDateTime();

private:
  const char* cCaption = "• RuleAuto:";
  const char* cIndent  = "  ◦ ";

  String poolPumpStart = "10:30";
  String poolPumpEnd   = "16:45";

};
