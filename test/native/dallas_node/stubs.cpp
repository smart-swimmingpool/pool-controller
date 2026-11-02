// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file stubs.cpp
 * @brief Stand-ins for the production dependencies DallasTemperatureNode
 *        links against but this isolated binary does not build for real.
 */

#include <map>
#include <string>

#include "Arduino.h"
#include "DegradationManager.hpp"
#include "Preferences.h"

uint32_t g_millisOffset = 0;
std::map<std::string, std::string> Preferences::s_data;
std::string SerialClass::s_capture;
bool SerialClass::s_captureEnabled = false;
uint32_t ESPClass::s_mockFreeHeap = 180000;
int ESPClass::s_restartCount = 0;

namespace PoolController {
// Health reporting is not under test here.
void DegradationManager::reportSensorStatus(const char *, bool) {}
}  // namespace PoolController
