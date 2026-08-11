#include "OlimexEncoderHandler.hpp"

#if defined(OLIMEX_ESP32_C6_EVB)

#include "Config.hpp"
#include "Ky040Decoder.hpp"

#include <Arduino.h>

namespace PoolController {
namespace {

Ky040Decoder decoder;
constexpr std::uint32_t SAMPLE_INTERVAL_MS{2};
std::uint32_t lastSampleMs{0};

}  // namespace

OlimexEncoderHandler::EventCallback OlimexEncoderHandler::eventCallback_{};

void OlimexEncoderHandler::begin() {
  pinMode(PIN_ENCODER_CLK, INPUT_PULLUP);
  pinMode(PIN_ENCODER_DT, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
}

void OlimexEncoderHandler::loop() {
  const std::uint32_t nowMs = millis();
  if (nowMs - lastSampleMs < SAMPLE_INTERVAL_MS) {
    return;
  }
  lastSampleMs = nowMs;

  const bool clkHigh = digitalRead(PIN_ENCODER_CLK) == HIGH;
  const bool dtHigh = digitalRead(PIN_ENCODER_DT) == HIGH;
  const bool swPressed = digitalRead(PIN_ENCODER_SW) == LOW;
  const LocalUiEvent event = decoder.update(clkHigh, dtHigh, swPressed, nowMs);
  if (event != LocalUiEvent::NONE && eventCallback_) {
    eventCallback_(event);
  }
}

}  // namespace PoolController

#endif
