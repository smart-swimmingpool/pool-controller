// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file SensorSlots.cpp
 * @brief Cross-task temperature slots with consistent snapshots.
 */

#include "SensorSlots.hpp"

#include <cmath>

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#else
#include <mutex>
#endif

namespace PoolController {

namespace {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
portMUX_TYPE slotsMux = portMUX_INITIALIZER_UNLOCKED;
inline void lockSlots() {
  portENTER_CRITICAL(&slotsMux);
}
inline void unlockSlots() {
  portEXIT_CRITICAL(&slotsMux);
}
#else
std::mutex slotsMutex;
inline void lockSlots() {
  slotsMutex.lock();
}
inline void unlockSlots() {
  slotsMutex.unlock();
}
#endif
}  // namespace

SensorSlots::Reading SensorSlots::slots_[static_cast<uint8_t>(SensorId::COUNT)] = {
  {NAN, false},
  {NAN, false},
  {NAN, false},
};

void SensorSlots::reset() {
  lockSlots();
  for (auto &slot : slots_) {
    slot = {NAN, false};
  }
  unlockSlots();
}

void SensorSlots::write(SensorId id, float value, bool found) {
  lockSlots();
  slots_[static_cast<uint8_t>(id)] = {value, found};
  unlockSlots();
}

SensorSlots::Reading SensorSlots::snapshot(SensorId id) {
  lockSlots();
  Reading reading = slots_[static_cast<uint8_t>(id)];
  unlockSlots();
  return reading;
}

}  // namespace PoolController
