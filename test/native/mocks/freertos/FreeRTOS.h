// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

// Minimal FreeRTOS surface for compiling task launchers in native tests.
#pragma once

#include <cstdint>

typedef int BaseType_t;
typedef unsigned int UBaseType_t;

#ifndef PRO_CPU_NUM
#define PRO_CPU_NUM 0
#endif
#ifndef APP_CPU_NUM
#define APP_CPU_NUM 1
#endif
