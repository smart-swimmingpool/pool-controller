// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>

namespace PoolController {

enum class LocalUiEvent : std::uint8_t {
  NONE = 0,
  ROTATE_CLOCKWISE,
  ROTATE_COUNTER_CLOCKWISE,
  SHORT_PRESS,
  LONG_PRESS,
};

enum class LocalUiPage : std::uint8_t {
  OVERVIEW = 0,
  NETWORK,
  SYSTEM,
  QRCODE,
  MENU,
};

enum class LocalMenuItem : std::uint8_t {
  MODE = 0,
  PUMP,
  NETWORK_STATUS,
  QR_CODE,
  EXIT,
};

enum class LocalMenuAction : std::uint8_t {
  NONE = 0,
  CYCLE_MODE,
  TOGGLE_PUMP,
};

enum class DisplayDriver : std::uint8_t {
  ILI9341 = 0,
  ST7789,
};

enum class DisplaySizeClass : std::uint8_t {
  COMPACT = 0,
  NORMAL,
};

struct TftDisplayConfig {
  std::uint16_t width;
  std::uint16_t height;
  DisplayDriver driver;
  DisplaySizeClass sizeClass;
};

}  // namespace PoolController
