// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file SystemMonitor.cpp
 * @brief Watchdog loop tick, memory monitoring, reboot-on-critical, and min/max/avg tracking.
 */

#include "SystemMonitor.hpp"

namespace PoolController {

// Static member initialization
uint32_t SystemMonitor::lastMemoryCheck = 0;
uint32_t SystemMonitor::minFreeHeap = 0;
bool SystemMonitor::lowMemoryWarning = false;

}  // namespace PoolController
