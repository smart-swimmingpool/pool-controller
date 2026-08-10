// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file Config.hpp
 * @brief Pin assignments and compile-time constants for the Pool Controller.
 *
 * All GPIO pin assignments and tunable constants are centralized here.
 * Changing these values requires rebuilding the firmware.
 *
 * ## Hardware Variants
 *
 * Two hardware configurations are supported:
 *
 *   **Standard** — ESP32 Dev Board (default)
 *     Uses the optimized pin assignment documented in the hardware guides:
 *     DS18B20 on GPIO32/33, relays on GPIO25/26.
 *
 *   **NORVI AE01-R** — define the preprocessor macro `NORVI_AE01_R`
 *     Industrial ESP32-WROOM32 controller with built-in relays, OLED display,
 *     buttons, and 24V DC supply. See docs/norvi-ae01-r.md for details.
 *
 * ## Standard Pin Assignment (optimierte Belegung)
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

// ═══════════════════════════════════════════════════════════════════════════
// NORVI IIOT-AE01-R — Industrial ESP32 Controller
// ═══════════════════════════════════════════════════════════════════════════
//
// Build with:  pio run -e norvi_ae01_r
//
// Pin mapping derived from the official datasheet:
//   https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/
//   https://norvi.io/docs/norvi-iiot-ae01-r-user-guide/
//
// Key differences from a standard ESP32 dev board:
//   • Supply voltage: 24V DC (no 5V USB needed)
//   • 6 built-in SPST relays (we use Relay 0 & 1)
//   • 0.96" SSD1306 OLED on I2C (GPIO16/GPIO17)
//   • 3 front-panel buttons via analog ADC (GPIO32)
//   • No built-in status LED — use Transistor Output 0.1 (GPIO27)
//   • Digital inputs (GPIO18/19/21/22/23/34/35/39) are INPUT-ONLY
//     (optocoupler-isolated for 24V signals) — NOT usable for OneWire
// ═══════════════════════════════════════════════════════════════════════════

#ifdef NORVI_AE01_R

/** @brief DS18B20 data pin — both sensors share a single bus on Expansion Port Pin 1. */
constexpr std::uint8_t PIN_DS_SOLAR{25};
/** @brief DS18B20 data pin — shared bus with solar sensor on GPIO25 (Expansion Port Pin 1). */
constexpr std::uint8_t PIN_DS_POOL{25};
/** @brief Relay control pin — pool circulation pump (Relay Output 0). */
constexpr std::uint8_t PIN_RELAY_POOL{14};
/**
 * @brief Relay control pin — solar heating pump.
 *
 * Moved from Relay Output 1 (GPIO12) to Relay Output 5 (GPIO33): a field
 * unit's R1 channel was found to be permanently conducting (relay audibly
 * clicks on setSwitch(), but COM1/NO1 never opens) — a hardware fault
 * (welded/fused contact or NO/NC miswiring), not a firmware issue: R0 uses
 * the identical RelayModuleNode logic and switches correctly. R5/GPIO33 is
 * otherwise unused on NORVI builds.
 *
 * @note When changing the relay output for the solar pump (e.g. from R1 to R5),
 *       the NVS state ("solar-pump"/"switch") from the previous relay pin
 *       carries over and may cause the new relay to energize at boot. The
 *       RelayModuleNode::begin() safe-start sequence mitigates this by forcing
 *       the GPIO to OFF before enabling output, then transitioning to the
 *       persisted state. If the pump runs despite the controller showing OFF
 *       after a pin change, clear NVS manually:
 *         pio run -e norvi_ae01_r -t exec -- prefs erase solar-pump
 *       Or add a one-time NVS clear by calling preferences.clear() on the
 *       "solar-pump" namespace during the migration boot.
 */
constexpr std::uint8_t PIN_RELAY_SOLAR{33};
static_assert(PIN_RELAY_SOLAR != 12,
  "PIN_RELAY_SOLAR must not be reverted to GPIO12 (Relay Output 1) without "
  "confirming the field hardware fault is resolved — see the doc comment "
  "above and docs/norvi-ae01-r.md for context. R1 was found permanently "
  "conducting (welded/fused contact or NO/NC miswiring) on at least one "
  "field unit; Relay Output 0 uses identical firmware logic and works.");
/** @brief Status LED — external LED via transistor output 0.1 (open-collector, 100 mA max). */
constexpr std::uint8_t PIN_LED_STATUS{27};
/** @brief Optional warning LED — not used on NORVI. */
constexpr std::int8_t PIN_LED_WARN{-1};

// ── NORVI-specific peripheral pins ─────────────────────────────────────────

