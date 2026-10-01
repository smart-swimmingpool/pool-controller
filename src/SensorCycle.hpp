// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorCycle.hpp
 * @brief One DS18B20 measurement cycle for both bus topologies.
 *
 * - Shared bus (NORVI AE01-R): solar is the bus master and its
 *   beginMeasurement() starts one conversion for both sensors; the pool
 *   node's beginMeasurement() is a no-op.
 * - Dedicated buses (esp32dev): each node starts the conversion on its own
 *   bus, so both beginMeasurement() calls are required. Both conversions run
 *   in parallel during the single wait.
 *
 * Templated so the scheduling contract is covered by native tests with fake
 * nodes for both topologies.
 */

#pragma once

namespace PoolController {

template <typename Node, typename WaitFn> void runDallasMeasurementCycle(Node &solar, Node &pool, WaitFn waitForConversion) {
  solar.beginMeasurement();
  pool.beginMeasurement();
  waitForConversion();
  solar.finishMeasurement();
  pool.finishMeasurement();
}

}  // namespace PoolController
