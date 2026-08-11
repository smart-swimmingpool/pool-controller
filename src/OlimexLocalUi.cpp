#include "OlimexLocalUi.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include "LocalSettingsMenu.hpp"
#include "OlimexEncoderHandler.hpp"
#include "OlimexTftDisplay.hpp"

namespace PoolController {
namespace {

LocalSettingsMenu menu;

}  // namespace

void OlimexLocalUi::begin() {
  OlimexTftDisplay::begin();
  OlimexEncoderHandler::onEvent([](LocalUiEvent event) {
    menu.handleEvent(event);
    OlimexTftDisplay::requestRedraw();
  });
  OlimexEncoderHandler::begin();
  OlimexTftDisplay::drawPage(menu.currentPage(), menu.currentMenuItem());
}

void OlimexLocalUi::loop() {
  OlimexEncoderHandler::loop();
  if (menu.needsRedraw()) {
    OlimexTftDisplay::requestRedraw();
    menu.clearRedraw();
  }
  OlimexTftDisplay::drawPage(menu.currentPage(), menu.currentMenuItem());
}

void OlimexLocalUi::requestRedraw() {
  OlimexTftDisplay::requestRedraw();
}

}  // namespace PoolController

#else

namespace PoolController {

void OlimexLocalUi::begin() {}
void OlimexLocalUi::loop() {}
void OlimexLocalUi::requestRedraw() {}

}  // namespace PoolController

#endif
