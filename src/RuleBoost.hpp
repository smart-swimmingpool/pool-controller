// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file RuleBoost.hpp
 * @brief Boost mode — full heating power, both pumps enabled.
 */

#pragma once

#include "Rule.hpp"
#include "RelayModuleNode.hpp"

/**
 * @brief Boost operation mode — both pumps run continuously.
 *
 * Pool pump and solar pump are both switched ON regardless of temperature
 * conditions. Used for rapid heating or manual override.
 */
class RuleBoost : public Rule {
public:
  RuleBoost(RelayModuleNode *solarRelay, RelayModuleNode *poolRelay);

  const char *getMode() { return "boost"; };

  void setSolarRelayNode(RelayModuleNode *relay) { _solarRelay = relay; };
  void setPoolRelayNode(RelayModuleNode *relay) { _poolRelay = relay; };

  virtual void loop();

protected:
  RelayModuleNode *_solarRelay;
  RelayModuleNode *_poolRelay;

private:
  const char *cCaption = "• RuleBoost:";
  const char *cIndent = "  ◦ ";
};
