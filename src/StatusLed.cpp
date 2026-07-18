// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file StatusLed.cpp
 * @brief Status-LED-Implementierung — Homie-konforme Blink-Codes für den
 *        Netzwerk- und Systemzustand.
 *
 * Verwendet eine modulo-basierte Timing-Logik ohne millis()-Blocking.
 * Der Pin wird beim Start automatisch auf LED_BUILTIN gesetzt, falls die
 * Platform dies definiert (modellunabhängig).
 */

#include "StatusLed.hpp"

#include <Arduino.h>
#include "Config.hpp"

namespace PoolController {

// ── Statische Member ───────────────────────────────────────────────────────

StatusLedPattern StatusLed::currentPattern_ = StatusLedPattern::OFF;
bool StatusLed::lastOutputState_ = false;
uint8_t StatusLed::ledPin_ = PIN_LED_STATUS;
int8_t StatusLed::warnPin_ = PIN_LED_WARN;

// ── Initialisierung ────────────────────────────────────────────────────────

void StatusLed::begin() {
  // Reload from config — handles board-specific overrides (e.g. NORVI GPIO27).
  ledPin_ = PIN_LED_STATUS;

#ifdef LED_BUILTIN
  // Override Config.hpp default only when it uses the generic value (GPIO2),
  // meaning no board-specific config set a different pin.  This lets NORVI
  // (external LED on GPIO27) keep its explicit assignment while standard
  // ESP32 dev boards still get LED_BUILTIN (typically also GPIO2).
  if (PIN_LED_STATUS == 2) {
    ledPin_ = static_cast<uint8_t>(LED_BUILTIN);
    Serial.printf("• StatusLed using LED_BUILTIN (GPIO %d)\n", ledPin_);
  } else {
    Serial.printf("• StatusLed using config pin GPIO %d (board-specific, LED_BUILTIN overridden)\n",
                  ledPin_);
  }
#else
  Serial.printf("• StatusLed using config default GPIO %d (no LED_BUILTIN)\n", ledPin_);
#endif

  pinMode(ledPin_, OUTPUT);
  digitalWrite(ledPin_, LOW);
  lastOutputState_ = false;
  currentPattern_ = StatusLedPattern::OFF;

  if (warnPin_ >= 0) {
    pinMode(static_cast<uint8_t>(warnPin_), OUTPUT);
    digitalWrite(static_cast<uint8_t>(warnPin_), LOW);
    Serial.printf("• StatusLed WARN pin enabled on GPIO %d\n", warnPin_);
  }

  Serial.println("✓ StatusLed initialized");
}

// ── Pattern setzen ─────────────────────────────────────────────────────────

void StatusLed::setPattern(StatusLedPattern pattern) {
  currentPattern_ = pattern;
}

StatusLedPattern StatusLed::getCurrentPattern() {
  return currentPattern_;
}

// ── Loop (muss regelmäßig aufgerufen werden) ───────────────────────────────

void StatusLed::loop() {
  const uint32_t now = millis();
  const bool desiredState = computeDesiredState(now);

  // Nur schreiben, wenn sich der Zustand ändert (vermeidet unnötige
  // GPIO-Traffic und flackern).
  if (desiredState != lastOutputState_) {
    lastOutputState_ = desiredState;
    digitalWrite(ledPin_, desiredState ? HIGH : LOW);
  }
}

// ── Timing-Logik (modulo-basiert, zustandslos) ────────────────────────

bool StatusLed::computeDesiredState(const uint32_t nowMs) {
  switch (currentPattern_) {
  case StatusLedPattern::ONLINE:
    return true;  // dauerhaft an

  case StatusLedPattern::OFF:
    return false;  // dauerhaft aus

  case StatusLedPattern::AP_MODE: {
    // 100ms an / 100ms aus = 5 Hz
    const uint32_t cycle = T_AP_ON + T_AP_OFF;
    return (nowMs % cycle) < T_AP_ON;
  }

  case StatusLedPattern::CONNECTING: {
    // 500ms an / 500ms aus = 1 Hz
    const uint32_t cycle = T_CONN_ON + T_CONN_OFF;
    return (nowMs % cycle) < T_CONN_ON;
  }

  case StatusLedPattern::CONNECTED_NO_MQTT: {
    // 1500ms an / 500ms aus — „Herzschlag"-Muster
    const uint32_t cycle = T_NOMQTT_ON + T_NOMQTT_OFF;
    return (nowMs % cycle) < T_NOMQTT_ON;
  }

  case StatusLedPattern::OTA_UPDATE: {
    // 50ms an / 50ms aus = 10 Hz
    const uint32_t cycle = T_OTA_ON + T_OTA_OFF;
    return (nowMs % cycle) < T_OTA_ON;
  }

  case StatusLedPattern::SAFE_MODE: {
    // Doppel-Blink: 200ms an, 200ms aus, 200ms an, 600ms aus
    // Gesamtzyklus: 1200ms
    const uint32_t phase = nowMs % T_SAFE_CYCLE;
    return (phase < T_SAFE_PHASE1_END) || (phase >= T_SAFE_PHASE2_END && phase < T_SAFE_PHASE3_END);
  }
  }

  return false;  // Fallback — Sicherheitshalber aus
}

}  // namespace PoolController
