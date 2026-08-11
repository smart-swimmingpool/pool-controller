#include "Ky040Decoder.hpp"

namespace PoolController {

LocalUiEvent Ky040Decoder::update(bool clkHigh, bool dtHigh, bool swPressed, std::uint32_t nowMs) {
  const std::uint8_t state = static_cast<std::uint8_t>((clkHigh ? 0x02 : 0x00) | (dtHigh ? 0x01 : 0x00));
  const std::uint8_t transition = static_cast<std::uint8_t>((lastState_ << 2U) | state);
  lastState_ = state;

  switch (transition) {
    case 0b1110:
    case 0b1000:
    case 0b0001:
    case 0b0111:
      --delta_;
      break;
    case 0b1101:
    case 0b0100:
    case 0b0010:
    case 0b1011:
      ++delta_;
      break;
    default:
      break;
  }

  if (state == 0x03 && delta_ >= 4) {
    delta_ = 0;
    return LocalUiEvent::ROTATE_CLOCKWISE;
  }
  if (state == 0x03 && delta_ <= -4) {
    delta_ = 0;
    return LocalUiEvent::ROTATE_COUNTER_CLOCKWISE;
  }

  if (swPressed && !buttonWasPressed_) {
    buttonWasPressed_ = true;
    buttonPressStartMs_ = nowMs;
    return LocalUiEvent::NONE;
  }

  if (!swPressed && buttonWasPressed_) {
    buttonWasPressed_ = false;
    const std::uint32_t heldMs = nowMs - buttonPressStartMs_;
    return heldMs >= LONG_PRESS_MS ? LocalUiEvent::LONG_PRESS : LocalUiEvent::SHORT_PRESS;
  }

  return LocalUiEvent::NONE;
}

}  // namespace PoolController
