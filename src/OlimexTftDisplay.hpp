#pragma once

#include "LocalUiTypes.hpp"

namespace PoolController {

class OlimexTftDisplay {
 public:
  static void begin();
  static void drawPage(LocalUiPage page, LocalMenuItem menuItem);
  static void requestRedraw() { forceRedraw_ = true; }

 private:
  static void drawOverview();
  static void drawMenu(LocalMenuItem menuItem);
  static void drawNetwork();
  static void drawSystem();
  static void drawQrCode();
  static bool forceRedraw_;
};

}  // namespace PoolController
