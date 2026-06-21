---
title: Von Null aufgebaut
summary: Vollständige Schritt-für-Schritt-Anleitung zum Nachbau des Pool-Controllers — Hardware-Varianten, Stückliste, Flashen, Erstinbetriebnahme, MQTT-Einrichtung, Sensortest, Relaistest, Home-Assistant-Integration, elektrische Inbetriebnahme und Wartung
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "aufbau", "hardware", "anleitung", "inbetriebnahme", "tutorial"]
menu:
  docs:
    parent: Pool Controller
    name: Von Null aufgebaut
    weight: 10
---

> ⚠️ **WARNUNG: Diese Anleitung betrifft Arbeiten an 230V AC Netzspannung.**
> Fehlerhafte Verdrahtung kann zu Stromschlag, Brand oder Geräteschäden führen.
>
> - Nur fortfahren, wenn du Erfahrung mit Netzspannungsinstallationen hast
> - **Vor Arbeiten an der Schaltung immer stromlos schalten**
> - Einen FI-Schutzschalter (RCD) für den Controller-Stromkreis verwenden
> - Im Zweifel eine qualifizierte Elektrofachkraft beauftragen
> - Dies ist ein DIY-Projekt — nicht CE/UL-zertifiziert für den kommerziellen Einsatz

## Einleitung

Diese Anleitung führt dich Schritt für Schritt durch den Nachbau des Smart
Swimming Pool Controllers — ganz ohne Vorkenntnisse zu diesem Projekt. Vom
leeren Tisch bis zum voll funktionsfähigen, in Home Assistant integrierten
Pool-Controller.

> **Gesamtkosten**: unter 100 € (ohne Pumpen und Pool-Infrastruktur)\
> **Erfahrungsstufe**: Anfänger — grundlegende Lötkenntnisse reichen\
> **Zeitaufwand**: 2–4 Stunden

---

## Hardware-Varianten

Der Controller unterstützt ESP32 DevKit V1 mit folgenden Relais-Konfigurationen:

| Variante         | Relais-Modul                        | Max. Last           | Anwendungsfall             |
| ---------------- | ----------------------------------- | ------------------- | -------------------------- |
| **Basic**        | 2-Kanal 5V Relaismodul (active-low) | 10A pro Kanal       | Poolpumpe + Solarpumpe     |
| **NORVI AE01-R** | Integrierter ESP32 + Relais         | 10A                 | All-in-One industriell     |
| **Custom**       | Bel. 3,3V/5V Relais via GPIO        | Abhängig vom Relais | Fortgeschrittene Aufbauten |

Diese Anleitung konzentriert sich auf die **Basic**-Variante. Siehe
[Hardware-Anleitung](/docs/hardware-guide/) für NORVI und kundenspezifische
Konfigurationen.

---

## Stückliste (BOM)

### Benötigte Komponenten

| Menge | Komponente                          | Ca. Preis | Hinweise                              |
| ----- | ----------------------------------- | --------- | ------------------------------------- |
| 1     | ESP32 DevKit V1 (ESP32-WROOM-32)    | 8–12 €    | Jedes ESP32-Entwicklungsboard         |
| 1     | 2-Kanal 5V Relaismodul (active-low) | 4–6 €     | HL-52S, SRD-05VDC-SL-C o. Ä.          |
| 1     | DS18B20 Temperatursensor (Pool)     | 3–5 €     | Edelstahl-Fühler, 3m Kabel            |
| 1     | DS18B20 Temperatursensor (Solar)    | 3–5 €     | Edelstahl-Fühler, 3m Kabel            |
| 1     | 4,7 kΩ Widerstand                   | 0,10 €    | Pull-up für DS18B20 Datenleitung      |
| 1     | Steckbrett + Jumper-Kabel           | 5 €       | Für Prototypen                        |
| 1     | Micro-USB-Kabel + 5V Netzteil       | 5 €       | Mindestens 1A                         |
| 1     | 230V AC Relaismodul (falls nötig)   | 10 €      | Nur wenn Relais nicht direkt schalten |
| 1     | Projektgehäuse (IP65)               | 10–15 €   | Wetterschutzgehäuse                   |

### Optionale Komponenten

| Menge | Komponente                          | Zweck                               |
| ----- | ----------------------------------- | ----------------------------------- |
| 1     | DHT22 Temperatur-/Luftfeuchtesensor | Umgebungstemperatur                 |
| 1     | 5V Summer                           | Akustische Signale                  |
| 1     | LED + 220Ω Widerstand               | Visueller Status                    |
| 1     | 12V → 5V DC-DC-Wandler              | Stromversorgung aus 12V Pool-System |

