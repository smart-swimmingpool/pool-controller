// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file RuleAuto.hpp
 * @brief Automatic solar-optimised mode — heats pool when solar temperature permits.
 */

#pragmaonce

#include "Rule.hpp"
#include "RelayModuleNode.hpp"
#include "TimeClientHelper.hpp"

/**
 * @brief Automatic operation mode with smart solar heating.
 *
 * The pool pump runs on a timer schedule. Solar heating is activated when:
 *   - The pool pump is running (timer active)
 *   - Solar collector temperature exceeds pool temperature + hysteresis
 *   - Pool water temperature is below the configured maximum
 */
class RuleAuto : public Rule {
public:
  RuleAuto(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay);

  const char *getMode() override { return "auto"; }

  void setSolarRelayNode(RelayModuleNode *relay) { _solarRelay = relay; }
  void setPoolRelayNode(RelayModuleNode *relay) { _poolRelay = relay; }

  void loop() override;

private:
  RelayModuleNode *_solarRelay;
  RelayModuleNode *_poolRelay;

  const char *cCaption = "• RuleAuto:";
  const char *cIndent = "  ◦ ";
};
