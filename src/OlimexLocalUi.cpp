#include "OlimexLocalUi.hpp"

#if defined(OLIMEX_ESP32_C6_EVB) && defined(HAS_LOCAL_TFT_UI)

#include "LocalSettingsMenu.hpp"
#include "OlimexEncoderHandler.hpp"
#include "OlimexTftDisplay.hpp"
#include "LogCapture.hpp"
#include "Nodes.hpp"

namespace PoolController {
namespace {

LocalSettingsMenu menu;
std::uint32_t lastLiveRedrawMs{0};

bool isLivePage(LocalUiPage page) {
  return page == LocalUiPage::OVERVIEW || page == LocalUiPage::NETWORK || page == LocalUiPage::SYSTEM;
}

}  // namespace

namespace {

CommandDispatcher commandDispatcher = nullptr;

void executePendingAction(LocalMenuAction action) {
  ControllerCommand command{};
  command.source = CommandSource::LOCAL_UI;
  switch (action) {
  case LocalMenuAction::CYCLE_MODE:
    command.type = ControllerCommandType::CYCLE_MODE;
    break;
  case LocalMenuAction::TOGGLE_PUMP:
    command.type = ControllerCommandType::TOGGLE_POOL_PUMP;
    command.toggleModePolicy = PumpToggleModePolicy::ENTER_MANUAL;
    break;
  case LocalMenuAction::NONE:
    return;
  }
  if (dispatchCommand(commandDispatcher, command) != CommandStatus::APPLIED) {
    LOG_WARN("Local UI: menu action was not applied\n");
  }
}

}  // namespace

void OlimexLocalUi::setCommandDispatcher(CommandDispatcher dispatcher) {
  commandDispatcher = dispatcher;
}

void OlimexLocalUi::begin() {
  OlimexTftDisplay::begin();
  lastLiveRedrawMs = millis();
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
  const auto now = millis();
  if (isLivePage(menu.currentPage()) && (now - lastLiveRedrawMs >= 1000UL)) {
    lastLiveRedrawMs = now;
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
void OlimexLocalUi::setCommandDispatcher(CommandDispatcher) {}

}  // namespace PoolController

#endif
