#pragma once

#include "Rule.hpp"

/**
 * @brief Mock RuleBoost for native tests — avoids pulling in RelayModuleNode.
 */
class RuleBoost : public Rule {
public:
  RuleBoost() {}
  RuleBoost(void *, void *) {}

  const char *getMode() override { return "boost"; }
  void loop() override {}

  uint16_t getEffectiveRuntimeMinutes() const { return 1440; }
};
