// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file main.cpp
 * @brief Main entry point and composition root for the Pool Controller firmware.
 *
 * Arduino entry point: setup() initializes all subsystems, loop() runs
 * the control, monitoring, and network stack indefinitely.
 */

#include <Arduino.h>

#include "ControllerCommandHandler.hpp"
#include "ControllerSnapshotPublisher.hpp"
#include "ControllerSnapshotStore.hpp"
#include "LogCapture.hpp"
#include "MqttRuntimeCommandAdapter.hpp"
#include "Nodes.hpp"
#include "PoolController.hpp"
#include "WebPortal.hpp"

/** @brief Singleton context owning the existing controller lifecycle. */
static PoolController::PoolControllerContext context{};

namespace {

/**
 * Construct application services only after the legacy global nodes completed
 * static initialization. Function-local statics avoid cross-TU initialization
 * order dependencies while keeping ownership explicit at the composition root.
 */
PoolController::ControllerCommandHandler &controllerCommandHandler() {
  static PoolController::ControllerCommandHandler::Dependencies dependencies{PoolController::operationModeNode,
    PoolController::poolPumpNode, PoolController::solarPumpNode, PoolController::solarTemperatureNode,
    PoolController::poolTemperatureNode, PoolController::ctrlTemperatureNode};
  static PoolController::ControllerCommandHandler handler(dependencies);
  return handler;
}

bool dispatchWebCommand(const PoolController::ControllerCommand &command, PoolController::ControllerCommandResult *result) {
  return controllerCommandHandler().handle(command, result);
}

PoolController::MqttRuntimeCommandAdapter &mqttRuntimeCommandAdapter() {
  static PoolController::MqttRuntimeCommandAdapter adapter(controllerCommandHandler());
  return adapter;
}

PoolController::ControllerSnapshotStore &controllerSnapshotStore() {
  static PoolController::ControllerSnapshotStore store;
  return store;
}

PoolController::ControllerSnapshotPublisher &controllerSnapshotPublisher() {
  static PoolController::ControllerSnapshotPublisher::Dependencies dependencies{PoolController::operationModeNode,
    PoolController::poolPumpNode, PoolController::solarPumpNode, PoolController::solarTemperatureNode,
    PoolController::poolTemperatureNode, PoolController::ctrlTemperatureNode, controllerSnapshotStore()};
  static PoolController::ControllerSnapshotPublisher publisher(dependencies);
  return publisher;
}

}  // namespace

/**
 * @brief Arduino setup() — initializes serial and delegates to PoolControllerContext.
 *
 * Waits up to 3 seconds for a USB serial connection (non-blocking fallback
 * for headless operation), then calls context.setup() to initialize all
 * subsystems and installs the typed MQTT runtime-command adapter.
 */
auto setup() -> void {
  Serial.begin(SERIAL_SPEED);

  // Wait for serial port to connect. Needed for native USB port only.
  // F29: Avoid infinite blocking when USB is not connected (e.g. native USB boards running headless).
  const uint32_t startWait = millis();
  while (!Serial && (millis() - startWait < 3000)) {
    delay(10);
  }

  // Central logging service — must start before context.setup() so boot-time
  // LOG_* entries (WiFi/MQTT init, sensor scans) are captured and the MQTT
  // export watermark sees the pre-boot sequence. Serial is already up here.
  PoolController::LogCapture::begin();

  PoolController::WebPortal::setCommandDispatcher(dispatchWebCommand);
  context.setup();

  // MqttPublisher::begin() is part of context.setup(). Replace its callback
  // afterwards so new runtime mutations flow through typed commands. The
  // adapter callback only copies bytes; mutation happens in loop().
  mqttRuntimeCommandAdapter().begin();

  // Publish a first immutable read model after setup established runtime state.
  controllerSnapshotPublisher().publish();
}

/**
 * @brief Arduino loop() — runs typed adapter hand-over and controller stack.
 *
 * Runtime MQTT commands are drained on the same Arduino/Core-1 loop task that
 * owns mutable controller state. After the legacy controller loop completes,
 * one immutable read model is published for outbound adapters.
 */
auto loop() -> void {
  mqttRuntimeCommandAdapter().processPendingCommands();
  context.loop();
  controllerSnapshotPublisher().publish();
}
