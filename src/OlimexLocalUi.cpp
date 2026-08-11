#include "OlimexLocalUi.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include "LocalSettingsMenu.hpp"
#include "OlimexEncoderHandler.hpp"
#include "OlimexTftDisplay.hpp"
#include "Nodes.hpp"

namespace PoolController {
namespace {

LocalSettingsMenu menu;

}  // namespace

namespace {

void cycleOperationMode() {
  const String currentMode = operationModeNode.getMode();
  if (currentMode == "auto") {
    operationModeNode.setMode("manu");
  } else if (currentMode == "manu") {
    operationModeNode.setMode("boost");
  } else if (currentMode == "boost") {
    operationModeNode.setMode("timer");
  } else {
    operationModeNode.setMode("auto");
  }
}

void executePendingAction(LocalMenuAction action) {
  switch (action) {
  case LocalMenuAction::CYCLE_MODE:
    cycleOperationMode();
    break;
  case LocalMenuAction::TOGGLE_PUMP:
    if (operationModeNode.getMode() != "manu") {
      operationModeNode.setMode("manu");
    }
    poolPumpNode.setSwitch(!poolPumpNode.getSwitch());
    break;
  case LocalMenuAction::NONE:
    break;
  }
}

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
  const auto pendingAction = menu.consumePendingAction();
  if (pendingAction != LocalMenuAction::NONE) {
    executePendingAction(pendingAction);
    OlimexTftDisplay::requestRedraw();
  }
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
