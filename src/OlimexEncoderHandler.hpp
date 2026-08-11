#pragma once

#include <functional>

#include "LocalUiTypes.hpp"

namespace PoolController {

class OlimexEncoderHandler {
public:
  using EventCallback = std::function<void(LocalUiEvent)>;

  static void begin();
  static void loop();
  static void onEvent(EventCallback cb) { eventCallback_ = cb; }

private:
  static EventCallback eventCallback_;
};

}  // namespace PoolController
