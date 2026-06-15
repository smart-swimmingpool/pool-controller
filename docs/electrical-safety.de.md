---
title: Elektrische Sicherheit
summary: Elektrische Sicherheitsinformationen für den Pool-Controller — 230V AC-Netzinstallation, Relais-Trennung, Absicherung, Gehäuseanforderungen und sichere Inbetriebnahme
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "sicherheit", "elektrik", "hardware", "inbetriebnahme"]
menu:
  docs:
    parent: Pool Controller
    name: Elektrische Sicherheit
    weight: 130
---

> ⚠️ **WARNUNG: Dieses Dokument beschreibt die Installation von 230V AC-Netzspannung.**
> Fehlerhafte Verdrahtung kann zu Stromschlag, Brand oder Geräteschäden führen.
>
> - Wenn du keine Erfahrung mit Netzspannungsinstallationen hast, beauftrage
>   eine qualifizierte Elektrofachkraft
> - Diese Anleitung dient der Information — örtliche Elektrovorschriften können
>   eine zertifizierte Installation vorschreiben
> - Dies ist ein DIY-Projekt — nicht CE/UL-zertifiziert für den kommerziellen Einsatz

## Überblick

Der Pool-Controller schaltet **230V AC-Netzspannung** für Pool- und
Solarpumpen. Unsachgemäße Installation birgt Risiken von Stromschlag, Brand
oder Geräteschäden. Bitte vor dem Anschließen der Netzspannung sorgfältig lesen.

> **Wenn du keine Erfahrung mit Netzspannungsinstallationen hast, beauftrage
> eine qualifizierte Elektrofachkraft.** Diese Anleitung dient der Information
> — örtliche Elektrovorschriften können eine zertifizierte Installation
> vorschreiben.

---

## Gefährdungsbeurteilung

| Gefahr        | Risikostufe | Beschreibung                                               |
| ------------- | ----------- | ---------------------------------------------------------- |
| Stromschlag   | Hoch        | Direkter Kontakt mit 230V AC kann tödlich sein             |
| Brand         | Mittel      | Überlastete oder lose Verbindungen können sich entzünden   |
| Geräteschaden | Mittel      | Falsche Verdrahtung kann Controller oder Pumpen zerstören  |
| Feuchtigkeit  | Hoch        | Pool-Umgebung hat hohe Luftfeuchte und Spritzwasser-Risiko |

---

## Allgemeine Sicherheitsregeln

1. **Immer stromlos schalten** vor Arbeiten am Controller oder den Pumpen
2. **Fehlerstromschutzschalter (RCD/FI)** für den Controller-Stromkreis verwenden
3. **Nie nass arbeiten** — trockene Hände und trockene Umgebung sicherstellen
4. **Passende Absicherung** für den Pumpenstrom wählen
5. **Netz- und Niederspannungsleitungen getrennt führen** im Gehäuse
6. **Alle Verbindungen zugentlasten**
7. **Alle Kabel eindeutig beschriften**

---

## Netzspannungs-Verdrahtung

### Relais-Ausgangsverdrahtung

Das Relaismodul schaltet nur den **Außenleiter (Phase)**. Der Neutralleiter
und Schutzleiter werden direkt zur Pumpe geführt.

```text
AC Netz          Relaismodul           Pumpe
┌────────┐     ┌──────────────┐     ┌─────┐
│ Phase  ──────┤ COM    NO    ──────┤ L   │
│ (230V) │     │              │     │     │
│ Neutr. ───────────────────────────┤ N   │
│        │     │              │     │     │
│ Erde   ───────────────────────────┤ PE  │
└────────┘     └──────────────┘     └─────┘
```

### Leitungsquerschnitte

| Pumpenleistung | Min. Leitungsquerschnitt | Absicherung |
| -------------- | ------------------------ | ----------- |
| ≤ 500 W        | 1,5 mm²                  | 6 A         |
| 500–1000 W     | 1,5 mm²                  | 10 A        |
| 1000–2000 W    | 2,5 mm²                  | 16 A        |

### Absicherung

- Einen Leitungsschutzschalter entsprechend der Pumpenlast installieren
- Die Sicherung muss auf dem **Außenleiter** vor dem Controller sitzen
- Hutschienen- oder Aufbaumontage-Sicherungshalter verwenden

---

## Niederspannungs-Verdrahtung

### ESP32 und Sensor-Verdrahtung

