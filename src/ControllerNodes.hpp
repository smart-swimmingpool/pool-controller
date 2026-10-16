// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file ControllerNodes.hpp
 * @brief The legacy runtime nodes that application services read or mutate.
 */

#pragma once

class DallasTemperatureNode;
class ESP32TemperatureNode;
class OperationModeNode;
class RelayModuleNode;

namespace PoolController {

/** @brief Explicit legacy runtime dependencies supplied by the composition root. */
struct ControllerNodes final {
  OperationModeNode &operationMode;
  RelayModuleNode &poolPump;
  RelayModuleNode &solarPump;
  DallasTemperatureNode &solarTemperature;
  DallasTemperatureNode &poolTemperature;
  ESP32TemperatureNode &controllerTemperature;
};

}  // namespace PoolController