### Benötigte Werkzeuge

- Lötkolben + Lötzinn
- Abisolierzange / Seitenschneider
- Multimeter
- Kleiner Schraubendreher (für Relais-Klemmen)
- USB-Kabel (Daten + Strom)

---

## Schritt 1: Entwicklungsumgebung vorbereiten

### PlatformIO installieren

PlatformIO ist das Build-System für die Pool-Controller-Firmware.

```bash
# Option A: PlatformIO Core installieren (empfohlen)
python3 -m pip install platformio

# Option B: VS Code mit PlatformIO-Erweiterung
# VS Code installieren, dann PlatformIO-Erweiterung aus dem Marketplace
```

Installation prüfen:

```bash
pio --version
# Sollte ausgeben: PlatformIO Core, Version 6.x oder höher
```

### Git installieren

```bash
# Linux (Debian/Ubuntu)
sudo apt install git

# macOS
brew install git

# Windows
# Download von https://git-scm.com/
```

### Repository klonen

```bash
git clone https://github.com/smart-swimmingpool/pool-controller.git
cd pool-controller
```

---

## Schritt 2: Firmware flashen

### Firmware bauen

```bash
pio run
```

Beim ersten Start lädt PlatformIO automatisch alle benötigten Bibliotheken
herunter.

### ESP32 verbinden

1. ESP32 DevKit per USB mit dem Computer verbinden
2. Seriellen Port ermitteln:
   - Linux: `/dev/ttyUSB0` oder `/dev/ttyACM0`
   - macOS: `/dev/cu.usbserial-*`
   - Windows: `COM3` oder ähnlich

### Firmware hochladen

```bash
# Firmware auf verbundenen ESP32 laden
pio run --target upload --upload-port /dev/ttyUSB0

# Oder automatisch erkennen lassen:
pio run --target upload
```

### Upload prüfen

```bash
# Serielle Ausgabe überwachen
pio device monitor --port /dev/ttyUSB0 --baud 115200
```

Es sollten Boot-Meldungen erscheinen. Der Controller startet im **AP-Modus**
(Access Point), wenn kein WLAN konfiguriert ist.

---

## Schritt 3: Steckbrett-Aufbau (Testaufbau)

Bevor gelötet oder fest installiert wird, die Schaltung auf einem Steckbrett
aufbauen.

### Schaltplan

```text
ESP32 DevKit V1          DS18B20 Sensoren
┌─────────────┐
│ GPIO15 ─────┼────┬──── DATA (Solar)
│ GPIO16 ─────┼────┬──── DATA (Pool)
│ 3.3V  ─────┼────┼──── VCC (beide Sensoren)
│ GND   ─────┼────┼──── GND (beide Sensoren)
│             │    │
│             │   4,7kΩ
│             │    │
│             └────┴──────────────── 3.3V
│
│ GPIO18 ───── IN1  (Relaismodul)
│ GPIO19 ───── IN2  (Relaismodul)
│ 5V    ───── VCC  (Relaismodul)
│ GND   ───── GND  (Relaismodul)
└─────────────┘
```

> **Wichtig**: Die DS18B20-Datenleitung benötigt einen **4,7 kΩ Pull-up-
> Widerstand** zwischen DATA und VCC. Ohne diesen zeigt der Sensor -127 °C an.

### Relaismodul anschließen

Active-Low-Relaismodule schalten, wenn der GPIO auf LOW geht. Die Firmware
ist standardmäßig für **active-low** eingestellt (`relay-invert = false`).

Falls dein Relaismodul **active-high** ist, setze `relay-invert = true` in der
Konfiguration (siehe Schritt 5).

---

## Schritt 4: Erster Start — WLAN-Konfiguration

### Mit AP-Modus verbinden

1. ESP32 per USB mit Strom versorgen
2. 10 Sekunden warten (Bootvorgang)
3. WLAN-Netzwerk **`Pool-Controller-Setup`** suchen
4. Verbinden (kein Passwort)

### WLAN konfigurieren

1. Browser öffnen und **`http://192.168.4.1`** aufrufen
2. Zum Tab **WiFi Setup** gehen
3. Das Heim-WLAN auswählen (oder Scan-Button nutzen)
4. WLAN-Passwort eingeben
5. **Save & Reconnect** klicken

