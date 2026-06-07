// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file RelayModuleNode.hpp
 * @brief Relay node — wraps a GPIO pin as a relay switch with state persistence.
 */

#pragma once

#include <Arduino.h>
#include <RelayModule.h>
#include <Preferences.h>
#include <memory>

/**
 * @brief Manages a single relay via a GPIO pin (active-high logic).
 *
 * Persists relay state to NVS so it survives reboots. Respects safe mode
 * (boot-loop protection) by refusing ON transitions while in safe mode.
 */
class RelayModuleNode {
public:
  /**
   * @brief Construct a relay node.
   * @param id  Unique node identifier (e.g. "pool-pump").
   * @param name  Human-readable name (e.g. "Pool Pump").
   * @param pin  GPIO pin number for the relay control signal.
   * @param measurementInterval  Minimum interval between status logs (seconds).
   */
  RelayModuleNode(const char *id, const char *name, const uint8_t pin, const int measurementInterval = MEASUREMENT_INTERVAL);
  ~RelayModuleNode() = default;

  /** @brief Get the node identifier. */
  const char *getId() const { return _id; }
  /** @brief Get the GPIO pin number. */
  uint8_t getPin() const { return _pin; }

  /** @brief Set the minimum interval between status log outputs. */
  void setMeasurementInterval(unsigned long interval) { _measurementInterval = interval; }
  /** @brief Get the current status-log interval. */
  unsigned long getMeasurementInterval() const { return _measurementInterval; }

  /**
   * @brief Set the relay state (ON/OFF).
   * @param state  true = relay ON, false = relay OFF.
   * @note In safe mode (boot-loop detected), ON requests are silently ignored.
   */
  void setSwitch(const bool state);
  /** @brief Get the current relay state. @return true if relay is energized (ON). */
  bool getSwitch();

  /** @brief Initialize the relay: restore persisted state, configure GPIO. */
  void begin();
  /** @brief Periodic status logging. */
  void loop();

private:
  static const int MIN_INTERVAL = 10;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;

  const char *_id;
  const char *_name;
  uint8_t _pin;
  unsigned long _measurementInterval;
  unsigned long _lastMeasurement;

  std::unique_ptr<RelayModule> relay;
  Preferences preferences;
};
