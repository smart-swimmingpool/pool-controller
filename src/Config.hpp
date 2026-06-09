// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file Config.hpp
 * @brief Pin assignments and compile-time constants for the Pool Controller.
 *
 * All GPIO pin assignments and tunable constants are centralized here.
 * Changing these values requires rebuilding the firmware.
 *
 * ## Optimierte Pin-Belegung
 *
 * Gegenüber den ursprünglichen Pins (GPIO15/16/18/19) wird hier die
 * in der Hardware-Dokumentation empfohlene optimierte Belegung verwendet:
 *
 *   | Funktion      | Alt  | Optimiert | Grund                                          |
 *   |---------------|:----:|:---------:|------------------------------------------------|
 *   | DS18B20 Solar | GPIO15 | **GPIO32** | GPIO15 ist Strapping-Pin — Risiko eliminiert   |
 *   | DS18B20 Pool  | GPIO16 | **GPIO33** | Saubere Trennung vom Strapping-Pin GPIO0       |
 *   | Relais Pool   | GPIO18 | **GPIO25** | ADC2-Pins (18/19) vermieden; sauberer Digital  |
 *   | Relais Solar  | GPIO19 | **GPIO26** | Gleicher Grund wie oben                        |
 *
 * Siehe docs/hardware-guide.de.md für Details.
 *
 * Die Status-LED verwendet den bord eigenen LED_BUILTIN (meist GPIO2 auf
 * ESP32 DevKit) und ist damit modellunabhängig. Eine zweite Warn-LED
 * kann optional über PIN_LED_WARN aktiviert werden.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief Interval for temperature sensor updates (seconds).
 *
 * @note SERIAL_SPEED (baud rate) is provided as a compiler macro from
 *       platformio.ini build flags, not defined here.
 */

constexpr std::uint8_t TEMP_READ_INTERVAL{30};

// ── Optimierte Pin-Belegung ────────────────────────────────────────────────

/** @brief DS18B20 data pin — solar collector temperature sensor (optimiert: GPIO32 statt 15). */
constexpr std::uint8_t PIN_DS_SOLAR{32};
/** @brief DS18B20 data pin — pool water temperature sensor (optimiert: GPIO33 statt 16). */
constexpr std::uint8_t PIN_DS_POOL{33};
/** @brief Relay control pin — pool circulation pump (optimiert: GPIO25 statt 18). */
constexpr std::uint8_t PIN_RELAY_POOL{25};
/** @brief Relay control pin — solar heating pump (optimiert: GPIO26 statt 19). */
constexpr std::uint8_t PIN_RELAY_SOLAR{26};

// ── Status-LED (modellunabhängig via LED_BUILTIN) ──────────────────────────

/** @brief Status-LED-Pin (built-in). Beim ESP32 DevKit i. d. R. GPIO2.
 *
 *  Modellunabhängiger Default. Falls die Platform LED_BUILTIN definiert
 *  (z. B. abweichendes Board wie TTGO T-Display), wird der Wert in
 *  StatusLed::begin() überschrieben — Config.hpp selbst bleibt lightweight.
 */
constexpr std::uint8_t PIN_LED_STATUS{2};

/** @brief Optionale zweite Warn-LED (z. B. für Safe-Mode). -1 = deaktiviert. */
constexpr std::int8_t PIN_LED_WARN{-1};

}  // namespace PoolController
