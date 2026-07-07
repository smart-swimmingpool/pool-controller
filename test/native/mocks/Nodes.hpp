// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file Nodes.hpp
 * @brief Mock shadow of src/Nodes.hpp for the native test harness.
 *
 * src/MqttPublisher.cpp includes "Nodes.hpp" which lives only in src/. When
 * compiled from test/native/build/wrappers/ (see CMakeLists.txt), a quoted
 * #include resolved from *inside* src/Nodes.hpp would resolve relative to
 * src/ first — bypassing the mocks/ shadow for DallasTemperatureNode.hpp,
 * ESP32TemperatureNode.hpp, OperationModeNode.hpp, and RelayModuleNode.hpp,
 * and causing "redefinition of class" errors when the real headers are
 * pulled in alongside the mock ones.
 *
 * This mock mirrors src/Nodes.hpp's extern declarations using the mock node
 * headers instead, keeping the wrapper's mocks-shadow-production strategy
 * intact.
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
