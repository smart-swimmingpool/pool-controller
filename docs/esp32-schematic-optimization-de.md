---
title: ESP32 Schaltplananalyse und Optimierung (Sensoren & Relais)
summary:
date: "2026-05-14"
lastmod: "2026-05-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "esp32", "hardware", "sensoren", "relais"]
menu:
  docs:
    parent: Pool Controller
    name: ESP32 Schaltplan-Optimierung
    weight: 25
---

# ESP32 Schaltplananalyse und Optimierung (Sensoren & Relais)

Dieses Dokument analysiert den vorhandenen Aufbau (Fritzing-Quelle + bestehende
Hardware-Dokumentation) und beschreibt einen optimierten ESP32-Vorschlag für
Sensoren und Relais.

## 1) Ist-Analyse (relevante Signale)

Aktuelle Firmware-Pinbelegung für ESP32 (`src/Config.hpp`):

- `PIN_DS_SOLAR` = GPIO15
- `PIN_DS_POOL` = GPIO16
- `PIN_RELAY_POOL` = GPIO18
- `PIN_RELAY_SOLAR` = GPIO19

Verwendete Hardware laut Doku:

- 2x DS18B20 Temperaturfühler
- 2-Kanal Relaismodul (5V)

## 2) Hauptthemen im aktuellen Aufbau

1. **DS18B20 auf GPIO15 (Strapping-Pin)**

   - GPIO15 ist beim ESP32 ein Boot-Strapping-Pin.
   - Ein OneWire-Bus mit Pull-up auf diesem Pin kann das Boot-Verhalten negativ
     beeinflussen.

2. **Relais-/Versorgungskopplung**

   - 5V-Relaismodule können Schaltstörungen verursachen (Spikes/Noise).
   - Ohne saubere Trennung von Logik- und Lastversorgung steigt das Risiko von
     Resets und Messrauschen.

3. **Fehlende explizite Fail-Safe-Definition im Schaltplan**
   - Beim Start oder Reset darf keine Pumpe unbeabsichtigt einschalten.

## 3) Optimierter ESP32-Vorschlag

### 3.1 Empfohlene Pinbelegung

| Funktion      | Alt (ESP32) | Empfehlung | Grund                              |
| ------------- | ----------- | ---------- | ---------------------------------- |
| DS18B20 Solar | GPIO15      | GPIO32     | Vermeidet Strapping-Pins           |
| DS18B20 Pool  | GPIO16      | GPIO33     | Klare Trennung der Sensorleitungen |
| Relais Pool   | GPIO18      | GPIO25     | Robuster Output, konfliktarm       |
| Relais Solar  | GPIO19      | GPIO26     | Robuster Output, konfliktarm       |

Hinweis: Bei bereits aufgebauter Hardware kann die alte Belegung bleiben. Die
Empfehlung zielt auf neue/überarbeitete ESP32-Installationen.

### 3.2 Sensor-Optimierung (DS18B20)

- Pro Sensorleitung einen sauberen **4.7k Pull-up nach 3.3V** vorsehen.
- Sensorleitungen räumlich von Relais-/230V-Leitungen trennen.
- Bei längeren Leitungen: verdrillte Leitung und gemeinsamer sauberer GND.
- 100nF nahe ESP32-Versorgung zur Entstörung vorsehen.

### 3.3 Relais-Optimierung

- **Relaismodul mit Optokoppler** und sauber dokumentierter Logikpegelrichtung
  (active-low/active-high) verwenden.
- Wenn Modul es unterstützt: **JD-VCC (Relais-Spule) von VCC (Logik) trennen**
  und gemeinsame Masseführung sternförmig auslegen.
- Auf Pumpenseite geeignete Netzschutzmaßnahmen einplanen (z. B. passende
  Absicherung/Schutzbeschaltung entsprechend Installation).
- Relais-Eingänge zusätzlich mit Pull-up/Pull-down fail-safe auslegen, damit
  beim Booten kein ungewolltes Schalten erfolgt.

## 4) Firmware-Bezug bei Umverdrahtung

Bei Übernahme der empfohlenen Pins müssen in `src/Config.hpp` die
ESP32-Konstanten angepasst werden:

- `PIN_DS_SOLAR`
- `PIN_DS_POOL`
- `PIN_RELAY_POOL`
- `PIN_RELAY_SOLAR`

## 5) Verifikations-Checkliste nach Umbau

- Bootet der ESP32 reproduzierbar (kalt/warm) ohne Fehlstart?
- Schalten Relais beim Boot **nicht** unbeabsichtigt?
- Sind Temperaturmessungen stabil, auch bei Relais-Schaltvorgängen?
- Keine Resets bei parallelem WLAN + Relaisbetrieb?

## 6) Betrieb und Sicherheit

Für den produktiven 24/7-Betrieb und Sicherheitsaspekte bitte zusätzlich
`AGENTS.md` beachten (insbesondere Ressourcenmanagement, Security, OTA und
Robustheit).
