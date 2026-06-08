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

| Domain | Object ID | Kategorie | Beschreibung |
|--------|-----------|-----------|-------------|
| `sensor` | `pool_temperature` | diagnostic | Wassertemperatur Pool |
| `sensor` | `solar_temperature` | diagnostic | Temperatur Solarkollektor |
| `sensor` | `controller_temperature` | diagnostic | ESP32-Chip-Temperatur |
| `sensor` | `free_heap_space` | diagnostic | Freier Heap-Speicher |
| `sensor` | `max_alloc_block` | diagnostic | Größter allozierbarer Block |
| `sensor` | `wifi_signal_strength` | diagnostic | WiFi-Signalstärke |
| `sensor` | `system_uptime` | diagnostic | Betriebszeit (Dauer) |
| `sensor` | `effective_runtime` | diagnostic | Effektive Filterlaufzeit (Dauer) |
| `sensor` | `local_time` | diagnostic | Aktuelle Ortszeit |
| `select` | `operation_mode` | — | Betriebsart (auto/manu/boost/timer) |
| `switch` | `pool_pump` | — | Pool-Umwälzpumpe |
| `switch` | `solar_pump` | — | Solar-Heizungspumpe |
| `number` | `max_pool_temp` | — | Zieltemperatur Pool max. |
| `number` | `min_solar_temp` | — | Minimale Solar-Aktivierungstemperatur |
| `number` | `temperature_hysteresis` | config | Temperaturhysterese |
| `number` | `temp_circ_threshold` | — | Schwellwert temp. Filterlaufzeit |
| `number` | `temp_circ_factor` | — | Faktor temp. Filterlaufzeit |
| `number` | `temp_circ_max_runtime` | — | Maximale Laufzeit temp. Filterlaufzeit |
| `time` | `timer_start` | — | Timer Startzeit (HH:MM) |
| `time` | `timer_end` | — | Timer Endzeit (HH:MM) |
| `select` | `timezone` | config | Zeitzonenauswahl |
| `text` | `ntp_server` | config | NTP-Server-Adresse |
| `update` | `firmware` | — | Firmware-Update-Entität |

> **Entity-IDs** in HA werden aus der MQTT unique_id generiert und können abweichen (z.B.
> `sensor.pool_controller_pool_temperature`). Prüfe **Entwickler-Tools → Entitäten** und filtere nach "pool"
> um deine IDs zu finden.

## Lovelace Dashboard

Eine vorgefertigte Lovelace-Dashboard-Konfiguration liegt in
[`home-assistant-dashboard-pool.yaml`](dashboard.yaml) bereit.

### Funktionen

- **Pool-Ansicht**: Temperaturanzeigen, Modus-Umschaltung, Timer, Pumpensteuerung, 24h-Verlauf
- **Konfigurations-Ansicht**: Zeitzone, NTP-Server, Systeminformationen, Firmware-Updates
- Aktive Modus-Hervorhebung für die Betriebsart-Buttons

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

Die Entity-IDs hängen vom MQTT-unique_id-Format des Controllers ab. Das YAML verwendet `pool_controller`
als Prefix — falls deine IDs abweichen (z.B. MAC-basiert), ersetze alle Vorkommen.

## Migration von früheren Versionen

Bei einem Update von einer älteren Firmware können alte Discovery-Konfigurationen im MQTT-Broker verbleiben.
Der Controller publiziert automatisch leere retained Configs für obsolete Entitäten
(`number/timer-start-h/min`, `number/timer-end-h/min`, `number/timezone`). Nach dem ersten MQTT-Reconnect
entfernt Home Assistant die verwaisten Entitäten automatisch.
