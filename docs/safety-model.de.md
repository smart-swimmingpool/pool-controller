---
title: Sicherheitsmodell
summary: System-Sicherheitsarchitektur des Pool-Controllers — mehrschichtiger Sicherheitsansatz mit Hardware-Failsafes, Firmware-Schutzmaßnahmen, Software-Sicherungen und Betriebsanweisungen
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "sicherheit", "architektur", "zuverlässigkeit"]
menu:
  docs:
    parent: Pool Controller
    name: Sicherheitsmodell
    weight: 140
---

> ⚠️ **HINWEIS**: Dieses Dokument beschreibt die Sicherheitsarchitektur des
> Pool-Controllers. Praktische Anleitungen zur elektrischen Sicherheit beim
> Bau des Controllers finden sich in [Elektrische Sicherheit](electrical-safety.de.md).

## Überblick

Der Pool-Controller ist nach dem **Defense-in-Depth**-Prinzip (mehrschichtige
Sicherheitsarchitektur) aufgebaut. Sicherheit ist keine einzelne Funktion,
sondern ein mehrschichtiges System aus Hardware, Firmware und Betriebsanweisungen.

```text
┌──────────────────────────────────────────────┐
│            Betriebsebene                      │
│  (Benutzerverfahren, Checklisten, Wartung)   │
├──────────────────────────────────────────────┤
│            Software-Ebene                     │
│  (Watchdog, Bootloop-Erkennung, Safe Mode)   │
├──────────────────────────────────────────────┤
│            Firmware-Ebene                     │
│  (Konfigurationsprüfung, Typsicherheit, CRC) │
├──────────────────────────────────────────────┤
│            Hardware-Ebene                     │
│  (Relais-Trennung, Sicherungen, IP65-Gehäuse)│
└──────────────────────────────────────────────┘
```

---

## Ebene 1: Hardware-Sicherheit

### Relais-Trennung

- Relais bieten **galvanische Trennung** zwischen ESP32 (3,3V/5V) und
  Netzspannung (230V AC)
- Mindestens 4 mm Kriechstrecke zwischen Spule und Kontakten bei
  Qualitätsmodulen
- Relaismodule mit Optokoppler-Eingang bieten zusätzliche Isolation

### Überstromschutz

- Ein **Leitungsschutzschalter** muss auf dem Außenleiter installiert werden
- Ausgelegt für die Pumpenlast (siehe [Elektrische Sicherheit](/docs/electrical-safety/))
- Schützt vor Kurzschluss und Überlast

### Gehäuse

- IP65-Gehäuse schützt vor Wasserstrahlen und Staub
- Kabelverschraubungen sorgen für Zugentlastung und erhalten die IP-Schutzart
- Physische Trennung von Netz- und Niederspannungsleitungen im Gehäuse

### Ausfallsicheres Relais-Verhalten

- Relais sind **Normally Open (NO)** — bei Spannungsverlust des ESP32 fallen
  die Relais ab und die Pumpen stoppen
- Verhindert unkontrollierten Pumpenbetrieb bei Controller-Ausfall

---

## Ebene 2: Firmware-Sicherheit

### Konfigurationsprüfung

Alle Konfigurationswerte werden vor der Übernahme validiert:

| Parameter               | Prüfung            | Bereich              |
| ----------------------- | ------------------ | -------------------- |
| Temperatur-Schwellwerte | Min-/Max-Grenzen   | 0–60 °C              |
| Timer-Werte             | 24h-Format         | 00:00–23:59          |
| WLAN-Einstellungen      | SSID-Längenprüfung | 1–32 Zeichen         |
| MQTT-Einstellungen      | Hostname-Format    | Gültiger Hostname/IP |
| Relais-Konfiguration    | Enum-Prüfung       | `true` / `false`     |

Ungültige Konfigurationen werden **zurückgewiesen** und der vorherige Wert
bleibt erhalten.

### NVS-CRC-Schutz

Die im ESP32-NVS gespeicherte Konfiguration enthält eine CRC32-Prüfsumme. Bei
Fehlern wird die Konfiguration auf Werkseinstellungen zurückgesetzt.

### Typsicherheit

Die Firmware verwendet stark typisierte Enums und Structs anstelle von
allgemeinen Integern für die Betriebsartenauswahl — dies reduziert das Risiko
ungültiger Zustände.

### Task-Überwachung

Jeder FreeRTOS-Task überwacht seine eigene Stack-Nutzung. Wenn ein Task den
konfigurierten Stack-Wasserstand überschreitet, wird eine Warnung protokolliert
und das System kann Korrekturmaßnahmen ergreifen.

