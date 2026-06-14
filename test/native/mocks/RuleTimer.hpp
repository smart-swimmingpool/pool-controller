#pragma once

#include "Rule.hpp"

/**
 * @brief Mock RuleTimer for native tests — avoids pulling in RelayModuleNode.
 */
class RuleTimer : public Rule {
public:
  RuleTimer() {}
  RuleTimer(void *, void *) {}

  const char *getMode() override { return "timer"; }
  void loop() override {}

  void setCustomEndMinutes(uint16_t minutes) { _activeEndMinutes = minutes; }
};
