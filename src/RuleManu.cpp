// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file RuleManu.cpp
 * @brief Manual mode rule — no automatic pump logic.
 */

#include "RuleManu.hpp"
#include <Arduino.h>
#include "LogCapture.hpp"

RuleManu::RuleManu() {}

void RuleManu::loop() {
  // no ruling if manual
  LOG_INFO("  ◦ § RuleManu: loop\n");
}
