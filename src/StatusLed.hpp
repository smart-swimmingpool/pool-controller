// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file StatusLed.hpp
 * @brief Status-LED mit Homie-konformen Blink-Codes für Netzwerk-Status.
 *
 * Die LED signalisiert den Systemzustand nach Homie-Convention:
 *
 *   Pattern         | Blink           | Bedeutung
 *   ----------------|-----------------|----------
 *   AP_MODE         | 100ms ⬜/100ms ⬛ | Kein WLAN konfiguriert / AP-Modus
 *   CONNECTING      | 500ms ⬜/500ms ⬛ | WLAN-Verbindung läuft
 *   CONNECTED_NO_MQTT| 1500ms ⬜/500ms ⬛ | WLAN OK, MQTT getrennt
 *   ONLINE          | Dauerhaft ⬜      | WLAN + MQTT verbunden
 *   OTA_UPDATE      | 50ms ⬜/50ms ⬛   | Firmware-Update läuft
 *   SAFE_MODE       | ⬜200⬛200⬜200⬛600 | Boot-Loop / Kritischer Fehler
 *
 * @note Der Pin wird automatisch an LED_BUILTIN angepasst, falls die
 *       Platform dies definiert. Bei Static-Analyzer-Builds ohne echte
 *       Hardware (z. B. test_desktop) wird StatusLed::loop() zum No-Op.
 *
 * Siehe https://homieiot.github.io/ für die Homie-Convention.
 */

#pragma once

#include <cstdint>

namespace PoolController {

/**
 * @brief LED-Blink-Pattern nach Homie-Convention.
 */
enum class StatusLedPattern : std::uint8_t {
  OFF = 0,            ///< LED aus (Stromsparmodus / deaktiviert)
  ONLINE,             ///< Dauerhaft an — alles verbunden
  CONNECTING,         ///< Langsames Blinken — WLAN-Verbindung läuft
  CONNECTED_NO_MQTT,  ///< Meist an, kurzes Blinken — WLAN OK, MQTT getrennt
  AP_MODE,            ///< Schnelles Blinken — AP-Modus aktiv
  OTA_UPDATE,         ///< Sehr schnelles Blinken — Firmware-Update
  SAFE_MODE,          ///< Doppel-Blink — Boot-Loop / Safe Mode
};

/**
 * @brief Status-LED mit zeitgesteuerten Blink-Mustern.
 *
 * Singleton mit statischen Methoden. Wird in PoolController::setup()
 * initialisiert und in loop() getaktet.
 *
 * @code
 * // In PoolController::setup()
 * StatusLed::begin();
 *
 * // In PoolController::loop()
 * StatusLed::setPattern(StatusLedPattern::ONLINE);
 * StatusLed::loop();
 * @endcode
 */
class StatusLed {
public:
  /** @brief GPIO initialisieren und LED ausschalten. */
  static void begin();

  /**
   * @brief LED-Zustand aktualisieren (muss regelmäßig in loop() aufgerufen
   *        werden, damit die Blink-Timing stimmt).
   */
  static void loop();

  /** @brief Neues Blink-Pattern setzen. */
  static void setPattern(StatusLedPattern pattern);

  /** @brief Aktuell aktives Pattern abfragen. */
  static StatusLedPattern getCurrentPattern();

private:
  /**
   * @brief Berechnet den gewünschten LED-Zustand (HIGH/LOW) für den
   *        aktuellen Zeitpunkt anhand des aktiven Patterns.
   *
   * Verwendet modulo-basierte Timing-Tabelle — kein Zustandsmaschinen-
   * Overhead.
   */
  static bool computeDesiredState(uint32_t nowMs);

  /// Aktuell aktives Blink-Pattern.
  static StatusLedPattern currentPattern_;

  /// Gemerkter physikalischer Zustand der LED (dient der Erkennung von
  /// Änderungen, um digitale Schreibzugriffe zu minimieren).
  static bool lastOutputState_;

  /// GPIO-Pin-Nummer der Haupt-LED.
  static uint8_t ledPin_;

  /// GPIO-Pin-Nummer der optionalen zweiten LED (-1 = deaktiviert).
  static int8_t warnPin_;

  // ── Timing-Konstanten (in Millisekunden) ─────────────────────────────

  /// AP-Modus: 100ms an, 100ms aus = 5 Hz
  static constexpr uint32_t T_AP_ON{100};
  static constexpr uint32_t T_AP_OFF{100};

  /// WLAN-Verbindung: 500ms an, 500ms aus = 1 Hz
  static constexpr uint32_t T_CONN_ON{500};
  static constexpr uint32_t T_CONN_OFF{500};

  /// WLAN OK, MQTT getrennt: meist an, kurzer „Herzschlag"-Aus
  static constexpr uint32_t T_NOMQTT_ON{1500};
  static constexpr uint32_t T_NOMQTT_OFF{500};

  /// OTA-Update: sehr schnell 50ms/50ms = 10 Hz
  static constexpr uint32_t T_OTA_ON{50};
  static constexpr uint32_t T_OTA_OFF{50};

  /// Safe-Mode Doppel-Blink (siehe computeDesiredState)
  static constexpr uint32_t T_SAFE_CYCLE{1200};
  static constexpr uint32_t T_SAFE_PHASE1_END{200};   // ON 0–200ms
  static constexpr uint32_t T_SAFE_PHASE2_END{400};   // OFF 200–400ms
  static constexpr uint32_t T_SAFE_PHASE3_END{600};   // ON 400–600ms
  // rest 600–1200ms = OFF
};

}  // namespace PoolController
