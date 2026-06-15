---
title: Fehlerbehebung
summary: Fehlerbehebungsanleitung für den Pool-Controller — häufige Probleme, Symptome, Ursachen und Lösungen für Hardware, Software, MQTT und Sensoren
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "fehlerbehebung", "troubleshooting", "anleitung", "debug"]
menu:
  docs:
    parent: Pool Controller
    name: Fehlerbehebung
    weight: 120
---

> ⚠️ **WARNUNG**: Die nachfolgenden Schritte zur Fehlerbehebung können
> Arbeiten an **230V AC-Netzspannung** umfassen (Relais-Test, Pumpen-
> Verdrahtung). **Vor Arbeiten an der Schaltung immer stromlos schalten.**
> Im Zweifel eine qualifizierte Elektrofachkraft beauftragen.

## Schnellreferenz-Matrix

| Symptom                             | Ursache                                                        | Lösung                                                                                         |
| ----------------------------------- | -------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| **-127 °C** Temperaturwert          | DS18B20 nicht erkannt                                          | 4,7 kΩ Pull-up prüfen, Verkabelung und Lötstellen kontrollieren                                |
| **HA findet kein Gerät**            | MQTT-Discovery-Payload nicht gesendet                          | `mqtt-protocol = "homeassistant"` prüfen, MQTT-Broker-Verbindung checken, MQTT Explorer nutzen |
| **Relais-Verhalten invertiert**     | Active-Low-Modul als Active-High konfiguriert (oder umgekehrt) | `relay-invert = true` oder `false` in Configuration → Advanced setzen                          |
| **Safe Mode aktiv**                 | Bootloop erkannt (4 aufeinanderfolgende kurze Boots < 5 Min.)  | Serielles Log auf Absturzursache prüfen, Problem beheben, neu starten                          |
| **OTA-Update fehlgeschlagen**       | TLS-Problem oder zu wenig Flash-Speicher                       | USB-Flash als Fallback nutzen, freien Speicher prüfen                                          |
| **Web-UI lädt nicht**               | ESP32 nicht im Netzwerk oder falsche IP                        | Router-DHCP-Liste prüfen, WLAN-Verbindung prüfen, `pool-controller.local` versuchen            |
| **MQTT-Verbindung verweigert**      | Falsche Zugangsdaten oder Broker nicht erreichbar              | Broker-IP, Port, Benutzername/Passwort in MQTT-Einstellungen prüfen                            |
| **Temperaturwert 85 °C**            | DS18B20-Parasitärversorgungsproblem                            | Externe 3,3V-Versorgung an Sensor-VCC anschließen                                              |
| **Relais klickt nicht**             | GPIO-Pin defekt oder Relaismodul nicht versorgt                | Relais-VCC/GND prüfen, GPIO-Spannung mit Multimeter messen                                     |
| **ESP32 startet immer wieder neu**  | Netzteil zu schwach oder Watchdog-Timeout                      | >1A-Netzteil verwenden, serielles Log auf Panikmeldungen prüfen                                |
| **WLAN-Scan findet nichts**         | ESP32-Antennenproblem oder Interferenz                         | Gerät näher an Router bringen, Antennenanschluss prüfen                                        |
| **mDNS wird nicht aufgelöst**       | Netzwerk unterstützt kein mDNS                                 | IP-Adresse direkt verwenden, mDNS-Reflektor installieren                                       |
| **Sensorwerte schwanken**           | Lose Verbindung oder Interferenz                               | Verkabelung prüfen, geschirmtes Kabel verwenden, Pull-up prüfen                                |
| **Konfiguration nach Neustart weg** | NVS-Korruption oder Konfiguration nicht gespeichert            | Konfiguration erneut speichern, bei Beständigkeit Werksreset versuchen                         |

---

## Detaildiagnose

### 1. Temperatursensor-Probleme

#### Symptom: Sensor zeigt -127 °C

Dies ist der Standardwert der DallasTemperature-Bibliothek, wenn kein Sensor
am OneWire-Bus gefunden wird.

**Checkliste:**

1. 4,7 kΩ Pull-up-Widerstand zwischen DATA und 3,3V prüfen
2. Verdrahtung prüfen: VCC (rot) → 3,3V, GND (schwarz) → GND, DATA
   (gelb/weiß) → GPIO
3. Sensor an einen bekanntermaßen funktionierenden GPIO-Pin anschließen
4. Mit einem funktionierenden DS18B20 testen
5. Spannung auf DATA-Pin messen: sollte ~3,3V im Leerlauf sein, Impulse
   während Kommunikation

#### Symptom: Sensor zeigt 85 °C

Dies deutet darauf hin, dass der DS18B20 im **Parasitärversorgungsmodus** läuft,
aber nicht genug Strom bekommt.

**Lösung:** VCC-Pin (rot) mit 3,3V verbinden statt auf parasitäre Versorgung
zu setzen.

---

### 2. MQTT / Home Assistant-Probleme

#### Symptom: Home Assistant entdeckt den Controller nicht

**Checkliste:**

