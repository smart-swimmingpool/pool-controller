// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file Nodes.hpp
 * @brief Central extern declarations for global node instances.
 *
 * All global temperature sensor, relay, and operation mode nodes are
 * defined in PoolController.cpp. This header provides a single point
 * for their extern declarations so that every translation unit that
 * needs them can #include this file instead of repeating extern lines.
 */

#pragma once

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "OperationModeNode.hpp"
#include "RelayModuleNode.hpp"

namespace PoolController {

// ── Temperature sensors ───────────────────────────────────────────────
extern DallasTemperatureNode solarTemperatureNode;
extern DallasTemperatureNode poolTemperatureNode;
extern ESP32TemperatureNode ctrlTemperatureNode;

// ── Relays ────────────────────────────────────────────────────────────
extern RelayModuleNode poolPumpNode;
extern RelayModuleNode solarPumpNode;

// ── Operation mode ────────────────────────────────────────────────────
extern OperationModeNode operationModeNode;

}  // namespace PoolController
