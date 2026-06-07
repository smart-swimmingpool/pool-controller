---
title: Hardware-Anleitung
summary: Schritt-für-Schritt-Hardware-Anleitung für den ESP32-Pool-Controller — Teileliste mit Bezugsquellen, Schaltplan für DS18B20-Sensoren und Relaismodul, Löt- und Fertigungstipps, Stromversorgung und Inbetriebnahme
date: "2026-06-07"
lastmod: "2026-06-07"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "hardware", "anleitung", "schaltplan", "aufbau", "loeten"]
menu:
  docs:
    parent: Pool Controller
    name: Hardware-Anleitung
    weight: 20
---

## Übersicht

Diese Anleitung führt dich Schritt für Schritt durch den Hardware-Aufbau des
Pool-Controllers — von der Teileauswahl bis zur fertigen Schaltung. Auch wenn
du noch nie gelötet hast, helfen dir die detaillierten Erklärungen, alles
richtig zusammenzubauen.

> **Zielgruppe**: Elektronik-Bastler mit grundlegenden Lötkenntnissen.
> Gesamtkosten: **unter 100€** (ohne Pumpen und Pool-Infrastruktur).

## Sicherheitshinweise ⚠️

- Der Controller schaltet **230V Netzspannung** für die Pumpen. Fehlerhafte
  Verdrahtung kann zu Stromschlag oder Brand führen.
- Arbeite nur **spannungsfrei** an der Schaltung.
- Verwende einen **FI-Schutzschalter (RCD)** für den Pumpenstromkreis.
- Sensorleitungen (Niederspannung) und Netzleitungen **räumlich getrennt** führen.
- Im Zweifel die Netzinstallation von einem **Elektrofachbetrieb** prüfen lassen.

---

## Benötigte Teile (BOM)

| # | Bauteil | Menge | ca. Preis | Hinweise |
|---|---------|:-----:|:---------:|----------|
| 1 | ESP32-Entwicklungsboard (z.B. ESP32 DevKit V1, NodeMCU-32S) | 1 | 10–15€ | Mindestens 4MB Flash |
| 2 | DS18B20-Temperatursensor (wasserfest, Edelstahl, 1m Kabel) | 2 | 8–12€ | Einer für Poolwasser, einer für Solarkollektor |
| 3 | 2-Kanal-5V-Relaismodul (mit Optokoppler) | 1 | 5–8€ | **Muss** active-high sein (siehe Hinweise) |
| 4 | Widerstand 4,7kΩ (¼W oder ⅛W, Metall- oder Kohleschicht) | 2 | < 1€ | Pull-up für die OneWire-Datenleitungen |
| 5 | Breadboard + Jumper-Kabel (zum Testen) **ODER** Lochrasterplatine + Stiftleisten/Schraubklemmen (für Daueraufbau) | 1 | 3–8€ | |
| 6 | USB-Netzteil 5V/≥1A (z.B. Handy-Ladegerät) | 1 | 5–10€ | Für den ESP32 allein |
| 7 | Schaltdraht, 0,14–0,5mm², verschiedene Farben | — | 3–5€ | |
| 8 | Optional: Gehäuse (ABS/PVC-Projektbox, IP54 oder besser) | 1 | 5–10€ | Schutz vor Spritzwasser/Staub |
| 9 | Optional: Schraubklemmen (2-polig, 5mm Raster) | 4–6 | 2–3€ | Für steckbare Sensor-/Stromanschlüsse |
| 10 | Optional: DS3231 RTC-Modul | 1 | 3–5€ | Backup-Uhr (nicht nötig bei NTP-Betrieb) |
| **Gesamt** | | | **~45–75€** | Ohne Gehäuse; deutlich unter 100€ |

### Bezugsquellen

Alle Teile gibt es bei Amazon, AliExpress, eBay oder bei Elektronik-Distributoren
wie:

- **Reichelt** (reichelt.de) — zuverlässig, schneller Versand in DE/AT
- **Pollin** (pollin.de) — günstige Komponenten
- **Conrad** (conrad.de) — großes Sortiment, etwas teurer
- **Amazon** — bequem, aber oft Aufpreis

