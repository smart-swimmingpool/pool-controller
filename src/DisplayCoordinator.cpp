// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayCoordinator.cpp
 * @brief Serialized NORVI button/UI state and OLED rendering on Core 1.
 */

#include "DisplayCoordinator.hpp"

#ifdef NORVI_AE01_R

#include <Arduino.h>

#include "CalibrationManager.hpp"
#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "NorviButtonHandler.hpp"
#include "NorviOledDisplay.hpp"
#include "LogCapture.hpp"

namespace PoolController {
namespace {

enum class UiEvent : uint8_t {
  BUTTON_1,
  BUTTON_2,
  BUTTON_3,
  BUTTON_3_LONG,
};

constexpr size_t EVENT_CAPACITY = 8;
UiEvent eventQueue[EVENT_CAPACITY]{};
size_t eventHead = 0;
size_t eventCount = 0;

void enqueueEvent(UiEvent event) {
  if (eventCount >= EVENT_CAPACITY) {
    LOG_WARN("⚠ DisplayCoordinator: UI event queue full — dropping event\n");
    return;
  }
  eventQueue[(eventHead + eventCount) % EVENT_CAPACITY] = event;
  eventCount++;
}

bool dequeueEvent(UiEvent &event) {
  if (eventCount == 0) {
    return false;
  }
  event = eventQueue[eventHead];
  eventHead = (eventHead + 1) % EVENT_CAPACITY;
  eventCount--;
  return true;
}

void handleButton1() {
  if (NorviOledDisplay::isMenuActive()) {
    NorviOledDisplay::menuPrevious();
  } else if (NorviOledDisplay::isSelectSensorStep()) {
    NorviOledDisplay::setupSelectPrevious();
    NorviOledDisplay::requestRedraw();
  } else if (NorviOledDisplay::isSelectRoleStep()) {
    NorviOledDisplay::setupSelectSolar();
    NorviOledDisplay::requestRedraw();
  } else {
    NorviOledDisplay::previousPage();
  }
}

void handleButton2() {
  if (NorviOledDisplay::isMenuActive()) {
    NorviOledDisplay::menuNext();
  } else if (NorviOledDisplay::isSelectSensorStep()) {
    NorviOledDisplay::setupSelectNext();
    NorviOledDisplay::requestRedraw();
  } else if (NorviOledDisplay::isSelectRoleStep()) {
    NorviOledDisplay::setupSelectPool();
    NorviOledDisplay::requestRedraw();
  } else {
    NorviOledDisplay::nextPage();
  }
}

void handleButton3() {
  if (NorviOledDisplay::isMenuActive()) {
    switch (NorviOledDisplay::getMenuSelection()) {
    case NorviOledDisplay::MenuItem::MODE: {
      constexpr const char *kSource = "button:S3/menu";
      const String &currentMode = operationModeNode.getMode();
      if (currentMode == "auto") {
        operationModeNode.setMode("manu", kSource);
      } else if (currentMode == "manu") {
        operationModeNode.setMode("boost", kSource);
      } else if (currentMode == "boost") {
        operationModeNode.setMode("timer", kSource);
      } else {
        operationModeNode.setMode("auto", kSource);
      }
      LOG_INFO("→ Mode switched to: %s\n", operationModeNode.getMode().c_str());
      break;
    }
    case NorviOledDisplay::MenuItem::PUMP:
      poolPumpNode.setSwitch(!poolPumpNode.getSwitch());
      LOG_INFO("→ Pump toggled: %s\n", poolPumpNode.getSwitch() ? "ON" : "OFF");
      break;
    case NorviOledDisplay::MenuItem::EXIT:
      break;
    }
    NorviOledDisplay::exitMenu();
  } else if (NorviOledDisplay::getCurrentPage() == NorviOledDisplay::Page::MAIN) {
    NorviOledDisplay::enterMenu();
  } else if (NorviOledDisplay::getCurrentPage() == NorviOledDisplay::Page::SENSOR_SETUP) {
    NorviOledDisplay::confirmAction();
  }
}

void handleButton3Long() {
  uint8_t solarAddr[8];
  uint8_t poolAddr[8];
  NorviOledDisplay::getMapping(solarAddr, poolAddr);
  ConfigManager::saveSensorMapping(solarAddr, poolAddr);
  LOG_INFO("→ Sensor mapping saved — rebooting...\n");
  NetworkManager::restart();
}

void processEvent(UiEvent event) {
  switch (event) {
  case UiEvent::BUTTON_1:
    handleButton1();
    break;
  case UiEvent::BUTTON_2:
    handleButton2();
    break;
  case UiEvent::BUTTON_3:
    handleButton3();
    break;
  case UiEvent::BUTTON_3_LONG:
    handleButton3Long();
    break;
  }
}

}  // namespace

void DisplayCoordinator::begin() {
  NorviOledDisplay::begin();
  NorviButtonHandler::begin();
  CalibrationManager::begin();

  NorviButtonHandler::onButton1Press([]() { enqueueEvent(UiEvent::BUTTON_1); });
  NorviButtonHandler::onButton2Press([]() { enqueueEvent(UiEvent::BUTTON_2); });
  NorviButtonHandler::onButton3Press([]() { enqueueEvent(UiEvent::BUTTON_3); });
  // The callback's return value decides synchronously whether the short press
  // is suppressed, so the mapping check cannot be deferred to the event queue.
  NorviButtonHandler::onButton3LongPress([]() -> bool {
    if (!NorviOledDisplay::isMappingComplete()) {
      return false;
    }
    enqueueEvent(UiEvent::BUTTON_3_LONG);
    return true;
  });
}

void DisplayCoordinator::loop() {
  // Button sampling, state transitions and rendering all run on the Arduino
  // control-loop task. This preserves single-writer ownership of mutable UI,
  // operation-mode, relay, network and configuration state.
  NorviButtonHandler::loop();
  CalibrationManager::loop();

  UiEvent event;
  while (dequeueEvent(event)) {
    processEvent(event);
  }

  NorviOledDisplay::update();
  NorviOledDisplay::render();
}

}  // namespace PoolController

#else

namespace PoolController {
void DisplayCoordinator::begin() {}
void DisplayCoordinator::loop() {}
}  // namespace PoolController

#endif
