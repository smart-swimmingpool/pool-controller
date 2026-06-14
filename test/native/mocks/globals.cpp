/**
 * @file globals.cpp
 * @brief PoolController namespace global node instances for native tests.
 *
 * Uses MOCK headers (same mock .hpp files that tests and production wrappers
 * include) so that PoolController:: objects have mock-compatible layout.
 * The mock constructors are trivial (no OneWire/DallasTemperature hardware
 * init) and all methods accessed from production wrappers are provided
 * inline by the mock headers with safe (no out-of-bounds) implementations.
 *
 * globals.cpp is compiled as part of MOCK_SOURCES — same include path as
 * test files and production wrappers.
 */

#include "Arduino.h"
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"

namespace PoolController {

DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", 0, 300);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", 1, 300);
ESP32TemperatureNode ctrlTemperatureNode("ctrl-temp", "Controller Temperature");
RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", 25);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", 26);
OperationModeNode operationModeNode("operation-mode", "Operation Mode", 300);

} // namespace PoolController
