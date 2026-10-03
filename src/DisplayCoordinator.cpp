// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

/**
 * @file DisplayCoordinator.cpp
 * @brief Serialized NORVI button/UI state and OLED rendering on Core 1.
 */

#include "DisplayCoordinator.hpp"

#ifdef NORVI_AE01_R

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "ConfigManager.hpp"
#include "NetworkManager.hpp"
#include "Nodes.hpp"
#include "NorviButtonHandler.hpp"
#include "NorviOledDisplay.hpp"

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

StaticSemaphore_t stateMutexStorage;
SemaphoreHandle_t stateMutex = nullptr;

void enqueueEvent(UiEvent event) {
  if (eventCount >= EVENT_CAPACITY) {
    Serial.println("⚠ DisplayCoordinator: UI event queue full — dropping event");
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
      const String &currentMode = operationModeNode.getMode();
      if (currentMode == "auto") {
        operationModeNode.setMode("manu");
      } else if (currentMode == "manu") {
        operationModeNode.setMode("boost");
      } else if (currentMode == "boost") {
        operationModeNode.setMode("timer");
      } else {
        operationModeNode.setMode("auto");
      }
      Serial.printf("→ Mode switched to: %s\n", operationModeNode.getMode().c_str());
      break;
    }
    case NorviOledDisplay::MenuItem::PUMP:
      poolPumpNode.setSwitch(!poolPumpNode.getSwitch());
      Serial.printf("→ Pump toggled: %s\n", poolPumpNode.getSwitch() ? "ON" : "OFF");
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
  if (!NorviOledDisplay::isMappingComplete()) {
    return;
  }

  uint8_t solarAddr[8];
  uint8_t poolAddr[8];
  NorviOledDisplay::getMapping(solarAddr, poolAddr);
  ConfigManager::saveSensorMapping(solarAddr, poolAddr);
  Serial.println("→ Sensor mapping saved — rebooting...");
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
  stateMutex = xSemaphoreCreateMutexStatic(&stateMutexStorage);
  if (stateMutex == nullptr) {
    Serial.println("✖ DisplayCoordinator: failed to create state mutex");
    return;
  }

  NorviOledDisplay::begin();
  NorviButtonHandler::begin();

  NorviButtonHandler::onButton1Press([]() { enqueueEvent(UiEvent::BUTTON_1); });
  NorviButtonHandler::onButton2Press([]() { enqueueEvent(UiEvent::BUTTON_2); });
  NorviButtonHandler::onButton3Press([]() { enqueueEvent(UiEvent::BUTTON_3); });
  NorviButtonHandler::onButton3LongPress([]() -> bool {
    enqueueEvent(UiEvent::BUTTON_3_LONG);
    return true;
  });
}

void DisplayCoordinator::loop() {
  // Button sampling, state transitions and rendering all remain on Core 1.
  // This preserves the single-writer rule for operationModeNode, relay state,
  // NetworkManager and ConfigManager which are read while drawing pages.
  NorviButtonHandler::loop();

  if (stateMutex == nullptr || xSemaphoreTake(stateMutex, 0) != pdTRUE) {
    return;
  }

  UiEvent event;
  while (dequeueEvent(event)) {
    processEvent(event);
  }
  NorviOledDisplay::update();
  NorviOledDisplay::render();

  xSemaphoreGive(stateMutex);
}

void DisplayCoordinator::render() {
  if (stateMutex == nullptr) {
    return;
  }

  // Kept as a serialized compatibility entry point. CoreScheduler no longer
  // creates a separate DisplayTask, so production rendering happens in loop().
  if (xSemaphoreTake(stateMutex, portMAX_DELAY) == pdTRUE) {
    NorviOledDisplay::render();
    xSemaphoreGive(stateMutex);
  }
}

}  // namespace PoolController

#else

namespace PoolController {
void DisplayCoordinator::begin() {}
void DisplayCoordinator::loop() {}
void DisplayCoordinator::render() {}
}  // namespace PoolController

#endif
