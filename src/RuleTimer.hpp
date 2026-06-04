// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

#pragma once

#include "Rule.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"

class RuleTimer : public Rule {
public:
  RuleTimer(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay);

  const char *getMode() override { return "timer"; }

  void setSolarRelayNode(RelayModuleNode *relay) { _solarRelay = relay; }
  void setPoolRelayNode(RelayModuleNode *relay) { _poolRelay = relay; }

  void loop() override;

private:
  RelayModuleNode *_solarRelay;
  RelayModuleNode *_poolRelay;

  const char *cCaption = "• RuleTimer:";
  const char *cIndent = "  ◦ ";
};
