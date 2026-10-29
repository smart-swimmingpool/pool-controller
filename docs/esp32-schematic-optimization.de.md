---
title: ESP32 Schaltplananalyse und Optimierung
summary: ESP32 Schaltplan-Optimierung für Sensoren (DS18B20, DHT22) und Relais-Ansteuerung — GPIO-Analyse, Pull-Up-Widerstände, Störungsvermeidung und Stromversorgung
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

Dieses Dokument analysiert den vorhandenen Aufbau (Fritzing-Quelle + bestehende
Hardware-Dokumentation) und beschreibt einen optimierten ESP32-Vorschlag für
Sensoren und Relais.

## 1) Ist-Analyse (relevante Signale)

Aktuelle Firmware-Pinbelegung für ESP32 (`src/Config.hpp`):

- `PIN_DS_SOLAR` = GPIO32
- `PIN_DS_POOL` = GPIO33
- `PIN_RELAY_POOL` = GPIO25
- `PIN_RELAY_SOLAR` = GPIO26

Verwendete Hardware laut Doku:

- 2x DS18B20 Temperaturfühler
- 2-Kanal Relaismodul (5V)

## 2) Hauptthemen im bisherigen Aufbau (vor Optimierung)

1. **DS18B20 war auf GPIO15 (Strapping-Pin)**

   GPIO15 ist beim ESP32 ein Boot-Strapping-Pin.
   Ein OneWire-Bus mit Pull-up auf diesem Pin kann das Boot-Verhalten negativ
   beeinflussen.

1. **Relais-/Versorgungskopplung**

   5V-Relaismodule können Schaltstörungen verursachen (Spikes/Noise).
   Ohne saubere Trennung von Logik- und Lastversorgung steigt das Risiko von
   Resets und Messrauschen.

1. **Fehlende explizite Fail-Safe-Definition im Schaltplan**

   Beim Start oder Reset darf keine Pumpe unbeabsichtigt einschalten.

## 3) Optimierter ESP32-Vorschlag

### 3.1 Empfohlene Pinbelegung

| Funktion      | Vorher (ESP32) | Jetzt (ESP32) | Grund                              |
| ------------- | -------------- | ------------- | ---------------------------------- |
| DS18B20 Solar | GPIO15         | GPIO32        | Vermeidet Strapping-Pins           |
| DS18B20 Pool  | GPIO16         | GPIO33        | Klare Trennung der Sensorleitungen |
| Relais Pool   | GPIO18         | GPIO25        | Robuster Output, konfliktarm       |
| Relais Solar  | GPIO19         | GPIO26        | Robuster Output, konfliktarm       |

Hinweis: Für bestehende Installationen mit alter Verdrahtung ist eine Anpassung
der Firmware-Pins und ggf. Umverdrahtung erforderlich.

### 3.1.1 Entscheidungsdokumentation (klarer Entscheid)

**Entscheidung:** Für ESP32 werden die Signale dauerhaft auf
`GPIO32/33` (DS18B20) sowie `GPIO25/26` (Relais) geführt.

**Warum genau diese Zuordnung:**

1. **Boot-Robustheit:** Sensoren liegen nicht mehr auf Strapping-Pins
   (insbesondere kein OneWire auf GPIO15).
2. **Störarmut:** Sensor-GPIOs sind von Relais-GPIOs logisch getrennt; dadurch
   weniger gegenseitige Beeinflussung bei Schaltvorgängen.
3. **Betriebssicherheit:** Relais auf gut nutzbaren Output-Pins mit klarer
   Fail-Safe-Auslegung beim Start/Reset.
4. **Wartbarkeit:** Einheitliche, dokumentierte Standardbelegung in Firmware und
   Hardware-Doku reduziert Fehlverdrahtung und Support-Aufwand.

**Nicht gewählt wurde:** Die frühere ESP32-Belegung (`15/16/18/19`), da sie
höheres Boot-/Störrisiko mitbringt.

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

### 3.4 Verdrahtung (Visualisierung)

```svgbob
 .---------------------.             .--------------------.
 |        ESP32        |             |   DS18B20 Solar    |
 |                     |             |                    |
 |           GPIO32 o--+----*--------+--o DATA            |
 |                     |    |        |                    |
 |                     | .--+--.     |                    |
 |                     | |4.7k |     |                    |
 |                     | '--+--'     |                    |
 |                     |    |        |                    |
 |              3V3 o--+----*--------+--o VDD             |
 |                     |             |                    |
 |              GND o--+-------------+--o GND             |
 '---------------------'             '--------------------'
```
```svgbob
 .---------------------.             .--------------------.
 |        ESP32        |             |    DS18B20 Pool    |
 |                     |             |                    |
 |           GPIO33 o--+----*--------+--o DATA            |
 |                     |    |        |                    |
 |                     | .--+--.     |                    |
 |                     | |4.7k |     |                    |
 |                     | '--+--'     |                    |
 |                     |    |        |                    |
 |              3V3 o--+----*--------+--o VDD             |
 |                     |             |                    |
 |              GND o--+-------------+--o GND             |
 '---------------------'             '--------------------'
```

```svgbob
 .---------------------.        .--------------------.
 |        ESP32        |        |    Relaismodul     |
 |                     |        |   2 Kanal, Opto    |
 |           GPIO25 o--+--------+--o IN1             |
 |           GPIO26 o--+--------+--o IN2             |
 |              3V3 o--+--------+--o VCC             |
 |               5V o--+--------+--o JD-VCC          |
 |              GND o--+--------+--o GND             |
 '---------------------'        '--------------------'
```

Die Logikversorgung VCC hängt vom Relaismodul ab (3V3 oder 5V, siehe Datenblatt).
Den JD-VCC-Jumper entfernen, damit die Spulen getrennt aus 5V versorgt werden.

## 4) Firmware-Bezug bei Umverdrahtung

Die empfohlenen Pins sind in `src/Config.hpp` umgesetzt. Relevant sind:

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

## 7) Siehe auch

- Ausführliche Aufbauanleitung mit Schritt-für-Schritt-Anleitung und
  Fertigungstipps: [Hardware-Anleitung](hardware-guide.de.md)
- Kompletter Verdrahtungsplan mit RTC-Modul:
  [ESP32 Komplett-Schaltplan](esp32-complete-wiring-schematic.de.md)
