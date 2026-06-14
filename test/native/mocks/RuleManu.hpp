#pragma once

#include "Rule.hpp"

/**
 * @brief Mock RuleManu for native tests — avoids pulling in RelayModuleNode.
 */
class RuleManu : public Rule {
public:
  RuleManu() {}
  RuleManu(void *) {}

  const char *getMode() override { return "manu"; }
  void loop() override {}

  uint16_t getEffectiveRuntimeMinutes() const { return 0; }
};