1. Prüfen, ob der MQTT-Broker läuft: `mosquitto_sub -h <broker> -t "#" -v`
2. Controller-MQTT-Einstellungen über Web-UI prüfen (Tab MQTT Settings)
3. Mit MQTT Explorer testen:
   - `pool-controller/#` abonnieren
   - Nach Discovery-Topics suchen: `homeassistant/sensor/pool-controller-*/config`
4. MQTT-Broker neu starten
5. Controller neustarten

#### Symptom: Entitäten werden als "nicht verfügbar" angezeigt

1. MQTT-Verbindungsstatus im Controller-Web-UI prüfen
2. Prüfen, ob der Broker vom Controller und von HA aus erreichbar ist
3. HA-MQTT-Integration neustarten

---

### 3. Relais-Probleme

#### Symptom: Relais klickt, aber Pumpe läuft nicht

1. COM/NO-Verdrahtung prüfen — Pumpe sollte am NO (normally open) liegen
2. Netzspannung am Relais-Eingang prüfen
3. Pumpenanschluss und Sicherung prüfen

#### Symptom: Relais klickt nicht

1. GPIO-Spannung messen — sollte zwischen HIGH und LOW wechseln
2. Relaismodul VCC (5V) und GND prüfen
3. Relaismodul unabhängig testen (5V zwischen IN und GND)

---

### 4. Boot- und Stabilitätsprobleme

#### Symptom: Safe Mode aktiviert

Der Controller wechselt in den Safe Mode nach 4 aufeinanderfolgenden Boots,
die jeweils kürzer als 5 Minuten waren. Alle Relais werden ausgeschaltet.

**Wiederherstellung:**

1. Serielles Monitor: `pio device monitor --baud 115200`
2. Boot-Log lesen — nach Panikmeldungen, Asserts oder Exceptions suchen
3. Häufige Ursachen:
   - Stack-Overflow in einem Task
   - NULL-Pointer-Dereferenzierung
   - Heap-Erschöpfung
   - Korrupte Konfiguration
4. Problem beheben und neustarten
5. Der Boot-Zähler wird nach einem erfolgreichen >5-Minuten-Lauf zurückgesetzt

#### Symptom: Zufällige Neustarts alle paar Stunden

1. `free_heap_space`-Sensor prüfen — sollte über 20 KB liegen
2. Spannungsversorgung prüfen (mindestens 1A)
3. Auf WLAN-Trennungs-/Wiederverbindungsschleifen achten
4. Serielle Ausgabe auf Absturzmeldungen überwachen

---

### 5. OTA-Update-Probleme

#### Symptom: OTA schlägt mit TLS-Fehler fehl

1. Das Web-Interface verwendet HTTPS für OTA — TLS-Handshake kann in
   manchen Netzwerken fehlschlagen
2. Sicherstellen, dass der ESP32 genug freien Heap hat (Dashboard prüfen)
3. Kleinere Firmware-Binärdatei versuchen

**Workaround:** Über USB flashen:

```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

#### Symptom: OTA läuft hoch, aber Gerät startet nicht

1. Die neue Firmware könnte korrupt oder inkompatibel sein
2. Eine bekanntermaßen funktionierende Version über USB flashen
3. Partitionstabelle-Größe prüfen

---

## Diagnosewerkzeuge

### Serielles Monitor

```bash
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

Nützlich für: Boot-Logs, Absturzausgaben, MQTT-Verbindungsstatus.

### MQTT Explorer

[MQTT Explorer](http://mqtt-explorer.com/) installieren oder CLI nutzen:

```bash
mosquitto_sub -h <broker> -t "pool-controller/#" -v
```

Nützlich für: Prüfen von MQTT-Discovery-Payloads, Anzeigen von Telemetriedaten.

### Web-UI Dashboard

Zugriff über `http://<gerät-ip>` — zeigt Live-Status, Temperaturen, Heap, RSSI.

### Netzwerkscan

```bash
# Controller im Netzwerk finden
nmap -sn 192.168.1.0/24
# Oder die DHCP-Client-Liste des Routers nutzen
```

---

## Hilfe bekommen

Falls die Fehlerbehebung dein Problem nicht löst:

1. [Vorhandene Issues](https://github.com/smart-swimmingpool/pool-controller/issues)
   durchsuchen
2. [Diskussionen](https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions)
   prüfen
3. Folgende Informationen in deinem Hilfegesuch angeben:
   - Firmware-Version (aus Web-UI oder seriellem Log)
   - Hardware-Variante (Basic, NORVI AE01-R, Custom)
   - ESP32-Board-Typ
   - MQTT-Broker und Version
   - Home Assistant-Version (falls zutreffend)
   - Serielle Monitor-Ausgabe (falls verfügbar)

---

## Verwandte Dokumente

- [Von Null aufgebaut](/docs/build-from-zero/) — Komplette Bauanleitung
- [MQTT-Konfiguration](/docs/mqtt-configuration/) — MQTT-Einrichtung
- [Elektrische Sicherheit](/docs/electrical-safety/) — Sicherheitsinformationen
- [Home-Assistant-Integration](/docs/home-assistant/) — HA-Einrichtungsanleitung
