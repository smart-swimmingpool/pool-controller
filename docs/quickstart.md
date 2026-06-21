---
title: "🚀 Quick Start: Pool Controller in 5 Schritten"
summary: "Schnellanleitung für den Einstieg – von der Teilebestellung bis zur Integration in dein Smart Home"
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "quickstart", "tutorial", "beginner"]
menu:
  docs:
    parent: Pool Controller
    name: Quick Start
    weight: 10
---

## 🎯 Ziel

Dieser Leitfaden führt dich **Schritt für Schritt** durch den Aufbau
und die Inbetriebnahme des **Smart Swimmingpool Controllers** – **ohne
Vorkenntnisse in IoT oder Elektronik** (außer Grundlagen wie
Lötkenntnisse).

**Zeitaufwand:** ~4–6 Stunden (inkl. Teilebeschaffung)
**Kosten:** ~45–75€ (ohne Pumpen/Pool-Infrastruktur)

---

## ⚠️ Wichtige Sicherheitshinweise

> **⚠️ ACHTUNG: Dieses Projekt arbeitet mit 230V Wechselstrom!**
>
> - **Nur für Personen mit Grundkenntnissen in Elektronik und Sicherheit!**
> - **Immer einen FI-Schalter (RCD) mit 30mA Auslösestrom verwenden!**
> - **Arbeite nur an spannungsfreien Schaltkreisen!**
> - **Dieses Projekt ist nicht CE- oder UL-zertifiziert – nur für den Eigenbau!**
> - **Bei Unsicherheit: Konsultiere einen Elektriker!**

**📌 Vollständige Sicherheitshinweise findest du hier:** [Sicherheitshinweise & Warnungen](safety.md)

---

## 📦 Schritt 1: Teile besorgen (BOM – Bill of Materials)

Hier ist die **komplette Teileliste** mit direkten Links zu empfohlenen Shops (Stand: Juni 2026).

### 🛒 Teileliste

