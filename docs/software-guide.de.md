---
title: Software-Entwicklung
summary: Software-Entwicklungsleitfaden für den Pool Controller — PlatformIO Build-Umgebung, Library-Abhängigkeiten, REST-API-Referenz, Weboberfläche und Code-Architektur
date: "2020-05-28"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "controller", "tutorial"]
menu:
  docs:
    parent: Pool Controller
    name: Software-Entwicklung
    weight: 30
---

## Entwicklungsumgebung

## Benötigte Bibliotheken

- [AsyncMqttClient](https://github.com/marvinroger/async-mqtt-client) @ 0.9.0
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [OneWire](https://github.com/PaulStoffregen/OneWire)
- [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor)
- [DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
- [NTPClient](https://github.com/arduino-libraries/NTPClient) @ 3.2.1
- [Timezone](https://github.com/JChristensen/Timezone) @ 1.2.6
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) @ 7.3.2
- [Bounce2](https://github.com/thomasfredericks/Bounce2)
- [Wire](https://github.com/espressif/arduino-esp32/tree/master/libraries/Wire)

Vielen Dank an die Entwickler dieser Bibliotheken!

## Pin-Konfiguration

In den Quellen unter `Config.hpp` sind die GPIO-Pin-Zuweisungen definiert.
Details siehe auch [Hardware-Leitfaden](../hardware-guide/#pin-assignment-firmware-defaults).

```cpp
constexpr uint8_t PIN_DS_SOLAR   = 15;  // Pin des Solar-Temperatursensors (GPIO15)
constexpr uint8_t PIN_DS_POOL    = 16;  // Pin des Pool-Temperatursensors (GPIO16)

constexpr uint8_t PIN_RELAY_POOL  = 18;  // Pin zur Ansteuerung des Pool-Pumpenrelais
constexpr uint8_t PIN_RELAY_SOLAR = 19;  // Pin zur Ansteuerung des Solar-Pumpenrelais

constexpr uint8_t TEMP_READ_INTERVAL = 30;
```

## Weboberfläche & Direktzugriff

Der Controller enthält einen **integrierten Webserver auf Port 80**, der ein
vollständiges Verwaltungs-Dashboard bereitstellt. Er läuft in zwei Modi:

| Modus                       | Bedingungen                           | Zugriff                                                            |
| --------------------------- | ------------------------------------- | ------------------------------------------------------------------ |
| **AP-Modus** (Access Point) | Kein WiFi konfiguriert (Werkszustand) | SSID `Pool-Controller-Setup`, IP `192.168.4.1`, kein Passwort      |
| **STA-Modus** (Station)     | Normale WiFi-Verbindung               | DHCP-IP des ESP32 im lokalen Netzwerk, Passwort-Login erforderlich |

### API-Endpunkte

| Route                    | Auth    | Funktion                                                                      |
| ------------------------ | ------- | ----------------------------------------------------------------------------- |
| `GET /`                  | Cookie  | Dashboard SPA (Single Page Application)                                       |
| `GET /login`             | Cookie  | Login-Seite                                                                   |
| `POST /api/login`        | -       | Session-Cookie ausstellen (SHA-256 Passwortprüfung)                           |
| `GET /api/status`        | ❌ Nein | Live-Telemetrie (Temperaturen, Pumpenstatus, Heap, RSSI, Temperaturschwellen) |
| `GET /api/scan`          | Ja      | Verfügbare WiFi-Netzwerke scannen                                             |
| `GET /api/config`        | Ja      | Aktuelle Konfiguration auslesen                                               |
| `POST /api/config`       | Ja      | Konfiguration speichern (`type=settings\|wifi\|mqtt\|password`)               |
| `GET /api/restart`       | Ja      | ESP32 neu starten                                                             |
| `GET /api/factory_reset` | Ja      | Konfigurationsdatei löschen, Neustart im AP-Modus                             |
| `POST /api/update`       | Ja      | OTA-Firmware-Update (signiertes .bin hochladen)                               |

### REST-API direkt nutzen

Sie können programmatisch auf den Controller zugreifen:

```bash
# Live-Telemetrie abrufen (keine Authentifizierung nötig)
curl http://<controller-ip>/api/status

# Session-Cookie holen
SESSION=$(curl -s -c - -X POST -d "password=admin" \
  http://<controller-ip>/api/login | grep session | awk '{print $NF}')

# Konfiguration auslesen
curl -b "session=$SESSION" http://<controller-ip>/api/config

# Einstellungen schreiben (bleiben nach Neustart erhalten und benachrichtigen Home Assistant)
curl -b "session=$SESSION" -X POST \
  -d "type=settings&mode=auto&max_pool=30.0&min_solar=55.0&hysteresis=1.0" \
  http://<controller-ip>/api/config
```

### Authentifizierung

- Im **AP-Modus** ist die Weboberfläche ungeschützt (absichtlich für die Einrichtung)
- Im **STA-Modus** ist eine cookie-basierte Sitzung erforderlich (15 Minuten Timeout)
- Standardpasswort ist `admin`
- Das Passwort wird als SHA-256-Hash in `/config.json` gespeichert
- Das Dashboard zeigt Temperaturen und Schwellwerte auch nach Session-Timeout
  an, da `/api/status` (ohne Authentifizierung) die Felder `temp_max_pool` und
  `temp_min_solar` enthält. Konfigurationen zu schreiben erfordert weiterhin
  eine gültige Session.

## Konfigurationspersistenz

Die Konfiguration wird in zwei unabhängigen Speichersystemen abgelegt, damit
alle Einstellungen Neustarts und Stromausfälle überdauern:

### 1. ConfigManager — Gerätekonfiguration (LittleFS)

| Datei        | `/config.json`                                                             |
| ------------ | -------------------------------------------------------------------------- |
| Max Größe    | 4 KB                                                                       |
| Inhalt       | WiFi, MQTT, NTP, ControllerSettings, Admin-Passwort-Hash                   |
| Verwaltung   | `ConfigManager::load()` beim Start, `ConfigManager::save()` bei Änderungen |
| Zurücksetzen | `ConfigManager::reset()` → Werkseinstellungen                              |

### 2. StateManager — Laufzeitstatus (ESP32 NVS / Preferences)

| Namespace | `pool-controller`                                                            |
| --------- | ---------------------------------------------------------------------------- |
| Inhalt    | Betriebsmodus, poolMaxTemp, solarMinTemp, Hysterese, timerStart/End          |
| API       | `StateManager::saveString/Float/Int/Bool` → typsichere Key-Value-Speicherung |

### Datenfluss bei Konfigurationsänderungen

```text
Web UI / REST API            MQTT (Home Assistant)
        │                            │
        ▼                            ▼
────────┴────── ConfigManager ───────┴────
                save() → /config.json (LittleFS)
                ↓
        OperationModeNode
        (Laufzeitparameter)
                ↓
        MqttPublisher::publishStates()
        → MQTT Topics → Home Assistant
```

> **Hinweis:** Werden Einstellungen über die Weboberfläche geändert, wird
> Home Assistant beim nächsten Messzyklus aktualisiert (alle `loopInterval`
> Sekunden, standardmäßig 10s). Änderungen über MQTT werden sofort bestätigt.

## MQTT-Kommunikation

Der Controller verwendet seit v3.3.0 ausschließlich **Home Assistant MQTT Discovery**.
Siehe die entsprechende Dokumentation für Details:

- **[MQTT-Konfiguration](../mqtt-configuration)** — Protokoll, Entity-Referenz, Homie-Migration
- **[Home Assistant Integration](../home-assistant)** — Lovelace-Dashboard, HA-Entity-IDs

### Retained Messages löschen

Wenn Sie zurückbehaltene MQTT-Nachrichten löschen müssen:

```bash
# Einen bestimmten Home Assistant Topic löschen
mosquitto_pub -h hostname -t "homeassistant/sensor/pool-controller/pool-temp/state" -n -r
```

## Konfigurationspersistenz (Detail)

Die Konfiguration wird in zwei unabhängigen Speichersystemen abgelegt, damit
alle Einstellungen Neustarts und Stromausfälle überdauern:

### 1. ConfigManager — Geräteeinstellungen (LittleFS)

Der Controller speichert WiFi-, MQTT-, NTP- und Geräteeinstellungen in einer
JSON-Datei auf LittleFS. Siehe [`ConfigManager`](../state-persistence) für Details.

### 2. Laufzeitstatus (ESP32 NVS / Preferences)

Betriebsmodus, Relaiszustände und Temperaturparameter werden in NVS
gespeichert für sofortige Wiederherstellung nach Stromausfall.

### Datenfluss bei Konfigurationsänderungen

```text
Web UI / REST API            MQTT (Home Assistant)
        │                            │
        ▼                            ▼
────────┴────── ConfigManager ───────┴────
                save() → /config.json (LittleFS)
                ↓
        OperationModeNode
        (Laufzeitparameter)
                ↓
        MqttPublisher::publishStates()
        → MQTT Topics → Home Assistant
```

> **Hinweis:** Werden Einstellungen über die Weboberfläche geändert, wird
> Home Assistant beim nächsten Messzyklus aktualisiert (alle `loopInterval`
> Sekunden, standardmäßig 10s). Änderungen über MQTT werden sofort bestätigt.
