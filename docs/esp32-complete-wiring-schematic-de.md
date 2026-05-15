---
title: ESP32 Komplett-Schaltplan (Verdrahtung aller Bauteile)
summary:
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

## 2) Komplett-Schaltplan (Textdarstellung)

```text
+----------------------------------+
|              ESP32               |
|                                  |
| DS18B20 Solar DATA ---- GPIO32   |
| DS18B20 Pool  DATA ---- GPIO33   |
| Relay IN1 (Pool)  ----- GPIO25   |
| Relay IN2 (Solar) ----- GPIO26   |
| RTC SDA --------------- GPIO21   |
| RTC SCL --------------- GPIO22   |
| 3V3 ------------------- 3V3      |
| GND ------------------- GND      |
+----------------+-----------------+
|
| gemeinsame Masse (GND)
v
+----------------------+    +------------------------------+
| DS18B20 Solar        |    | DS18B20 Pool                 |
| VDD -> 3V3           |    | VDD -> 3V3                   |
| GND -> GND           |    | GND -> GND                   |
| DATA -> GPIO32       |    | DATA -> GPIO33               |
+----------+-----------+    +----------+-------------------+
|                           |
+--[4.7kΩ]---> 3V3          +--[4.7kΩ]---> 3V3

+----------------------------------+
| 2-Kanal Relaismodul (5V, opto)   |
| IN1    <- GPIO25                 |
| IN2    <- GPIO26                 |
| VCC    <- 5V (oder Modul-Logik)  |
| JD-VCC <- 5V (falls getrennt)    |
| GND    <- GND                    |
+----------------+-----------------+
|
| Schaltkontakte
v
        Pool-Pumpe / Solar-Pumpe (AC)

+--------------------------------------+
| RTC DS3231 (optional, Uhr)           |
| VCC <- 3V3 (oder 5V je Modultyp)     |
| GND <- GND                           |
| SDA <- GPIO21                        |
| SCL <- GPIO22                        |
+--------------------------------------+
```

## 3) Verdrahtungstabelle

| Bauteil              | Signal       | ESP32 Pin | Zusatzbauteil                  |
| -------------------- | ------------ | --------- | ------------------------------ |
| DS18B20 Solar        | DATA         | GPIO32    | 4.7kΩ Pull-up nach 3V3         |
| DS18B20 Pool         | DATA         | GPIO33    | 4.7kΩ Pull-up nach 3V3         |
| Relais Kanal 1       | IN1          | GPIO25    | optional Pull-down (Fail-Safe) |
| Relais Kanal 2       | IN2          | GPIO26    | optional Pull-down (Fail-Safe) |
| RTC DS3231 (optional) | SDA          | GPIO21    | I2C-Bus                        |
| RTC DS3231 (optional) | SCL          | GPIO22    | I2C-Bus                        |
| Alle Bauteile        | GND          | GND       | gemeinsame Masseführung        |
| DS18B20 / RTC        | Versorgung   | 3V3       | sauber entkoppeln              |
| Relaismodul          | Versorgung   | 5V        | getrennte Last/Logik empfohlen |

## 4) Wichtige Hinweise

1. 230V/Netzseite der Pumpen nur nach geltenden Sicherheitsregeln verdrahten.
2. Sensorleitungen getrennt von Netz- und Relaisleitungen führen.
3. Beim Relaismodul aktive Logik prüfen (active-low oder active-high).
4. Für störarmen Betrieb nahe ESP32 zusätzliche Entkopplung (z. B. 100nF)
    vorsehen.
5. Die Firmware nutzt standardmäßig NTP; das RTC-Modul ist als zusätzliche
    Hardware-Uhr für Erweiterungen/Offline-Szenarien vorgesehen.

## 5) Bezug zu bestehender Doku

- Pin-Hintergrund und Optimierung:
  `docs/esp32-schematic-optimization-de.md`
- Allgemeiner Hardware-Guide:
  `docs/hardware-guide.md`
