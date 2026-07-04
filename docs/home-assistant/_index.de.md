---
title: Home Assistant Integration
summary: Pool Controller Home Assistant Integration — automatische MQTT Discovery Entitäten, Sensor/Switch/Number/Select/Time-Domänen, Lovelace Dashboard YAML, Migration von alten Konfigurationen
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

Der Pool Controller integriert sich nahtlos in [Home Assistant](https://home-assistant.io) über **MQTT
Discovery** (Standard-Protokoll). Alle Entitäten werden automatisch registriert, sobald der Controller mit
dem MQTT-Broker verbunden ist.

## MQTT Discovery

Der Controller publiziert [HA MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)-Payloads
beim Start und bei jeder MQTT-Wiederherstellung. Keine manuelle MQTT-Konfiguration nötig — die Geräte
erscheinen automatisch in Home Assistant.

### Verfügbare Entitäten

Die Entity-IDs werden aus dem `name`-Feld der Discovery-Payloads abgeleitet (z.B. Name `"Pool Temperature"`
ergibt `sensor.<prefix>_pool_temperature`). Ersetze `<prefix>` durch deinen Geräte-Prefix
(siehe Warnung unten).

| Domain | Object ID | Kategorie | Beschreibung |
|--------|-----------|-----------|-------------|
| `sensor` | `pool_temperature` | — | Wassertemperatur Pool |
| `sensor` | `solar_temperature` | — | Temperatur Solarkollektor |
| `sensor` | `controller_temperature` | diagnostic | ESP32-Chip-Temperatur |
| `sensor` | `free_heap_space` | diagnostic | Freier Heap-Speicher |
| `sensor` | `max_alloc_block` | diagnostic | Größter allozierbarer Block |
| `sensor` | `wifi_signal_strength` | diagnostic | WiFi-Signalstärke (dBm) |
| `sensor` | `system_uptime` | diagnostic | Betriebszeit (Dauer) |
| `sensor` | `effective_runtime` | diagnostic | Effektive Filterlaufzeit (Dauer) |
| `sensor` | `local_time` | diagnostic | Aktuelle Ortszeit |
| `sensor` | `pool_sensor_found` | diagnostic | Status Pool-Sensor (gefunden/fehlt) |
| `sensor` | `solar_sensor_found` | diagnostic | Status Solar-Sensor (gefunden/fehlt) |
| `select` | `operation_mode` | — | Betriebsart (auto/manu/boost/timer) |
| `select` | `pool_sensor` | config | Pool-Sensor-Adresszuordnung |
| `select` | `solar_sensor` | config | Solar-Sensor-Adresszuordnung |
| `select` | `timezone` | config | Zeitzonenauswahl |
| `switch` | `pool_pump` | — | Pool-Umwälzpumpe |
| `switch` | `solar_pump` | — | Solar-Heizungspumpe |
| `number` | `max_pool_temp` | config | Zieltemperatur Pool max. |
| `number` | `min_solar_temp` | config | Minimale Solar-Aktivierungstemperatur |
| `number` | `temperature_hysteresis` | config | Temperaturhysterese |
| `number` | `circ_temp_threshold` | config | Schwellwert temp. Filterlaufzeit |
| `number` | `circ_temp_factor` | config | Faktor temp. Filterlaufzeit |
| `number` | `circ_max_runtime` | config | Maximale Laufzeit temp. Filterlaufzeit |
| `time` | `timer_start` | config | Timer Startzeit (HH:MM) |
| `time` | `timer_end` | config | Timer Endzeit (HH:MM) |
| `text` | `ntp_server` | config | NTP-Server-Adresse |
| `update` | `firmware` | config | Firmware-Update-Entität |
| `climate` | `pool_thermostat` | config | Pool-Thermostat (HVAC-Modus + Zieltemp.) |

> **Entity-IDs** in HA werden aus dem `name`-Feld der MQTT Discovery generiert. Die entity_id ist
> `sensor.<device_prefix>_pool_temperature` usw. — wobei `<device_prefix>` in der Regel
> `pool_controller` ist (vom Gerätenamen). Prüfe **Entwickler-Tools → Entitäten** und filtere nach
> "pool" um deine IDs zu finden. Ersetze `pool_controller` im Dashboard-YAML durch deinen
> Geräte-Prefix falls er abweicht.

## Lovelace Dashboard

Eine vorgefertigte Lovelace-Dashboard-Konfiguration liegt in
[`dashboard.yaml`](dashboard.yaml) bereit.

### Funktionen

Zwei zielgruppenspezifische Ansichten:

- **🏊 Pool-Ansicht** — für den Bademeister (tägliche Bedienung): Temperaturanzeigen, Modus-Umschaltung (mit Hervorhebung), Timer, Pumpensteuerung, Klima-Thermostat, temperatureabhängige Filterlaufzeit, 24h-Verlauf mit Controller-Temperatur
- **⚙ System-Ansicht** — für den IoT-Entwickler (Diagnose & Konfiguration): Zeitzone & NTP, Sensor-Zuordnung (DS18B20-Adressauswahl), Systemdiagnose (Heap, WiFi, Betriebszeit, Controller-Temperatur, effektive Laufzeit), Firmware-Updates

### Einrichtung

1. Kopiere die YAML-Datei in dein HA-Konfigurationsverzeichnis
2. HA öffnen → **Dashboard → Bearbeiten → Drei-Punkte → Raw-Konfigurationseditor**
3. Inhalt einfügen
4. Entity-IDs an deine Installation anpassen (siehe Warnung)
5. Speichern

### Abhängigkeiten

Die Modus-Buttons nutzen [`button-card`](https://github.com/custom-cards/button-card) (Custom Card) zur
visuellen Hervorhebung des aktiven Modus:

```text
HACS → Frontend → button-card → Installieren
```

Ohne `button-card` `custom:button-card` durch `type: button` ersetzen (dann keine Hervorhebung).

### Warnung zu Entity-IDs

Die Entity-IDs hängen von der MQTT Discovery-Konfiguration ab — das YAML verwendet `pool_controller` als
Geräte-Prefix. Falls deine IDs einen abweichenden Prefix haben, ersetze alle Vorkommen im YAML. Prüfe
deine tatsächlichen Entity-IDs in **Entwickler-Tools → Entitäten** (filtere nach "pool").

## Migration von früheren Versionen

Bei einem Update von einer älteren Firmware können alte Discovery-Konfigurationen im MQTT-Broker verbleiben.
Der Controller publiziert automatisch leere retained Configs für obsolete Entitäten
(`number/timer-start-h/min`, `number/timer-end-h/min`, `number/timezone`). Nach dem ersten MQTT-Reconnect
entfernt Home Assistant die verwaisten Entitäten automatisch.