| **#** | **Komponente**                     | **Menge** | **Preis (ca.)** | **Empfohlene Shops**                                                                 | **Hinweise**                                                                                     |
|-------|------------------------------------|-----------|------------------|------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------|
| 1     | ESP32 DevKit V1                     | 1         | 10–15€           | [Amazon](https://www.amazon.de/s?k=ESP32+DevKit+V1), [Reichelt](https://www.reichelt.de/), [AliExpress](https://www.aliexpress.com/) | **Mind. 4MB Flash**, z. B. "ESP32 DevKit V1" oder "NodeMCU-32S"                              |
| 2     | DS18B20 Temperatur-Sensor (wasserdicht, Edelstahl, 1m Kabel) | 2         | 8–12€            | [Amazon](https://www.amazon.de/s?k=DS18B20+wasserdicht), [Conrad](https://www.conrad.de/), [AliExpress](https://www.aliexpress.com/) | **1x für Pool, 1x für Solarkreis**                                                           |
| 3     | 2-Kanal 5V Relay-Modul (mit Optokoppler) | 1         | 5–8€             | [Amazon](https://www.amazon.de/s?k=2+Kanal+5V+Relay+Optokoppler), [Reichelt](https://www.reichelt.de/) | **Muss Aktiv-High sein!** (Prüfe Datenblatt oder Teste mit Jumper)                          |
| 4     | Widerstand 4.7kΩ (¼W, Metallfilm)   | 2         | <1€              | [Reichelt](https://www.reichelt.de/), [Conrad](https://www.conrad.de/) | **Pull-Up für DS18B20**                                                                       |
| 5     | Breadboard (830 Punkte) + Jumper-Kabel | 1         | 3–8€             | [Amazon](https://www.amazon.de/s?k=Breadboard+830), [Reichelt](https://www.reichelt.de/) | Für Prototyping                                                                               |
| 6     | USB-Netzteil (5V/1A+)               | 1         | 5–10€            | Jedes USB-Ladegerät (z. B. Handy-Ladegerät)                                   | **Mind. 1A Stromstärke**                                                                       |
| 7     | Gehäuse (IP54 oder höher)           | 1         | 5–10€            | [Amazon](https://www.amazon.de/s?k=IP54+Gehäuse), [Reichelt](https://www.reichelt.de/) | Optional, aber **empfohlen für Outdoor-Einsatz**                                              |
| 8     | Schraubklemmen (2-Pin, 5mm Raster)  | 4–6       | 2–3€             | [Reichelt](https://www.reichelt.de/), [Conrad](https://www.conrad.de/) | Für einfache Verbindung der Sensoren/Relais                                                   |
| **Gesamt** | | | **~45–75€** | | **Ohne Pumpen/Pool-Infrastruktur** |

### 💡 Tipps zur Teilebeschaffung

- **Alle Teile sind bei Amazon, Reichelt, Conrad oder AliExpress verfügbar.**
- **ESP32:** Achte auf **"ESP32 DevKit V1"** oder **"NodeMCU-32S"** (nicht ESP32-S2/S3/C3, da nicht kompatibel mit der Firmware).
- **DS18B20:** Suche nach **"wasserdicht" und "Edelstahl"** für Langlebigkeit.
- **Relay-Modul:** **Aktiv-High** ist entscheidend! Teste das Modul vor dem Einbau mit einem einfachen Arduino-Sketch.

---

## 🛠️ Schritt 2: Hardware aufbauen (Prototyping auf Breadboard)

### 📌 Benötigte Werkzeuge

- Lötkolben + Lötzinn (falls du ein Perfboard verwendest)
- Seitenschneider + Abisolierzange
- Multimeter (für Kontinuitätstests)
- Schraubendreher (für Gehäuse)

### 🔌 Schaltplan

Hier ist der **komplette Schaltplan** für den Pool Controller:

```text
  ── POWER SUPPLY ──────────────────────────────────────────────────

  [USB Netzteil 5V/1A+]       [ESP32 Board]           [Relay-Modul]
   ┌──────────┐              ┌─────────────┐          ┌────────────┐
   │ 5V (+) ──┼──────────────┤ VIN         │          │ VCC        │
   │          │              │  (versorgt  │      ┌───┤ (versorgt  │
   │          │              │   Board)    │      │   │  Spulen)   │
   │          │              │             │      │   │            │
   │          │              │ 3V3 ───┬────┘      │   │ GND ◄──────┼──┐
   │          │              │        │           │   └────────────┘  │
   │ GND ─────┼──────────────┤ GND ◄──┼───────────┼──────────────────┘
   └──────────┘              └────────┘           │

  ── SENSOREN (DS18B20) ────────────────────────────────────────────

  [DS18B20 Solar]                               [DS18B20 Pool]
   ┌───────────┐                                 ┌───────────┐
   │ VDD (rot) ─┼── 3.3V ─────────────────────────┼── VDD (rot)│
   │           │                                 │           │
   │ GND (schwarz)─┼── GND ──────────────────────────┼── GND (schwarz)│
   │           │                                 │           │
   │DATA (gelb) ─┼── GPIO32 (PIN_DS_SOLAR)        │           │
   │           │  │                              │           │
   │           │  └──[4.7kΩ]── 3.3V ← pull-up    │           │
   │           │                   resistor      │           │
   │           │                                 │DATA (gelb) ─┼── GPIO33 (PIN_DS_POOL)
   │           │                                 │           │  │
   │           │                                 │           │  └──[4.7kΩ]── 3.3V
   └───────────┘                                 └───────────┘

  ── RELAY STEUERUNG ────────────────────────────────────────────────

  ESP32 GPIO25 ────────────────────────────────── Relay IN1 (Pool-Pumpe)
  ESP32 GPIO26 ────────────────────────────────── Relay IN2 (Solar-Pumpe)

  ── STATUS-LED ──────────────────────────────────────────────────────

  ESP32 Built-in LED (GPIO2) ── Status blink codes (Homie)
```

### 📋 Schritt-für-Schritt Aufbau auf dem Breadboard

1. **ESP32 platzieren**
   - Setze das ESP32-Board **über die Mitte des Breadboards** (so dass die Pins auf beiden Seiten frei sind).

2. **Stromversorgung anschließen**
   - Verbinde **5V** des USB-Netzteils mit der **5V-Leiste** des Breadboards.
   - Verbinde **GND** des USB-Netzteils mit der **GND-Leiste** des Breadboards.
   - Verbinde **ESP32 VIN** mit der **5V-Leiste** (für Relay-Modul).
   - Verbinde **ESP32 GND** mit der **GND-Leiste**.

3. **DS18B20-Sensoren anschließen**
   - **VDD (rot)** → **3.3V-Leiste** des ESP32.
   - **GND (schwarz)** → **GND-Leiste**.
   - **DATA (gelb/weiß)** → **GPIO32 (Solar)** und **GPIO33 (Pool)**.
   - **Füge einen 4.7kΩ-Widerstand zwischen DATA und 3.3V hinzu** (Pull-Up).

4. **Relay-Modul anschließen**
   - **VCC** → **5V-Leiste**.
   - **GND** → **GND-Leiste**.
   - **IN1** → **GPIO25** (Pool-Pumpe).
   - **IN2** → **GPIO26** (Solar-Pumpe).

5. **230V-Seite (ACHTUNG: Nur für Fachleute!)**
   - **COM1** → **Phase (L)** der Pool-Pumpe.
   - **NO1** → **Pool-Pumpe**.
   - **COM2** → **Phase (L)** der Solar-Pumpe.
   - **NO2** → **Solar-Pumpe**.
   - **Nullleiter (N)** → **Direkt zur Pumpe** (ohne Relay!).
   - **Immer einen RCD (FI-Schalter) verwenden!**

### 📸 Visuelle Anleitung

> **🔗 [Fritzing-Datei herunterladen](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)**
> (Öffne mit [Fritzing](https://fritzing.org/))

Hier ist ein **Beispiel-Foto** des Aufbaus:

![Pool Controller Breadboard Aufbau](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/images/pool-controller_breadboard.png)
*Beispielaufbau auf einem Breadboard*

---

## 💻 Schritt 3: Firmware flashen

### 📌 Voraussetzungen

- **Arduino IDE** oder **PlatformIO** (empfohlen)
- **USB-Kabel** (für ESP32)
- **Git** (optional, für manuelle Updates)

### 🔧 Option 1: Arduino IDE (einfachste Methode)

1. **Arduino IDE installieren**
   - Lade die [Arduino IDE](https://www.arduino.cc/en/software) herunter und installiere sie.

2. **ESP32-Board-Unterstützung hinzufügen**
   - Öffne die Arduino IDE.
   - Gehe zu **Datei → Einstellungen**.
   - Füge folgende URL unter **Zusätzliche Board-Verwalter-URLs** hinzu:

     ```text
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```

   - Klicke auf **OK**.
   - Gehe zu **Werkzeuge → Board → Boards-Verwalter**.
   - Suche nach **esp32** und installiere das Paket.

3. **Benötigte Bibliotheken installieren**
   - Gehe zu **Sketch → Bibliothek einbinden → Bibliotheken verwalten**.
   - Installiere folgende Bibliotheken:
     - **ArduinoJson** (von Benoît Blanchon, Version 7.3.0)
     - **NTPClient** (von Fabrice Weinberg)
     - **OneWire** (von Paul Stoffregen)
     - **DallasTemperature** (von Miles Burton)

4. **Projekt herunterladen**
   - Klone das Repository oder lade es als ZIP herunter:

     ```bash
     git clone https://github.com/smart-swimmingpool/pool-controller.git
     ```

   - Das Projekt verwendet `src/main.cpp` als Einstiegspunkt (Standard-Arduino-Framework).
     **Hinweis:** Der Pool Controller ist für PlatformIO organisiert. Falls du die Arduino IDE
     nutzt, installiere die **PlatformIO-Erweiterung** (empfohlen) oder erstelle eine
     `.ino`-Datei, die `src/main.cpp` inkludiert.

5. **Board & Port auswählen**
   - Gehe zu **Werkzeuge → Board → ESP32 Arduino → ESP32 Dev Module**.
   - Wähle den **COM-Port** deines ESP32 aus (unter **Werkzeuge → Port**).

6. **Firmware kompilieren & flashen**
   - Klicke auf den **Häkchen-Button (Kompilieren)**.
   - Klicke auf den **Pfeil-Button (Hochladen)**.
   - Warte, bis der Upload abgeschlossen ist (ca. 1–2 Minuten).

---

### 🔧 Option 2: PlatformIO (für Fortgeschrittene)

1. **PlatformIO installieren**
   - Installiere die [PlatformIO-Erweiterung](https://platformio.org/install) für VS Code.

2. **Projekt öffnen**
   - Öffne den **`pool-controller`**-Ordner in VS Code.
   - PlatformIO erkennt das Projekt automatisch.

3. **Firmware flashen**
   - Klicke auf den **PlatformIO-Button** in der Seitenleiste.
   - Wähle dein **ESP32-Board** aus (z. B. `esp32dev`).
   - Klicke auf **Build & Upload** (Pfeil-Button).

---

## 🌐 Schritt 4: WiFi & MQTT einrichten

### 📡 WiFi-Konfiguration (AP-Modus)

1. **Stromversorgung anschließen**
   - Verbinde das ESP32 mit dem USB-Netzteil.

2. **Auf AP-Modus warten**
   - Die **Status-LED** blinkt **schnell (5x pro Sekunde)** → AP-Modus aktiv.
   - Verbinde dein **Smartphone oder Laptop** mit dem WiFi-Netzwerk:
     - **SSID:** `Pool-Controller-Setup`
     - **Passwort:** Keines (offenes Netzwerk)

3. **Web-Interface öffnen**
   - Öffne einen Browser und gehe zu:

   ```text
   http://192.168.4.1
   ```

   - Du wirst automatisch zum **Setup-Portal** weitergeleitet.

4. **WiFi-Netzwerk auswählen**
   - Gehe zum Tab **WiFi Setup**.
   - Klicke auf **Scan Networks**.
   - Wähle dein **Heim-WiFi-Netzwerk** aus.
   - Gib dein **WiFi-Passwort** ein.
   - Klicke auf **Save**.

5. **Neustart abwarten**
   - Der Controller **startet neu** und verbindet sich mit deinem WiFi.
   - Die **Status-LED** blinkt **langsam (1x pro Sekunde)** → WiFi-Verbindung wird hergestellt.
   - Sobald die LED **dauerhaft leuchtet**, ist die Verbindung erfolgreich.

6. **IP-Adresse finden**
   - Gehe zu deinem **Router** und suche nach dem Gerät **`Pool-Controller`** in der DHCP-Client-Liste.
   - Alternativ: Verwende einen **Netzwerk-Scanner** (z. B. `nmap`):

     ```bash
     nmap -p 80 192.168.1.0/24  # Ersetze 192.168.1.0 mit deinem Subnetz
     ```

---

### 📡 MQTT-Broker einrichten (für Home Assistant/openHAB)

#### Option A: Mosquitto auf Raspberry Pi (empfohlen)

1. **Mosquitto installieren**

   ```bash
   sudo apt update
   sudo apt upgrade
   sudo apt install mosquitto mosquitto-clients
   ```

2. **Mosquitto starten**

   ```bash
   sudo systemctl start mosquitto
   sudo systemctl enable mosquitto
   ```

3. **MQTT im Pool Controller konfigurieren**
   - Öffne das **Web-Interface** des Controllers (z. B. `http://192.168.1.100`).
   - Gehe zum Tab **MQTT Settings**.
   - Trage folgende Daten ein:
     - **MQTT Host:** `IP deines MQTT-Brokers` (z. B. `192.168.1.50`)
     - **MQTT Port:** `1883`
     - **MQTT Username/Password:** Falls aktiviert.
   - Klicke auf **Save**.
   - **Starte den Controller neu** (über das Web-Interface oder durch Stromtrennung).

#### Option B: Home Assistant mit eingebautem MQTT-Broker

1. **Home Assistant installieren**
   - Folge der [offiziellen Anleitung](https://www.home-assistant.io/installation/).

2. **MQTT-Broker Add-on installieren**
   - Gehe zu **Supervisor → Add-on Store**.
   - Suche nach **Mosquitto broker** und installiere es.
   - Starte das Add-on und aktiviere **Start on boot**.

3. **MQTT im Pool Controller konfigurieren**
   - Trage folgende Daten ein:
     - **MQTT Host:** `IP deines Home Assistant-Servers`
     - **MQTT Port:** `1883`
     - **MQTT Username/Password:** Falls im Add-on konfiguriert.

---

## 🏠 Schritt 5: Integration in Smart Home (Home Assistant/openHAB)

### 🏡 Option A: Home Assistant (empfohlen)

1. **Home Assistant starten**
   - Falls noch nicht geschehen, installiere [Home Assistant](https://www.home-assistant.io/).

2. **MQTT-Integration aktivieren**
   - Gehe zu **Einstellungen → Geräte & Dienste**.
   - Klicke auf **+ Hinzufügen**.
   - Suche nach **MQTT** und wähle es aus.
   - Trage die **IP deines MQTT-Brokers** ein (z. B. `192.168.1.50`).
   - Klicke auf **Überprüfen** und dann auf **Einrichten**.

3. **Geräte automatisch erkennen lassen**
   - Der Pool Controller **sollte automatisch erkannt werden** (Home Assistant MQTT Discovery).
   - Gehe zu **Einstellungen → Geräte & Dienste**.
   - Suche nach **Pool Controller** und füge es hinzu.

4. **Dashboard anpassen**
   - Gehe zu **Dashboard → Bearbeiten**.
   - Füge die **Pool Controller-Entitäten** hinzu (z. B. Temperatursensoren, Schalter für Pumpen).
   - **Beispiel-Dashboard:**
     - [Lovelace Dashboard für Pool Controller](https://github.com/smart-swimmingpool/pool-controller/blob/main/docs/home-assistant-dashboard-pool.yaml)

---

### 🏡 Option B: openHAB

1. **openHAB installieren**
   - Folge der [offiziellen Anleitung](https://www.openhab.org/docs/installation/).

2. **MQTT-Binding installieren**
   - Gehe zu **Paper UI → Add-ons → Bindings**.
   - Suche nach **MQTT Binding** und installiere es.

3. **MQTT-Broker konfigurieren**
   - Bearbeite die Datei **`/etc/openhab/services/mqtt.cfg`**:

   ```ini
   broker.url=tcp://192.168.1.50:1883
   broker.user=mqtt_user
   broker.password=secret
   ```

   - Starte openHAB neu:

     ```bash
     sudo systemctl restart openhab
     ```

4. **Pool Controller manuell einrichten**
   - Da der Controller **keine Homie-Unterstützung mehr hat** (ab v3.3.0), musst du die Geräte **manuell** einrichten.
   - **Beispiel-Konfiguration:**
     - [openHAB-Konfiguration für Pool Controller](https://github.com/smart-swimmingpool/openhab-config)

---

## 🔍 Schritt 6: Sensoren kalibrieren & testen

### 🌡️ Temperatur-Sensoren (DS18B20) testen

1. **Sensoren überprüfen**
   - Öffne das **Web-Interface** des Controllers.
   - Gehe zum **Dashboard**-Tab.
   - Prüfe, ob die **Temperaturwerte** angezeigt werden:
     - **Pool-Temperatur** (sollte ~Raumtemperatur sein, wenn Sensor nicht im Wasser ist).
     - **Solar-Temperatur** (sollte ähnlich sein).

2. **Häufige Probleme & Lösungen**

   | **Problem**                          | **Lösung**                                                                                     |
   |--------------------------------------|-------------------------------------------------------------------------------------------------|
   | Sensor zeigt `-127°C` an              | **Kein Pull-Up-Widerstand** oder **falsche GPIO-Pins** → Prüfe 4.7kΩ-Widerstand und Pin-Belegung. |
   | Sensor zeigt `85°C` an                | **Kurzschluss oder defekter Sensor** → Teste Sensor mit einfachem Arduino-Sketch.              |
   | Sensor wird nicht erkannt            | **Falsche GPIO-Pins oder defektes Kabel** → Prüfe Verbindungen mit Multimeter.                 |

---

### 🔌 Relay-Modul testen

1. **Relays manuell steuern**
   - Gehe zum **Web-Interface → Configuration**-Tab.
   - Wähle den **Modus: Manual**.
   - Aktiviere **Pool Pump** oder **Solar Pump**.
   - **Hörst du ein Klick-Geräusch?** → Relay schaltet.

2. **Pumpen testen (ACHTUNG: Nur für Fachleute!)**
   - Verbinde die Pumpen mit den **Relay-Ausgängen** (COM/NO).
   - Aktiviere die Pumpen über das Web-Interface.
   - **Läuft die Pumpe?** → Alles funktioniert!

---

## 🎉 Schritt 7: Fertig! – Nächste Schritte

✅ **Herzlichen Glückwunsch!** Dein Pool Controller ist jetzt einsatzbereit.

### 📌 Nächste Schritte

1. **Automatisierung einrichten**
   - Konfiguriere **Regeln** (Auto, Timer, Boost) im Web-Interface.
   - Beispiel:
     - **Auto-Modus:** Pumpe läuft nur, wenn Solar-Temperatur > Pool-Temperatur + 5°C.
     - **Timer-Modus:** Pumpe läuft täglich von 8:00–20:00 Uhr.

2. **Daten visualisieren**
   - **Grafana:** Importiere das [Grafana-Dashboard](https://github.com/smart-swimmingpool/grafana-dashboard) für Temperaturverläufe.
   - **Home Assistant:** Erstelle ein **Lovelace-Dashboard** mit allen Sensoren.

3. **Erweiterungen**
   - **Wasserqualitätsmodul:** (Falls verfügbar) zur Überwachung von pH/Wert und Chlorid.
   - **Weitere Sensoren:** Drucksensoren, Durchflusssensoren.

4. **Community beitreten**
   - **GitHub Discussions:** [smart-swimmingpool/pool-controller/discussions](https://github.com/smart-swimmingpool/pool-controller/discussions)
   - **Issues melden:** Falls du Probleme hast, erstelle ein [Issue](https://github.com/smart-swimmingpool/pool-controller/issues).

---

## 🆘 Häufige Probleme & Lösungen (FAQ)

| **Problem**                          | **Lösung**                                                                                     |
|--------------------------------------|-------------------------------------------------------------------------------------------------|
| **Controller startet nicht**         | Prüfe USB-Kabel und Stromversorgung. Prüfe, ob die **Status-LED leuchtet**.                  |
| **WiFi verbindet sich nicht**         | Prüfe WiFi-Passwort. Starte im **AP-Modus** neu und konfiguriere WiFi.                        |
| **MQTT verbindet sich nicht**        | Prüfe **MQTT-Broker-IP/Port**. Prüfe, ob der Broker läuft (`sudo systemctl status mosquitto`). |
| **Sensoren werden nicht erkannt**    | Prüfe **Pull-Up-Widerstände** (4.7kΩ). Prüfe **GPIO-Pins** in `Config.hpp`.                     |
| **Relays schalten nicht**            | Prüfe **Aktiv-High/Aktiv-Low**. Prüfe **Stromversorgung des Relay-Moduls (5V)**.             |
| **Home Assistant erkennt Controller nicht** | Prüfe **MQTT Discovery** (ab v3.3.0). Lösche alte Homie-Nachrichten vom Broker.         |

**📌 Ausführliche FAQ findest du hier:** [FAQ: Häufige Probleme & Lösungen](faq.md)

---

## 📚 Weitere Ressourcen

- [Hardware Guide](hardware-guide.md) – Detaillierte Schaltpläne & Pin-Belegung
- [User Guide](users-guide.md) – Bedienung des Web-Interfaces
- [MQTT Configuration](mqtt-configuration.md) – MQTT-Einrichtung & Home Assistant Integration
- [Sicherheitshinweise](safety.md) – Wichtige Sicherheitsregeln für 230V-Anlagen
- [GitHub Discussions](https://github.com/smart-swimmingpool/pool-controller/discussions) – Community-Hilfe
- [GitHub Issues](https://github.com/smart-swimmingpool/pool-controller/issues) – Fehler melden
