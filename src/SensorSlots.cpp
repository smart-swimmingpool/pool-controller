// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorSlots.cpp
 * @brief Lock-free temperature slot implementation.
 */

#include "SensorSlots.hpp"

#include <cmath>

namespace PoolController {

SensorSlots::Slot SensorSlots::slots_[static_cast<uint8_t>(SensorId::COUNT)] = {
  {NAN, false},
  {NAN, false},
  {NAN, false},
};

void SensorSlots::reset() {
  for (auto &slot : slots_) {
    slot.value = NAN;
    slot.found = false;
  }
}

void SensorSlots::write(SensorId id, float value, bool found) {
  Slot &slot = slots_[static_cast<uint8_t>(id)];
  slot.value = value;
  slot.found = found;
}

float SensorSlots::read(SensorId id) {
  return slots_[static_cast<uint8_t>(id)].value;
}

bool SensorSlots::isFound(SensorId id) {
  return slots_[static_cast<uint8_t>(id)].found;
}

}  // namespace PoolController
