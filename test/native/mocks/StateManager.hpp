#pragma once
#include <string>

namespace PoolController {

class StateManager {
public:
  static void begin() {}
  static void loop() {}
  static std::string getState() { return "idle"; }
  static bool isTransitioning() { return false; }
};

} // namespace PoolController
