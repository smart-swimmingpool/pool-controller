---
title: MQTT-Konfiguration
summary: Home Assistant MQTT Discovery Konfiguration, Entity-Referenztabelle und Migration von Homie für den ESP32 Pool Controller
date: "2026-06-11"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
tags: ["docs", "mqtt", "home-assistant", "discovery", "configuration"]
menu:
  docs:
    parent: Pool Controller
    name: MQTT-Konfiguration
    weight: 45
---

# MQTT-Konfiguration (Home Assistant Discovery)

Der Pool Controller verwendet [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
zur automatischen Geräteregistrierung. Alle Geräte erscheinen automatisch in Home Assistant
ohne manuelle Konfiguration.

> **Hinweis**: Die bisherige Homie-Protokoll-Unterstützung wurde in v3.3.0 entfernt. Der Controller
> verwendet ausschließlich Home Assistant MQTT Discovery.

## Konfiguration

Konfigurieren Sie Ihre MQTT-Broker-Verbindung über die Web-UI (Einstellungen → MQTT-Tab):

- **MQTT-Hostname/IP**: Ihre MQTT-Broker-Adresse
- **MQTT-Port**: Standard 1883
- **MQTT-Benutzername/Passwort**: Optionale Authentifizierungsdaten

Nach dem Speichern und Neustarten erscheinen alle Entitäten automatisch in Home Assistant.

### Über die REST-API

Die MQTT-Einstellungen können auch programmatisch konfiguriert werden:

```bash
curl -b "session=<session>" -X POST \
  -d "type=mqtt&host=192.168.1.100&port=1883&username=mqtt_user&password=secret" \
  http://<controller-ip>/api/config
```

## Home Assistant Entitäten

Der Controller veröffentlicht die folgenden Entitäten über MQTT Discovery, gruppiert nach Typ mit ihrer standardmäßigen HA-Entity-Category:

| Kategorie | Anzeige in HA |
|---|---|
| *(keine)* | Hauptseite des Geräts — primäre Messwerte |
| `control` | **Steuerung**-Bereich |
| `config` | **Konfiguration**-Bereich |
| `diagnostic` | **Diagnose**-Bereich |

### Sensoren (schreibgeschützt)

| Funktion                       | HA-Komponente/Objekt-ID   | Entity Category | State-Topic                                                    |
| ------------------------------ | ------------------------- | --------------- | -------------------------------------------------------------- |
| Solar-Temperatur               | `sensor/solar-temp`       | —               | `homeassistant/sensor/pool-controller/solar-temp/state`        |
| Pool-Temperatur                | `sensor/pool-temp`        | —               | `homeassistant/sensor/pool-controller/pool-temp/state`         |
| Controller-Temperatur (ESP32)  | `sensor/controller-temp`  | `diagnostic`    | `homeassistant/sensor/pool-controller/controller-temp/state`   |
| Freier Heap-Speicher           | `sensor/heap`             | `diagnostic`    | `homeassistant/sensor/pool-controller/heap/state`              |
| Max. Alloc-Block               | `sensor/max-alloc`        | `diagnostic`    | `homeassistant/sensor/pool-controller/max-alloc/state`         |
| WiFi-Signalstärke              | `sensor/rssi`             | `diagnostic`    | `homeassistant/sensor/pool-controller/rssi/state`              |
| System-Betriebszeit            | `sensor/uptime`           | `diagnostic`    | `homeassistant/sensor/pool-controller/uptime/state`            |
| Lokale Uhrzeit                 | `sensor/local-time`       | `diagnostic`    | `homeassistant/sensor/pool-controller/local-time/state`        |
| Zeitzonen-Info                 | `sensor/timezone-info`    | `diagnostic`    | `homeassistant/sensor/pool-controller/timezone-info/state`     |
| Log-Ausgabe                    | `sensor/log`              | `diagnostic`    | `homeassistant/sensor/pool-controller/log/state`               |
| OTA-Status                     | `sensor/ota-status`       | `diagnostic`    | `homeassistant/sensor/pool-controller/ota-status/state`        |
| Effektive Laufzeit             | `sensor/effective-runtime`| `diagnostic`    | `homeassistant/sensor/pool-controller/effective-runtime/state` |
| Solar-Sensor gefunden          | `sensor/solar-sensor-found` | `diagnostic`  | `homeassistant/sensor/pool-controller/solar-sensor-found/state` |
| Pool-Sensor gefunden           | `sensor/pool-sensor-found`  | `diagnostic`  | `homeassistant/sensor/pool-controller/pool-sensor-found/state`  |

### Steuerung

| Funktion                       | HA-Komponente/Objekt-ID   | Entity Category | Command-Topic                                                |
| ------------------------------ | ------------------------- | --------------- | ------------------------------------------------------------ |
| Pool-Pumpen-Relais             | `switch/pool-pump`        | `control`       | `homeassistant/switch/pool-controller/pool-pump/set`         |
| Solar-Pumpen-Relais            | `switch/solar-pump`       | `control`       | `homeassistant/switch/pool-controller/solar-pump/set`        |
| Betriebsmodus                  | `select/mode`             | `control`       | `homeassistant/select/pool-controller/mode/set`              |

### Konfiguration (einstellbar via HA)

| Funktion                       | HA-Komponente/Objekt-ID   | Entity Category | Command-Topic                                                   |
| ------------------------------ | ------------------------- | --------------- | --------------------------------------------------------------- |
| Pool max. Temp.                | `number/pool-max-temp`    | `config`        | `homeassistant/number/pool-controller/pool-max-temp/set`        |
| Solar min. Temp.               | `number/solar-min-temp`   | `config`        | `homeassistant/number/pool-controller/solar-min-temp/set`       |
| Hysterese                      | `number/hysteresis`       | `config`        | `homeassistant/number/pool-controller/hysteresis/set`           |
| Circ. Temp.-Schwellwert        | `number/temp-circ-threshold` | `config`     | `homeassistant/number/pool-controller/temp-circ-threshold/set`  |
| Circ. Temp.-Faktor             | `number/temp-circ-factor`    | `config`     | `homeassistant/number/pool-controller/temp-circ-factor/set`     |
| Circ. max. Laufzeit            | `number/temp-circ-max-runtime` | `config`   | `homeassistant/number/pool-controller/temp-circ-max-runtime/set` |
| Timer-Startzeit                | `time/timer-start`        | `config`        | `homeassistant/time/pool-controller/timer-start/set`            |
| Timer-Endzeit                  | `time/timer-end`          | `config`        | `homeassistant/time/pool-controller/timer-end/set`              |
| Zeitzone                       | `select/timezone`         | `config`        | `homeassistant/select/pool-controller/timezone/set`             |
| NTP-Server                     | `text/ntp-server`         | `config`        | `homeassistant/text/pool-controller/ntp-server/set`             |
| Log-Level                      | `select/log-level`        | `config`        | `homeassistant/select/pool-controller/log-level/set`            |
| Log auf Seriell                | `switch/log-serial`       | `config`        | `homeassistant/switch/pool-controller/log-serial/set`           |
| Solar-Sensor-Zuordnung         | `select/solar-sensor`     | `config`        | `homeassistant/select/pool-controller/solar-sensor/set`         |
| Pool-Sensor-Zuordnung          | `select/pool-sensor`      | `config`        | `homeassistant/select/pool-controller/pool-sensor/set`          |

### Buttons & Diagnose

| Funktion                       | HA-Komponente/Objekt-ID   | Entity Category | Command-Topic                                               |
| ------------------------------ | ------------------------- | --------------- | ----------------------------------------------------------- |
| OTA-Update-Trigger             | `button/ota-update`       | `config`        | `homeassistant/button/pool-controller/ota-update/set`       |
| Firmware                       | `update/firmware-update`  | `config`        | `homeassistant/update/pool-controller/firmware-update/set`  |
| Pool-Thermostat                | `climate/thermostat`      | `config`        | (Modus + Temperatur via Climate-Topics)                     |

### Entity-Category-Referenz

| Kategorie | Beschreibung |
|---|---|
| *(keine)* | Primäre Messwerte, auf der Geräte-Hauptseite angezeigt |
| `control` | Primäre Steuerungen (Schalter, Modus-Auswahl) |
| `config` | Konfigurationswerte und Einstellungen |
| `diagnostic` | Gerätediagnose und Systeminformationen |

## Funktionen

Alle Entitäten unterstützen:

- Temperatursensoren (Pool, Solar, Controller)
- Relais-Schalter (Pool-Pumpe, Solar-Pumpe)
- Betriebsmodi (auto, manual, boost, timer)
- Parameter der temperaturabhängigen Filterlaufzeit (Schwellwert, Faktor, max. Laufzeit)
- Konfiguration über MQTT
- Zustandsüberwachung

## Home Assistant Dashboard Beispiel

Ein gebrauchsfertiges Lovelace-Dashboard-Beispiel mit:

- Schnellem Modus-Wechsel (`auto`, `timer`, `boost`, `manu`)
- hervorgehobenen Pool- und Solar-/Speichertemperaturen
- einer dedizierten Konfigurationsansicht
- einem kombinierten Verlaufsdiagramm für Schaltzeiten und Temperaturen

ist verfügbar in:

- [`docs/home-assistant-dashboard-pool.yaml`](home-assistant-dashboard-pool.yaml)

## Migration von Homie (Vor v3.3.0)

Wenn Sie von einer älteren Firmware aktualisieren, die das Homie-Protokoll verwendet hat:

1. Das **Homie-Protokoll wurde in v3.3.0 entfernt** — der Controller verwendet jetzt
   ausschließlich **Home Assistant MQTT Discovery**
2. Nach dem Flashen der neuen Firmware **alte zurückbehaltene Homie-Nachrichten**
   aus Ihrem MQTT-Broker löschen, um Verwirrung zu vermeiden:
   ```bash
   mosquitto_pub -h broker -t homie -n -r
   mosquitto_sub -t 'homie/#' -v | cut -d' ' -f1 | xargs -I{} mosquitto_pub -h broker -t {} -n -r
   ```
3. In Home Assistant den alten Geräteeintrag löschen (Einstellungen → Geräte & Dienste →
   MQTT → Geräte → pool-controller → Löschen) — die neuen Entitäten erscheinen
   automatisch über MQTT Discovery
4. Gerät neu starten, um frische Discovery-Ankündigungen auszulösen