Suchbegriffe:
- **ESP32**: "ESP32 DevKit V1" oder "ESP32 NodeMCU-32S". Kein ESP32-S2/S3/C3
  kaufen — die Firmware läuft auf dem Standard-ESP32 (Xtensa Dual-Core).
- **DS18B20**: **Edelstahl, wasserdicht** mit 1m Kabel. Günstige
  Kunststoff-Varianten gehen auch, sind aber weniger haltbar im Außeneinsatz.
- **Relaismodul**: **Achtung: Logikpegel prüfen!** Die Firmware schaltet per
  **active-high** (GPIO HIGH → Relais EIN). Viele günstige Module sind
  active-low (mit Jumper zum Umschalten). Vor dem Festlöten prüfen.

---

## Pin-Belegung (Firmware-Standard)

Die Firmware verwendet diese GPIO-Pins (definiert in `src/Config.hpp`):

| Konstante | GPIO | Zweck |
|-----------|:----:|-------|
| `PIN_DS_SOLAR` | **GPIO15** | DS18B20 Daten — Solarkollektor-Temperatur |
| `PIN_DS_POOL` | **GPIO16** | DS18B20 Daten — Pool-Wassertemperatur |
| `PIN_RELAY_POOL` | **GPIO18** | Relais-Ansteuerung — Pool-Umwälzpumpe |
| `PIN_RELAY_SOLAR` | **GPIO19** | Relais-Ansteuerung — Solar-Heizungspumpe |