/** @brief I2C SDA pin for the built-in 0.96" SSD1306 OLED display. */
constexpr std::uint8_t PIN_OLED_SDA{16};
/** @brief I2C SCL pin for the built-in 0.96" SSD1306 OLED display. */
constexpr std::uint8_t PIN_OLED_SCL{17};
/** @brief Analog input pin for the three front-panel buttons. */
constexpr std::uint8_t PIN_BUTTON_ADC{32};

#elif defined(OLIMEX_ESP32_C6_EVB)

/** @brief DS18B20 data pin — solar collector temperature sensor. */
constexpr std::uint8_t PIN_DS_SOLAR{6};
/** @brief DS18B20 data pin — pool water temperature sensor. */
constexpr std::uint8_t PIN_DS_POOL{20};
/** @brief Relay control pin — pool circulation pump (Olimex relay). */
constexpr std::uint8_t PIN_RELAY_POOL{10};
/** @brief Relay control pin — solar heating pump (Olimex relay). */
constexpr std::uint8_t PIN_RELAY_SOLAR{11};
/** @brief Status LED — Olimex user LED. */
constexpr std::uint8_t PIN_LED_STATUS{8};
/** @brief Optional warning LED — not used on Olimex. */
constexpr std::int8_t PIN_LED_WARN{-1};

// ── Olimex local UI pins ────────────────────────────────────────────────────
constexpr std::uint8_t PIN_TFT_MOSI{18};
constexpr std::uint8_t PIN_TFT_SCLK{19};
constexpr std::uint8_t PIN_TFT_MISO{20};
constexpr std::uint8_t PIN_TFT_CS{21};
constexpr std::uint8_t PIN_TFT_DC{7};
constexpr std::int8_t PIN_TFT_RST{-1};  // -1 = display reset tied high
constexpr std::int8_t PIN_TFT_BACKLIGHT{-1};  // -1 = fixed 3.3 V backlight

constexpr std::uint8_t PIN_ENCODER_CLK{4};
constexpr std::uint8_t PIN_ENCODER_DT{5};
constexpr std::uint8_t PIN_ENCODER_SW{0};

constexpr std::uint16_t TFT_DISPLAY_WIDTH{320};
constexpr std::uint16_t TFT_DISPLAY_HEIGHT{240};
constexpr bool TFT_DISPLAY_SIZE_CLASS_COMPACT{false};
constexpr bool TFT_DRIVER_ILI9341{true};
constexpr bool TFT_DRIVER_ST7789{false};

// ═══════════════════════════════════════════════════════════════════════════
// Standard ESP32 Dev Board (default)
// ═══════════════════════════════════════════════════════════════════════════

#else

/** @brief DS18B20 data pin — solar collector temperature sensor (optimiert: GPIO32 statt 15). */
constexpr std::uint8_t PIN_DS_SOLAR{32};
/** @brief DS18B20 data pin — pool water temperature sensor (optimiert: GPIO33 statt 16). */
constexpr std::uint8_t PIN_DS_POOL{33};
/** @brief Relay control pin — pool circulation pump (optimiert: GPIO25 statt 18). */
constexpr std::uint8_t PIN_RELAY_POOL{25};
/** @brief Relay control pin — solar heating pump (optimiert: GPIO26 statt 19). */
constexpr std::uint8_t PIN_RELAY_SOLAR{26};

/** @brief Status-LED-Pin (built-in). Beim ESP32 DevKit i. d. R. GPIO2.
 *
 *  Modellunabhängiger Default. Falls die Platform LED_BUILTIN definiert
 *  (z. B. abweichendes Board wie TTGO T-Display), wird der Wert in
 *  StatusLed::begin() überschrieben — Config.hpp selbst bleibt lightweight.
 */
constexpr std::uint8_t PIN_LED_STATUS{2};

/** @brief Optionale zweite Warn-LED (z. B. für Safe-Mode). -1 = deaktiviert. */
constexpr std::int8_t PIN_LED_WARN{-1};

#endif

#if defined(OLIMEX_ESP32_C6_EVB)
static_assert(TFT_DISPLAY_WIDTH == 320, "Olimex local UI expects a 320 px wide display");
static_assert(TFT_DISPLAY_HEIGHT == 240, "Olimex local UI expects a 240 px high display");
static_assert(TFT_DRIVER_ILI9341 != TFT_DRIVER_ST7789, "Select exactly one TFT driver");
static_assert(PIN_TFT_RST < 0 || PIN_TFT_RST != PIN_TFT_CS, "TFT reset pin must not conflict with TFT CS");
static_assert(PIN_TFT_BACKLIGHT < 0 || PIN_TFT_BACKLIGHT != PIN_TFT_CS, "Backlight control pin must not conflict with TFT CS");
#endif

}  // namespace PoolController