Der Controller startet neu und verbindet sich mit dem Heimnetzwerk.

### Geräte-IP finden

Im Router-DHCP-Client-Liste nachsehen oder Netzwerkscanner verwenden. Das Gerät
heißt `pool-controller` (mDNS: `pool-controller.local`).

Nach erfolgreicher Verbindung ist das Web-Interface unter `http://<gerät-ip>`
erreichbar.

---

## Schritt 5: MQTT-Konfiguration

### Voraussetzungen

- Ein laufender MQTT-Broker (Mosquitto, Home Assistant Add-on, etc.)
- MQTT-Zugangsdaten (falls Authentifizierung aktiviert)

### Über Web-Interface konfigurieren

1. Web-Interface öffnen (`http://<gerät-ip>`)
2. Zum Tab **MQTT Settings** gehen
3. Eingeben:
   - **Broker**: IP oder Hostname des MQTT-Brokers
   - **Port**: 1883 (Standard) oder 8883 (TLS)
   - **Username**: (optional, falls Authentifizierung aktiviert)
   - **Password**: (optional)
   - **Protocol**: `homeassistant` (empfohlen)
4. **Save & Reconnect** klicken

Der Controller startet neu und verbindet sich mit MQTT.

### MQTT-Verbindung prüfen

MQTT Explorer oder Kommandozeilen-Tool verwenden:

```bash
# Alle Pool-Controller-Topics abonnieren
mosquitto_sub -h <broker-ip> -t "pool-controller/#" -v
```

Temperaturwerte und Statusmeldungen sollten erscheinen.

---

## Schritt 6: Sensorprüfung

### Temperaturwerte prüfen

Im Web-Interface **Dashboard**-Tab prüfen:

- **Pool-Temperatur** sollte die Umgebungs-/Wassertemperatur anzeigen
- **Solar-Temperatur** sollte die Kollektortemperatur anzeigen

Wenn ein Sensor **-127 °C** anzeigt, wird der Sensor nicht erkannt:

1. 4,7 kΩ Pull-up-Widerstand prüfen
2. Verkabelung prüfen (VCC, GND, DATA)
3. Kalte Lötstellen prüfen

### DHT22 (Optional)

Falls ein DHT22-Sensor angeschlossen ist, die Luftfeuchte auf dem Dashboard
prüfen.

---

## Schritt 7: Relais-Test (ohne Last)

**WICHTIG**: Während dieses Tests keine 230V AC-Verbraucher anschließen. Die
Relais-Funktion mit einem Multimeter oder einer 5V LED + Widerstand prüfen.

### Manueller Relais-Test

1. Web-Interface Dashboard öffnen
2. **Pool Pump**-Schalter auf ON stellen
3. Relais-Klick hörbar? — Das Relais sollte hörbar schalten
4. Durchgang Relais COM/NO mit Multimeter messen
5. Auf OFF stellen — Relais sollte mit Klick abschalten
6. Für **Solar Pump** wiederholen

### Erwartetes Verhalten

| Relais-Zustand | COM–NO      | COM–NC      |
| -------------- | ----------- | ----------- |
| AUS (inaktiv)  | Offen       | Geschlossen |
| AN (aktiv)     | Geschlossen | Offen       |

Falls das Relais **invertiert** schaltet (AN = COM–NC geschlossen),
`relay-invert = true` in Configuration → Advanced setzen.

---

## Schritt 8: Home-Assistant-Integration

Der Controller nutzt **MQTT Discovery** zur automatischen Anmeldung aller
Entitäten.

### Voraussetzungen

- MQTT konfiguriert und funktionsfähig (Schritt 5)
- Home Assistant mit konfigurierter MQTT-Integration
- `mqtt-protocol = "homeassistant"` in den Controller-Einstellungen

### Auto-Discovery

Nachdem der Controller mit MQTT verbunden ist:

1. Home Assistant öffnen
2. **Einstellungen → Geräte & Dienste** aufrufen
3. Auf **MQTT** klicken — der Pool-Controller sollte automatisch erscheinen
4. Folgende Entitäten sind verfügbar:
   - Pool- und Solar-Temperatursensoren
   - Betriebsarten-Wahlschalter
   - Poolpumpen- und Solarpumpen-Schalter
   - Timer-Konfiguration
   - Temperatur-Schwellwerte

### Lovelace Dashboard

Ein vorgefertigtes Dashboard ist in `docs/home-assistant/dashboard.yaml`
verfügbar.