> **Hinweis zu GPIO15**: Dies ist ein **Strapping-Pin** am ESP32 — er
> beeinflusst den Boot-Vorgang, wenn er beim Start HIGH oder LOW anliegt. Die
> Firmware funktioniert zuverlässig mit dieser Belegung. Falls du Boot-Probleme
> hast, kannst du die DS18B20-Sensoren auf **GPIO32/33** legen (siehe
> [Verdrahtungsoptimierung](#optional-verdrahtungsoptimierung)).

---

## Schaltplan

```
                         +-------------------------+
                         |         ESP32           |
                         |                         |
  DS18B20 SOLAR DATA --->| GPIO15   (PIN_DS_SOLAR) |
  DS18B20 POOL  DATA --->| GPIO16   (PIN_DS_POOL)  |
  Relais IN1 (Pool) ---->| GPIO18   (PIN_RELAY_POOL)|
  Relais IN2 (Solar) --->| GPIO19   (PIN_RELAY_SOLAR)|
                         |                         |
  3,3V ------------------>| 3V3                     |
  GND ------------------->| GND                     |
                         +-----------+-------------+
                                     |
                                     | gemeinsame Masse
                                     v
         +--------+--------+--------+--------+
         |        |        |        |        |
    +----+--+  +--+----+  |   +----+--+  +--+----+
    |Solar  |  | Pool  |  |   | RLY1  |  | RLY2  |
    |DS18B20|  |DS18B20|  |   |(Pool) |  |(Solar)|
    |       |  |       |  |   |       |  |       |
    | VDD --+--+ VDD --+--+---> 3,3V  |  |       |
    | GND --+--+ GND --+-------> GND  |  |       |
    |DATA --->| GPIO15|  |   |       |  |       |
    |       |  |DATA -->|  |   | VCC  |  | VCC  |
    +-------+  +-------+  |   | -- 5V |  | -- 5V|
                           |   | GND  |  | GND  |
                    4,7kΩ  |   |      |  |      |
                     |     |   | IN1  |  | IN2  |
                     v     |   | <-18 |  | <-19 |
                3,3V ----+-+--- 3,3V
```

---

## Schritt-für-Schritt-Verdrahtung

### 1. Temperatursensoren (DS18B20)

Der DS18B20 hat drei Adern (bei wasserdichten Fühlern: **rot = VDD**,
**gelb/weiß = DATA**, **schwarz = GND** — **immer mit Datenblatt deines
Sensors vergleichen!**):

| DS18B20-Ader | Anschluss |
|:------------:|-----------|
| Rot (VDD) | ESP32 **3,3V** |
| Schwarz (GND) | ESP32 **GND** |
| Gelb/Weiß (DATA) | **GPIO15** (Solar) oder **GPIO16** (Pool) |

**Wichtig — den Pull-Up-Widerstand nicht vergessen:**

Löte einen **4,7kΩ-Widerstand** zwischen die DATA-Leitung und **3,3V**.
Einen Widerstand pro Sensor, möglichst nahe am Sensor-Anschluss.

```
    ESP32 3,3V ──┬── 4,7kΩ ──── DS18B20 DATA
                  │
                DS18B20 VDD
```

Ohne diesen Widerstand wird der Sensor nicht erkannt — das ist die **häufigste
Fehlerquelle** bei "Sensor nicht gefunden"-Meldungen.

### 2. Relaismodul

| Relaismodul | Anschluss |
|:-----------:|-----------|
| VCC (oder VDD) | **5V** (vom ESP32-VIN-Pin oder externem 5V-Netzteil) |
| GND | **GND** (gemeinsam mit ESP32) |
| IN1 | **GPIO18** (Pool-Pumpe) |
| IN2 | **GPIO19** (Solar-Pumpe) |

**Relais-Logik**: Die Firmware setzt den GPIO-Pin auf HIGH (3,3V) um das
Relais zu aktivieren. Wenn dein Modul bei LOW schaltet (active-low), den
Jumper umstecken oder die Firmware anpassen (siehe `src/RelayModuleNode.cpp`).

**Lastseite (230V-Seite):**

```
    L ──┤ FI ├──┤ LS ├──┬──┤ Relais-KOM1 ├── Pool-Pumpe ── N
                          └──┤ Relais-KOM2 ├── Solar-Pumpe ── N
```

- Den Außenleiter (L) der Pumpe an den **COM**-Kontakt (Mitte) des Relais.
- Den **NO**-Kontakt (Schließer, "normally open") mit der Pumpe verbinden.
- Den anderen Pumpenanschluss an **Neutralleiter (N)**.
- Den 230V-Stromkreis **immer** über einen FI-Schutzschalter und einen
  passend abgesicherten Leitungsschutzschalter führen.

### 3. Stromversorgung

| Komponente | Spannung | Quelle |
|-----------|:--------:|--------|
| ESP32-Board | **5V USB** (stabilisiert) | Handy-Ladegerät, USB-Port oder 5V-Netzteil |
| Relaismodul (Spulen) | **5V** | Vom ESP32-VIN-Pin oder externem 5V-Netzteil |
| DS18B20-Sensoren | **3,3V** | Vom ESP32-3,3V-Ausgang |
| (Optional) RTC DS3231 | **3,3V** | Vom ESP32-3,3V-Ausgang |

> **Wichtig**: Der eingebaute 3,3V-Spannungsregler des ESP32 liefert ca. 600mA.
> Die DS18B20-Sensoren brauchen zusammen < 5mA — völlig unkritisch. Wenn du
> viele zusätzliche 3,3V-Komponenten anschließt, einen externen 3,3V-Regler
> verwenden.

---

## Aufbau-Schritte

### Testaufbau auf dem Breadboard

1. **ESP32 aufstecken** — mittig auf dem Breadboard, über die Mittellücke
2. **Stromschienen verbinden** — 3,3V und GND auf beiden Seiten
3. **Pull-Up-Widerstände einstecken** — zwischen DATA-Reihe und 3,3V-Schiene
4. **DS18B20-Sensoren anschließen** — Jumper-Kabel für VDD (3,3V), GND und DATA
5. **Relaismodul anschließen** — Jumper für VCC (5V), GND, IN1, IN2
6. **Stromversorgung per USB** — Der VIN/USB-Pin des ESP32 liefert 5V fürs
   Relaismodul
7. **Alle Verbindungen prüfen** bevor du Spannung anlegst
8. **Test**: Siehe [Inbetriebnahme](#inbetriebnahme-und-test)

{{< figure
  library="true"
  src="../pool-controller_breadboard.png"
  title="Pool-Controller Breadboard-Aufbau mit ESP32, zwei DS18B20-Temperatursensoren, 4,7kΩ-Pull-Up-Widerständen und 2-Kanal-Relaismodul"
  lightbox="true" >}}

> 💡 **Tipp**: Unterschiedliche Jumper-Farben verwenden — z.B. rot für
> Spannung (3,3V/5V), schwarz für Masse (GND), gelb für Sensordaten, blau für
> Relais-Steuerung.

### Dauerhafter Aufbau auf Lochrasterplatine

Wenn der Testaufbau auf dem Breadboard funktioniert, baust du die endgültige
Version:

1. **Layout planen**: Komponenten vor dem Löten auf der Platine anordnen.
   230V-Relais-Anschlüsse an einer Kante, Sensoranschlüsse an der gegenüber-
   liegenden Seite.
2. **In dieser Reihenfolge löten**:
   - Stiftleisten / Schraubklemmen für den ESP32 (fasse ihn in eine Sockelleiste,
     löte nicht direkt auf die Platine)
   - Widerstände (4,7kΩ)
   - Stiftleisten fürs Relaismodul (auch hier sockeln)
   - Schraubklemmen für Sensoranschlüsse
3. **Verbindungen führen**: Massivdraht für die Verbindungen. Datenleitungen
   kurz halten.
4. **Auf Lötzinnbrücken prüfen**: Jede Lötstelle mit der Lupe oder Multimeter
   (Durchgangsprüfung) kontrollieren.
5. **Ins Gehäuse einbauen**: Abstandsbolzen oder doppelseitiges Klebeband
   verwenden. Löcher für Sensor- und Relaiskabel bohren. **Kabelverschraubungen**
   (PG7/PG9) für wasserdichte Kabeldurchführung nutzen.

---

## Fertigungstipps

### Löten

- **Blei-Lot verwenden** (Sn60Pb40 oder Sn63Pb37) — es fließt besser als
  bleifreies Lot, besonders für Anfänger. (In DE/AT für Hobbyzwecke weiterhin
  erlaubt und empfohlen.)
- **Flussmittel**: Flussmittelkern-Lot nehmen; bei widerspenstigen Lötstellen
  zusätzlich Flüssig-Flussmittel.
- **Temperatur**: Lötkolben auf 320–350°C (Bleilot) oder 370–400°C (bleifrei)
  einstellen.
- **Spitze sauber halten**: Vor **jeder** Lötstelle an feuchtem Schwamm oder
  Messingwolle reinigen.
- **Gute Lötstelle**: Glänzend, konkave Verrundung, die um den Draht herum
  fließt. Eine matte, rissige oder kugelförmige Stelle ist **schlecht** —
  nachwärmen und frisches Lot zugeben.

### Gehäuse

- **ABS- oder PVC-Projektbox** mit mindestens IP54-Schutzart für den
  Außeneinsatz (Spritzwasserschutz).
- **Lüftungsschlitze** bohren, wenn das Relaismodul warm wird — aber nach
  unten gerichtet, damit kein Wasser eindringt.
- ESP32 auf **M2,5- oder M3-Nylon-Abstandsbolzen** montieren, um
  Kurzschlüsse zu vermeiden.
- Außenanschlüsse beschriften (Pool-Sensor, Solar-Sensor, Pumpe 1, Pumpe 2,
  USB-Strom).
- **Kabelverschraubungen** (PG7 für dünne Sensorkabel, PG9 für dickere
  Stromkabel) an den Kabeleinführungen verwenden.

### Kabelmanagement

- **Beide Enden jedes Kabels beschriften** — mit Schrumpfschlauch-Etiketten
  oder kleinen Klebeetiketten. Zukünftiges Du wird es dir danken.
- **Sensor- und Relaiskabel getrennt führen** im Gehäuse, um
  Einstreuungen zu minimieren.
- **Zugentlastung**: Kabel an Montagepunkten oder mit Kabelbindern fixieren,
  damit Zug an den Außenkabeln nicht die Lötstellen belastet.
- **Service-Schlaufe**: Genug Drahtreserve im Gehäuse lassen, damit du es
  öffnen und arbeiten kannst ohne alles zu trennen.

### Sensoren im Außenbereich

- **DS18B20-Fühler** sind wasserdicht, aber der Kabeleingang am Sensor ist
  nicht immer dicht. **Schrumpfschlauch** über die Kabelverbindung ziehen
  oder **selbstverschmelzendes Silikonband** für Außeninstallationen.
- Sensorkabel in **Leerrohr** (PVC oder flexibel) verlegen, wo sie
  mechanisch belastet werden (Rasen, Einfahrt).
- **Maximale Kabellänge**: DS18B20 funktioniert zuverlässig bis ca. 30m mit
  4,7kΩ-Pull-up und verdrilltem Kabel. Bei längeren Strecken den Pull-up auf
  2,2kΩ verringern oder einen OneWire-Treiber verwenden.
- Pool-Sensor in den **Pumpenkreislauf** einbauen (nach dem Filter, vor dem
  Rücklauf) für eine genaue Durchschnittstemperatur.
- Solar-Sensor an der **heißesten Stelle des Solarkollektors** (meist das
  obere Austrittsrohr) anlegen.

---

## Stromversorgungs-Optionen

| Option | Vorteile | Nachteile |
|--------|----------|-----------|
| **USB-Handy-Ladegerät** (5V/1A+) | Günstig, überall verfügbar, sicher | Begrenzter Strom für Zusatzgeräte |
| **Hutschienen-Netzteil** (Mean Well HDR-15-5 o.ä.) | Professionell, zuverlässig, passt in Verteiler | Etwas teurer (~15€) |
| **ESP32-VIN via USB** + Relais von derselben 5V | Einfache Verdrahtung | Gesamtstrom muss unter Grenze des ESP32-Boards bleiben |

**Empfehlung**: Wenn du einen festen Installationsort in der Nähe deiner
Pumpensteuerung hast, nimm ein **Hutschienen-5V-Netzteil** (z.B. Mean Well
HDR-15-5). Das ist sauber, zuverlässig und versorgt ESP32 und Relaismodul
problemlos.

---

## Inbetriebnahme und Test

### 1. Sichtprüfung

Vor dem Anlegen der Spannung:
- Auf **Lötbrücken** zwischen benachbarten Pins prüfen
- **Polung** aller Komponenten kontrollieren (DS18B20 VDD/GND, Relais VCC/GND)
- **Keine losen Drahtenden**, die benachbarte Pins kurzschließen
- Widerstand zwischen **3,3V und GND** messen — sollte > 10kΩ sein (kein Kurzschluss)

### 2. Einschalten

1. USB-Strom (oder 5V-Netzteil) anschließen
2. Die **eingebaute LED** des ESP32 blinkt schnell → Boot-Vorgang
3. Nach ca. 3 Sekunden blinkt die LED **langsam** → wartet auf WiFi

### 3. Sensoren prüfen

Serielles Monitor öffnen (9600 Baud):
```
Pool Controller v3.3.0
Starting up...
Initialized pins: GPIO15, GPIO16, GPIO18, GPIO19
Solar Temp: GPIO15
Pool Temp:  GPIO16
Pool Relay: GPIO18
Solar Relay: GPIO19
```

Wenn die Sensoren angeschlossen sind und funktionieren:
```
Solar temperature: 25.3°C
Pool temperature:  22.1°C
```

Bei `Sensor error` oder `-127°C`, prüfe:
- [ ] 4,7kΩ-Pull-up-Widerstand auf jeder DATA-Leitung vorhanden?
- [ ] DS18B20 VDD an 3,3V (nicht 5V)?
- [ ] DS18B20 GND an gemeinsamer Masse?
- [ ] DATA-Pin stimmt mit Firmware-Konfiguration überein?

### 4. Relais testen

Über das Web-UI (Reiter Konfiguration) oder per seriellem Befehl:
```
Mode: manual
Pool pump: ON  → Relais klickt hörbar, Pumpe läuft an
Solar pump: ON → Relais klickt hörbar, Pumpe läuft an
```

Wenn das Relais nicht klickt:
- [ ] Ist das Relaismodul mit Strom versorgt (5V zwischen VCC und GND)?
- [ ] Stimmt der Logikpegel (active-high vs. active-low)?
- [ ] Leuchtet die LED auf dem Relaismodul, wenn GPIO auf HIGH geht?

---

## Fehlersuche

| Symptom | Wahrscheinliche Ursache | Lösung |
|---------|------------------------|--------|
| "Sensor error" oder `-127°C` | Pull-up-Widerstand fehlt | 4,7kΩ zwischen DATA und 3,3V einlöten |
| "Sensor error" | Falscher GPIO-Pin | `PIN_DS_SOLAR`/`PIN_DS_POOL` in `src/Config.hpp` prüfen |
| Unbeständige Messwerte | Wackelkontakt oder Einstreuungen | Lötstellen prüfen, Daten- und Relaisleitungen trennen |
| Relais schaltet nicht | Falscher Logikpegel | Active-high vs. active-low prüfen; Jumper umstecken |
| Relais klickt, Pumpe läuft nicht | 230V-Verdrahtungsfehler | COM/NO-Kontakte prüfen, Pumpenanschluss kontrollieren |
| ESP32 startet nicht (Brownout) | Zu schwache Stromversorgung | 5V/≥1A-Netzteil verwenden; 100µF-Kondensator nahe VIN |
| ESP32 resetet beim Relais-Schalten | Spannungsspitze an der Relais-Spule | Freilaufdiode parallel zur Spule, oder Modul mit eingebautem Schutz |
| Messwerte springen beim Schalten | Elektrisches Rauschen | Sensorleitungen getrennt von Relais-/Stromkabeln führen |

---

## Optional: Verdrahtungsoptimierung

Die Standard-Pins (GPIO15/16/18/19) funktionieren bei den meisten Anwendern
zuverlässig. Falls du Boot-Probleme hast oder die robusteste Konfiguration
möchtest, verwende diese **optimierten Pins**:

| Funktion | Standard-Pin | Optimierter Pin | Grund |
|----------|:------------:|:----------------:|-------|
| DS18B20 Solar | GPIO15 | **GPIO32** | GPIO15 ist ein Strapping-Pin — OneWire da wegnehmen eliminiert jedes Boot-Risiko |
| DS18B20 Pool | GPIO16 | **GPIO33** | Saubere Trennung vom verbleibenden Strapping-Pin GPIO0 |
| Relais Pool | GPIO18 | **GPIO25** | ADC2-Pins (GPIO18/19) werden vermieden; GPIO25 ist sauberer Digitalausgang |
| Relais Solar | GPIO19 | **GPIO26** | Gleicher Grund wie oben |

Für die optimierten Pins in `src/Config.hpp` anpassen:

```cpp
constexpr uint8_t PIN_DS_SOLAR{32};     // war 15
constexpr uint8_t PIN_DS_POOL{33};      // war 16
constexpr uint8_t PIN_RELAY_POOL{25};   // war 18
constexpr uint8_t PIN_RELAY_SOLAR{26};  // war 19
```

Diese optimierte Pinbelegung ist bereits im Dokument
[ESP32 Schaltplananalyse und Optimierung](esp32-schematic-optimization-de.md)
ausführlich analysiert und begründet.

---

## Referenzen

- Fritzing-Quelldatei: [pool-controller.fzz](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)
- [ESP32 Schaltplananalyse und Optimierung](esp32-schematic-optimization-de.md)
- [ESP32 Komplett-Schaltplan](esp32-complete-wiring-schematic-de.md)
- [DS18B20 Datenblatt](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf)
- [ESP32 Pin-Referenz](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [Config.hpp Pin-Quelle](https://github.com/smart-swimmingpool/pool-controller/blob/main/src/Config.hpp)
