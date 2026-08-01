// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "CoreScheduler.hpp"

namespace PoolController {
uint8_t CoreScheduler::sensorPriority = 0;
uint16_t CoreScheduler::sensorStack = 0;

void CoreScheduler::begin() {
  sensorPriority = TASK_PRIORITY_SENSOR;
  sensorStack = TASK_STACK_SENSOR;
}

void CoreScheduler::logStackWatermarks() {}
}  // namespace PoolController
