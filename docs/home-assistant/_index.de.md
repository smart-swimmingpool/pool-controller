---
title: Home Assistant Integration
summary: Pool Controller Home Assistant Integration \u2014 automatische MQTT Discovery Entit\u00e4ten, Sensor/Switch/Number/Select/Time-Dom\u00e4nen, Lovelace Dashboard YAML, Migration von alten Konfigurationen
date: "2026-06-06"
lastmod: "2026-06-06"
draft: false
toc: true
type: docs
menu:
  docs:
    parent: Pool Controller
    name: Home Assistant
    weight: 50
---

Der Pool Controller integriert sich nahtlos in [Home Assistant](https://home-assistant.io) \u00fcber **MQTT
Discovery** (Standard-Protokoll). Alle Entit\u00e4ten werden automatisch registriert, sobald der Controller mit
dem MQTT-Broker verbunden ist.

## MQTT Discovery

Der Controller publiziert [HA MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)-Payloads
beim Start und bei jeder MQTT-Wiederherstellung. Keine manuelle MQTT-Konfiguration n\u00f6tig \u2014 die Ger\u00e4te
erscheinen automatisch in Home Assistant.

### Verf\u00fcgbare Entit\u00e4ten

| Domain          | Object ID                | Kategorie  | Beschreibung                           |
| --------------- | ------------------------ | ---------- | -------------------------------------- |
| `sensor`        | `pool_temperature`       | \u2014     | Wassertemperatur Pool                  |
| `sensor`        | `solar_temperature`      | \u2014     | Temperatur Solarkollektor              |
| `sensor`        | `controller_temperature` | diagnostic | ESP32-Chip-Temperatur                  |
| `sensor`        | `free_heap_space`        | diagnostic | Freier Heap-Speicher                   |
| `sensor`        | `max_alloc_block`        | diagnostic | Gr\u00f6\u00dfter allozierbarer Block  |
| `sensor`        | `wifi_signal_strength`   | diagnostic | WiFi-Signalst\u00e4rke (dBm)           |
| `sensor`        | `system_uptime`          | diagnostic | Betriebszeit (Dauer)                   |
| `sensor`        | `effective_runtime`      | diagnostic | Effektive Filterlaufzeit (Dauer)       |
| `sensor`        | `local_time`             | diagnostic | Aktuelle Ortszeit                      |
| `binary_sensor` | `pool_sensor_found`      | diagnostic | Status Pool-Sensor (gefunden/fehlt)    |
| `binary_sensor` | `solar_sensor_found`     | diagnostic | Status Solar-Sensor (gefunden/fehlt)   |
| `binary_sensor` | `mqtt_status`            | diagnostic | MQTT-Verbindungsstatus                 |
| `select`        | `mode`                   | \u2014     | Betriebsart (auto/manu/boost/timer)    |
| `select`        | `pool_sensor`            | config     | Pool-Sensor-Adresszuordnung            |
| `select`        | `solar_sensor`           | config     | Solar-Sensor-Adresszuordnung           |
| `select`        | `timezone`               | config     | Zeitzonenauswahl                       |
| `switch`        | `pool_pump`              | \u2014     | Pool-Umw\u00e4lzpumpe                  |
| `switch`        | `solar_pump`             | \u2014     | Solar-Heizungspumpe                    |
| `number`        | `pool_max_temp`          | config     | Zieltemperatur Pool max.               |
| `number`        | `solar_min_temp`         | config     | Minimale Solar-Aktivierungstemperatur  |
| `number`        | `hysteresis`             | config     | Temperaturhysterese                    |
| `number`        | `temp_circ_threshold`    | config     | Schwellwert temp. Filterlaufzeit       |
| `number`        | `temp_circ_factor`       | config     | Faktor temp. Filterlaufzeit            |
| `number`        | `temp_circ_max_runtime`  | config     | Maximale Laufzeit temp. Filterlaufzeit |
| `time`          | `timer_start`            | config     | Timer Startzeit (HH:MM)                |
| `time`          | `timer_end`              | config     | Timer Endzeit (HH:MM)                  |
| `text`          | `ntp_server`             | config     | NTP-Server-Adresse                     |
| `update`        | `firmware`               | config     | Firmware-Update-Entit\u00e4t           |
| `climate`       | `pool_climate`           | \u2014     | Pool-Thermostat mit Preset-Modi        |

> **Entity-IDs** in HA werden aus der MQTT unique_id generiert und enthalten einen ger\u00e4tespezifischen
> MAC-Suffix (z.B. `sensor.pool_controller_a1b2c3_pool_temperature`). Pr\u00fcfe **Entwickler-Tools \u2192 Entit\u00e4ten** und
> filtere nach "pool" um deine IDs zu finden. Ersetze `pool_controller` im Dashboard-YAML durch deinen
> tats\u00e4chlichen Prefix.

## Lovelace Dashboard

Eine vorgefertigte Lovelace-Dashboard-Konfiguration liegt in
[`dashboard.yaml`](dashboard.yaml) bereit.

### Funktionen

Zwei zielgruppenspezifische Ansichten:

- **\ud83c\udfca Pool-Ansicht** \u2014 f\u00fcr den Bademeister (t\u00e4gliche Bedienung): Temperaturanzeigen, Modus-Umschaltung (mit Hervorhebung), Timer, Pumpensteuerung, Klima-Thermostat, temperatureabh\u00e4ngige Filterlaufzeit, 24h-Verlauf mit Controller-Temperatur
- **\u2699 System-Ansicht** \u2014 f\u00fcr den IoT-Entwickler (Diagnose & Konfiguration): Zeitzone & NTP, Sensor-Zuordnung (DS18B20-Adressauswahl), Systemdiagnose (Heap, WiFi, Betriebszeit, Controller-Temperatur, effektive Laufzeit), Firmware-Updates

### Einrichtung

1. Kopiere die YAML-Datei in dein HA-Konfigurationsverzeichnis
2. HA \u00f6ffnen \u2192 **Dashboard \u2192 Bearbeiten \u2192 Drei-Punkte \u2192 Raw-Konfigurationseditor**
3. Inhalt einf\u00fcgen
