// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

// Mock statics this suite links without the full test_runner captures.

#include <map>
#include <set>
#include <string>

#include "Arduino.h"

uint32_t g_millisOffset = 0;
std::string SerialClass::s_capture;
bool SerialClass::s_captureEnabled = false;
std::map<std::string, std::string> Preferences::s_data;
