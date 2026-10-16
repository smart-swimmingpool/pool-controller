// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file DallasAddressLookup.hpp
 * @brief Find a DS18B20 on a OneWire bus by its ROM address.
 *
 * Header-only and templated on the bus type so the lookup can be tested
 * natively with a fake bus.
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace PoolController {

constexpr size_t kDallasAddressSize = 8;

/**
 * @brief Search the bus for the device whose ROM address equals @p target.
 * @param bus         Object with `bool getAddress(uint8_t *addr, uint8_t index)`.
 * @param deviceCount Number of devices currently on the bus.
 * @param target      Address to look for.
 * @param[out] index  Bus index of the match (only set on success).
 * @return true if the address is on the bus. Never falls back to an index.
 */
template <typename Bus> bool findDallasDeviceByAddress(Bus &bus, uint8_t deviceCount, const uint8_t *target, uint8_t &index) {
  for (uint8_t i = 0; i < deviceCount; i++) {
    uint8_t addr[kDallasAddressSize];
    if (bus.getAddress(addr, i) && memcmp(addr, target, kDallasAddressSize) == 0) {
      index = i;
      return true;
    }
  }
  return false;
}

}  // namespace PoolController