- ESP32 arbeitet mit 3,3V / 5V (USB-Stromversorgung)
- DS18B20-Sensoren verwenden 3,3V — max. Kabellänge: 10 m
- Sensorkabel von Netzleitungen getrennt führen (min. 5 cm Abstand)
- Geschirmte, verdrillte Zweidrahtleitung für lange Sensorstrecken

### Spannungsversorgung

- **Geregeltes** 5V-Netzteil verwenden (kein rohes Steckernetzteil)
- Mindestens 1A für ESP32 + Relaismodul
- Für 12V-Pool-Systeme: 12V→5V DC-DC-Wandler verwenden

---

## Gehäuseanforderungen

### IP-Schutzart

| Standort                  | Min. IP-Schutzart | Anforderungen                    |
| ------------------------- | ----------------- | -------------------------------- |
| Innen (trocken)           | IP54              | Staubschutz, Spritzwasserschutz  |
| Außen (unter Dach)        | IP65              | Wasserstrahlschutz, staubdicht   |
| Außen (direkte Witterung) | IP66+             | Starker Wasserstrahl, wetterfest |

### Gehäuse-Tipps

- **Kabelverschraubungen** (PG9 oder PG11) für alle Kabeleinführungen verwenden
- **Tropfschlaufe** an externen Kabeln vor dem Gehäuseeintritt vorsehen
- Netz- und Niederspannungskabel auf **gegenüberliegenden Seiten** des
  Gehäuses führen
- **Zugentlastung** an allen Kabeln anbringen
- Gehäuse sollte **verschließbar** sein

---

## Schutzleiter (Erdung)

### Schutzleiteranschluss

- Metallgehäuse an Schutzleiter anschließen
- Pumpenmotor-Schutzleiter anschließen
- Grün-gelben Draht (min. 2,5 mm²) für Erdverbindungen verwenden
- Durchgang vor Inbetriebnahme mit Multimeter prüfen

### Masseschleifen vermeiden

- Alle Erdleiter zu **einem zentralen Erdungspunkt** führen (Stern-Topologie)
- Erdverbindungen nicht in Reihe schalten
- Isolierte Ringkabelschuhe für Erdanschlüsse verwenden

---

## Inbetriebnahme

### Vor dem Anschließen der Netzspannung

1. [Von Null aufgebaut](/docs/build-from-zero/) Steckbrett-Test abschließen
2. Sensoren geben plausible Werte aus
3. Relais-Funktion mit Multimeter geprüft (ohne Last)
4. Gehäuse ordnungsgemäß abgedichtet

### Erster Netz-Einschalt

1. **Controller in den ersten 30 Minuten nicht unbeaufsichtigt lassen**
2. Auf ungewöhnliche Geräusche achten (Brummen, Überschläge)
3. Auf Erwärmung an Relais-Klemmen und Netzteil prüfen
4. Prüfen, ob die Pumpe korrekt startet und stoppt
5. Prüfen, ob das Relais beim Ausschalten zuverlässig trennt

### Jährliche Prüfung

- [ ] Alle Schraubklemmen auf festen Sitz prüfen
- [ ] Kabel auf Beschädigung oder Abnutzung prüfen
- [ ] RCD/FI-Schalter testen
- [ ] Gehäuse-Lüftungsöffnungen reinigen
- [ ] Erdungsdurchgang prüfen

---

## Notfallmaßnahmen

### Bei Brandgeruch

1. **Sofort Strom abschalten** am Sicherungsautomaten
2. Controller nicht berühren
3. CO₂- oder Pulverlöscher verwenden (KEIN Wasser)
4. Installation vor Wiederinbetnahme überprüfen lassen

### Bei Stromschlag

1. **Person nicht berühren**, solange sie unter Spannung steht
2. Strom am Sicherungsautomaten abschalten
3. Sofort Rettungsdienst verständigen (112)
4. Erste Hilfe leisten (CPR, wenn ausgebildet), sobald stromlos

---

## Verwandte Dokumente

- [Von Null aufgebaut](/docs/build-from-zero/) — Komplette Bauanleitung
- [Hardware-Anleitung](/docs/hardware-guide/) — Hardware-Details
- [Sicherheitsmodell](/docs/safety-model/) — System-Sicherheitsarchitektur
- [Produktions-Checkliste](/docs/production-checklist/) — Prüfungen vor Inbetriebnahme
- [Sicherheits-Checkliste](/docs/security-checklist/) — Sicherheitshärtung
