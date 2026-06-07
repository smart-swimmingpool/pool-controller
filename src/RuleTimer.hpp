// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file RuleTimer.hpp
 * @brief Timer-only mode — cleaning pump on schedule, solar disabled.
 */

#pragma once

#include "Rule.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"

/**
 * @brief Timer-only operation mode — only the pool pump runs on a schedule.
 *
 * Solar heating is disabled. The pool circulation pump follows the configured
 * timer start/end times. Useful for scheduled cleaning without heating.
 */
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
