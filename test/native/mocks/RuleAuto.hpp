#pragma once

#include "Rule.hpp"

/**
 * @brief Mock RuleAuto for native tests — avoids pulling in RelayModuleNode.
 */
class RuleAuto : public Rule {
public:
  RuleAuto() {
    _timerSetting.timerStartHour = 8;
    _timerSetting.timerStartMinutes = 0;
    _timerSetting.timerEndHour = 18;
    _timerSetting.timerEndMinutes = 0;
  }
  RuleAuto(void *, void *) {
    _timerSetting.timerStartHour = 8;
    _timerSetting.timerStartMinutes = 0;
    _timerSetting.timerEndHour = 18;
    _timerSetting.timerEndMinutes = 0;
  }

  const char *getMode() override { return "auto"; }
  void loop() override {}
};
