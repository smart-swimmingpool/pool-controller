// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file RuleManu.hpp
 * @brief Manual mode rule — user controls pumps directly.
 */

#pragma once

#include "Rule.hpp"

/**
 * @brief Manual operation mode — pumps are controlled directly by the user.
 *
 * No automatic logic: the user sets each pump ON or OFF explicitly via
 * the Web UI, MQTT, or the REST API.
 */
class RuleManu : public Rule {
public:
  RuleManu();

  const char *getMode() { return "manu"; };

  virtual void loop();
};
