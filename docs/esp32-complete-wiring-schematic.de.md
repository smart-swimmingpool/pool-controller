---
title: ESP32 Komplett-Schaltplan
summary: Vollständiger ESP32-Verdrahtungsplan für den Pool Controller — DS18B20-Temperatursensoren, Relaismodule, optionaler RTC-DS3231, Widerstandsberechnung und Spannungsversorgung
date: "2026-05-15"
lastmod: "2026-05-15"
draft: false
toc: true
type: docs
featured: false
tags: ["docs", "esp32", "hardware", "schaltplan", "verdrahtung"]
menu:
  docs:
    parent: Pool Controller
    name: ESP32 Komplett-Schaltplan
    weight: 26
---

Dieses Dokument zeigt einen zusätzlichen, vollständigen Verdrahtungsplan für
den ESP32-Aufbau mit Sensoren, Relais, Widerständen und Uhr-Modul (RTC).

Die verwendeten Standard-Pins in der Firmware sind:

- `PIN_DS_SOLAR` = GPIO32
- `PIN_DS_POOL` = GPIO33
- `PIN_RELAY_POOL` = GPIO25
- `PIN_RELAY_SOLAR` = GPIO26

## 1) Komponenten (inkl. Uhr und Widerstände)

- 1x ESP32 Dev Board
- 2x DS18B20 Temperatursensor (Solar, Pool)
- 2x Widerstand 4.7kΩ (Pull-up für OneWire-Datenleitungen)
- 1x 2-Kanal-Relaismodul 5V (bevorzugt mit Optokoppler)
- 1x RTC-Modul DS3231 (Uhr, optional)
- 1x Netzteil 5V DC (ausreichend dimensioniert)
- Verdrahtungskabel, Klemmen, ggf. Sicherung/Schutzbeschaltung auf Lastseite

## 2) Komplett-Schaltplan

### Sensoren (DS18B20, 4,7kΩ Pull-up zwischen DATA und 3V3)

Jeder Sensor hat eigene Leitungen für DATA, VDD und GND. Der 4,7kΩ-Widerstand ist die einzige Verbindung zwischen DATA und 3V3.

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

### Relaismodul

Die Relaiskontakte schalten den Pumpenstromkreis (230V AC). Er ist nicht Teil dieser Niederspannungszeichnung, siehe [Elektrische Sicherheit](/docs/electrical-safety/).

```svgbob
 .---------------------.        .--------------------.
 |        ESP32        |        |    Relaismodul     |
 |                     |        |    2 Kanal, 5V     |
 |           GPIO25 o--+--------+--o IN1             |
 |           GPIO26 o--+--------+--o IN2             |
 |               5V o--+--------+--o VCC             |
 |              GND o--+--------+--o GND             |
 '---------------------'        '--------------------'
```

### RTC DS3231 (optional)

Die RTC wird je nach Modultyp mit 3V3 oder 5V versorgt.

```svgbob
 .---------------------.        .--------------------.
 |        ESP32        |        |     RTC DS3231     |
 |                     |        |                    |
 |           GPIO21 o--+--------+--o SDA             |
 |           GPIO22 o--+--------+--o SCL             |
 |              3V3 o--+--------+--o VCC             |
 |              GND o--+--------+--o GND             |
 '---------------------'        '--------------------'
```

## 3) Verdrahtungstabelle

| Bauteil               | Signal     | ESP32 Pin | Zusatzbauteil                  |
| --------------------- | ---------- | --------- | ------------------------------ |
| DS18B20 Solar         | DATA       | GPIO32    | 4.7kΩ Pull-up nach 3V3         |
| DS18B20 Pool          | DATA       | GPIO33    | 4.7kΩ Pull-up nach 3V3         |
| Relais Kanal 1        | IN1        | GPIO25    | optional Pull-down (Fail-Safe) |
| Relais Kanal 2        | IN2        | GPIO26    | optional Pull-down (Fail-Safe) |
| RTC DS3231 (optional) | SDA        | GPIO21    | I2C-Bus                        |
| RTC DS3231 (optional) | SCL        | GPIO22    | I2C-Bus                        |
| Alle Bauteile         | GND        | GND       | gemeinsame Masseführung        |
| DS18B20 / RTC         | Versorgung | 3V3       | sauber entkoppeln              |
| Relaismodul           | Versorgung | 5V        | getrennte Last/Logik empfohlen |

## 4) Wichtige Hinweise

1. 230V/Netzseite der Pumpen nur nach geltenden Sicherheitsregeln verdrahten.
2. Sensorleitungen getrennt von Netz- und Relaisleitungen führen.
3. Beim Relaismodul aktive Logik prüfen (active-low oder active-high).
4. Für störarmen Betrieb nahe ESP32 zusätzliche Entkopplung (z. B. 100nF)
   vorsehen.
5. Die Firmware nutzt standardmäßig NTP; das RTC-Modul ist als zusätzliche
   Hardware-Uhr für Erweiterungen/Offline-Szenarien vorgesehen.

## 5) Bezug zu bestehender Doku

- Ausführliche Aufbauanleitung (auch für Einsteiger):
  [Hardware-Anleitung](hardware-guide.de.md) — Schritt-für-Schritt,
  Fertigungstipps, Fehlersuche
- Pin-Hintergrund und Optimierung:
  [ESP32 Schaltplananalyse und Optimierung](esp32-schematic-optimization.de.md)
- [KiCad-Schaltplan: ESP32 Dev Board](kicad/esp32-dev-board/esp32-dev-board-schematic.pdf) — KiCad-9.0-Schaltplan als PDF-Export