---

## Ebene 3: Software-Sicherheit

### Hardware-Watchdog (WDT)

- ESP32-Hardware-Watchdog mit 30-Sekunden-Timeout
- Reset, wenn die Hauptschleife länger als 30 Sekunden hängt
- Automatisch nach jedem Boot aktiviert
- Stellt sicher, dass der Controller nicht unbegrenzt hängen bleibt

### Bootloop-Erkennung

- NVS-basierter Boot-Zähler erhöht sich bei jedem Boot
- Zähler wird nach einem erfolgreichen 5-Minuten-Lauf zurückgesetzt
- **4 aufeinanderfolgende kurze Boots** lösen den Safe Mode aus
- Im Safe Mode:
  - Alle Relais werden ausgeschaltet
  - Web-UI bleibt erreichbar
  - Serielles Log zeigt Safe-Mode-Indikator
  - Konfiguration kann geprüft und korrigiert werden

### Speicherüberwachung

- Freier Heap wird alle 10 Sekunden geprüft
- Bei **kritischem Schwellwert** (8 KB freier Heap):
  - Warnung wird protokolliert
  - Graceful-Auto-Reboot wird eingeleitet
- Bei **Warnschwellwert** (15 KB freier Heap):
  - Warnung wird protokolliert
  - Kein Neustart — Überwachung läuft weiter

### Sensor-Auto-Wiederherstellung

- Fehlerhafter DS18B20-Leseversuch löst schnelles Nachfragen aus (5s statt 300s)
- Nach 3 erfolgreichen Leseversuchen zurück zum normalen Intervall
- Verhindert unnötige Warnungen durch kurzzeitige Sensor-Aussetzer

### NTP-Graceful-Degradation

Dreistufige Fallback-Logik für die Zeitsynchronisation:

1. **Primär**: NTP-Server antwortet → normaler Betrieb
2. **Degradiert**: NTP fehlschlägt → letzte bekannte Zeit verwenden
3. **Safe**: Keine Zeit verfügbar → millis()-Laufzeit mit Warnung

---

## Ebene 4: Betriebssicherheit

### Inbetriebnahmeverfahren

- **Steckbrett-Test** vor Netzanschluss (siehe [Von Null aufgebaut](/docs/build-from-zero/))
- **Relais-Leerlauftest** vor Pumpenanschluss
- **Erster Netz-Einschalt**: 30-minütiger überwachter Betrieb

### Wartungsplan

- Monatlich: Temperaturen prüfen, Warnungen kontrollieren
- Jährlich: Verkabelung prüfen, RCD testen, Gehäusedichtungen kontrollieren

### Checklisten

- [Produktions-Checkliste](/docs/production-checklist/) — vor Inbetriebnahme
- [Sicherheits-Checkliste](/docs/security-checklist/) — Sicherheitshärtung

---

## Fehlerfall-Analyse

| Fehlerfall           | Auswirkung               | Sicherheitsebene         | Maßnahme                    |
| -------------------- | ------------------------ | ------------------------ | --------------------------- |
| ESP32-Absturz        | Relais aus, Pumpen stopp | Hardware                 | Failsafe-Relais (NO)        |
| Software-Hang        | Keine Relais-Updates     | WDT                      | 30s-Reset                   |
| Konfigurationsfehler | Ungültige Einstellungen  | Firmware                 | NVS CRC → Werksreset        |
| Wiederholte Abstürze | Unsicherer Zustand       | Bootloop                 | Safe Mode → Relais AUS      |
| Heap-Erschöpfung     | Unvorhersehbar           | Speicher-Monitor         | Graceful-Reboot             |
| Sensor-Ausfall       | Keine Temperaturdaten    | Sensor-Wiederherstellung | Schnelles Nachfragen        |
| Stromausfall         | Controller aus           | Hardware                 | Relais standardmäßig AUS    |
| Netzwerkausfall      | Keine Fernsteuerung      | Software                 | Lokales Web-UI funktioniert |

---

## Verwandte Dokumente

- [Elektrische Sicherheit](/docs/electrical-safety/) — Sicherheitsinformationen
- [Produktions-Checkliste](/docs/production-checklist/) — Prüfungen vor Inbetriebnahme
- [Sicherheits-Checkliste](/docs/security-checklist/) — Sicherheitshärtung
- [Von Null aufgebaut](/docs/build-from-zero/) — Komplette Bauanleitung
