#pragma once

#include <cstdint>

#include "LocalUiTypes.hpp"

namespace PoolController {

class Ky040Decoder {
public:
  LocalUiEvent update(bool clkHigh, bool dtHigh, bool swPressed, std::uint32_t nowMs);

private:
  static constexpr std::uint32_t LONG_PRESS_MS{2000};
  std::uint8_t lastState_{0x03};
  std::int8_t delta_{0};
  bool buttonWasPressed_{false};
  std::uint32_t buttonPressStartMs_{0};
};

}  // namespace PoolController