Siehe [Home-Assistant-Integration](/docs/home-assistant/) für die detaillierte
Einrichtung.

---

## Schritt 9: Elektrische Inbetriebnahme

### Sicherheits-Checkliste

Vor dem Anschließen von 230V AC:

- [ ] DS18B20-Sensoren zeigen plausible Temperaturen (nicht -127 °C)
- [ ] Beide Relais schalten hörbar beim Umschalten im Web-UI
- [ ] Relais-Verdrahtung korrekt: COM → Netz, NO → Pumpe
- [ ] Gehäuse geschlossen und abgedichtet
- [ ] Netzteil für die Last ausgelegt
- [ ] Alle Verbindungen zugentlastet

### Netzspannung anschließen

1. **STROM ABSCHALTEN** am Sicherungsautomaten
2. Relais-Ausgang verdrahten:
   - **COM** → Außenleiter (230V AC)
   - **NO** → Pumpen-Außenleiter
   - **Neutralleiter** → direkt zur Pumpe (nicht über Relais)
   - **Schutzleiter** → Schutzerde
3. Alle Verbindungen in einer Abzweigdose sichern
4. Strom wieder einschalten

### Pumpenfunktion prüfen

1. Web-Interface Dashboard öffnen
2. Poolpumpe auf ON schalten
3. Prüfen, ob die Pumpe läuft
4. Poolpumpe auf OFF schalten
5. Prüfen, ob die Pumpe stoppt
6. Für Solarpumpe wiederholen

> **Tipp zur Erstinbetriebnahme**: In den ersten 30 Minuten in der Nähe des
> Controllers bleiben. Temperaturen und Relais-Zustände überwachen.

---

## Schritt 10: Produktionsaufbau

### Gehäusemontage

1. ESP32 + Relaisplatine im IP65-Gehäuse montieren
2. Kabelverschraubungen für Sensor- und Stromkabel bohren
3. Zugentlastung an allen externen Kabeln anbringen
4. Hochspannungs- (230V) und Niederspannungsleitungen (5V/3,3V) getrennt führen

### Verdrahtungs-Tipps

- Aderendhülsen an Litzen für Schraubklemmen verwenden
- Alle Kabel beschriften
- DS18B20-Kabellängen unter 10m halten
- Geschirmte Kabel für Sensorleitungen in störbehafteter Umgebung

### Abschließende Prüfung

Siehe [Produktions-Checkliste](/docs/production-checklist/) für die vollständige
Inbetriebnahme-Checkliste.

---

## Wartung

### Regelmäßige Prüfungen (Monatlich)

- [ ] Temperaturwerte auf Plausibilität prüfen
- [ ] Web-Interface auf Warnmeldungen prüfen
- [ ] MQTT-Verbindung prüfen
- [ ] ESP32 freien Heap prüfen (sollte > 20 KB sein)

### Firmware-Updates

```bash
# Über Web-Interface
# 1. Aktuelle Firmware von GitHub Releases herunterladen
# 2. Web-UI → Security & Update → OTA Update
# 3. Firmware-Binärdatei auswählen und hochladen

# Über PlatformIO (USB)
pio run --target upload
```

Siehe [OTA-Updates](/docs/ota-updates/) für detaillierte Anleitung.

### Sensor austauschen

Falls ein DS18B20 ausfällt:

1. Controller spannungsfrei schalten
2. Sensor ersetzen
3. Wieder einschalten
4. Prüfen, ob der Wert wieder normal ist

### Wenn etwas nicht funktioniert

Siehe [Fehlerbehebung](/docs/troubleshooting/) für häufige Probleme und
Lösungen.

---

## Verwandte Dokumente

- [Hardware-Anleitung](/docs/hardware-guide/) — Detaillierter Hardware-Aufbau
- [Software-Anleitung](/docs/software-guide/) — Entwicklungsumgebung + API
- [MQTT-Konfiguration](/docs/mqtt-configuration/) — MQTT-Einrichtung
- [Elektrische Sicherheit](/docs/electrical-safety/) — Sicherheitsinformationen
- [Sicherheitsmodell](/docs/safety-model/) — System-Sicherheitsarchitektur
- [Produktions-Checkliste](/docs/production-checklist/) — Checkliste vor Inbetriebnahme
- [Sicherheits-Checkliste](/docs/security-checklist/) — Sicherheitshärtung
- [Fehlerbehebung](/docs/troubleshooting/) — Häufige Probleme und Lösungen
