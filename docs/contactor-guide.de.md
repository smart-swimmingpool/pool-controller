---
title: Schütz-Schaltung für Pumpen
summary: Warum die NORVI-Bordrelais bei Motorlast versagen und wie du mit externen Schützen die Pumpen zuverlässig schaltest — Teileliste, Schaltplan, Verdrahtung
date: "2026-07-13"
draft: false
toc: true
type: docs
menu:
  docs:
    parent: Pool Controller
    name: Schütz-Schaltung
    weight: 65
---

## Übersicht

Dieses Dokument erklärt, warum die eingebauten Relais des NORVI AE01-R (und
anderer handelsüblicher Relaismodule) bei Motorlast (Pumpen) versagen können,
und wie du mit **externen Installationsschützen** (Contacteurs/Contactor) eine
dauerhaft zuverlässige Schaltung aufbaust.

**Gilt für:** NORVI AE01-R (R0–R5) und externe 5V-Relaismodule (Standard-ESP32).

---

## Problem: Relaiskontakte verschweißen bei Motorlast

### Die NORVI-Relais

Der NORVI AE01-R hat **6 eingebaute elektromechanische Relais** (SPST-NO,
5A/250V AC). Diese sind für **ohmsche Last** spezifiziert — also Glühlampen,
Heizwiderstände oder elektronische Geräte mit geringem Einschaltstrom.

Eine **Pumpe ist eine induktive Last** (Motor mit Spulen):

| Eigenschaft | Ohmsche Last | Motorlast (induktiv) |
|-------------|:------------:|:--------------------:|
| Dauerstrom | Wie angegeben | Wie angegeben |
| Einschaltstrom (Inrush) | 1–1,5× Nennstrom | **5–10× Nennstrom** |
| Lichtbogen beim Schalten | Gering | **Stark** (durch Back-EMF) |
| Kontaktverschleiß | Minimal | **Hoch** |

**Praktische Folge:** Ein NORVI-Relais mit "5A" spezifikation kann an einer
300–600W-Pumpe (1,3–2,6A Dauerstrom, aber 6,5–13A Einschaltspitze) nach
einigen hundert Schaltvorgängen **verschweißte Kontakte** bekommen. Das Relais
klickt dann zwar noch hörbar (die Spule funktioniert), aber der NO-Kontakt
trennt nicht mehr — die Pumpe läuft **dauerhaft**, auch wenn der Controller
"AUS" anzeigt.

### Feldbericht

In einer realen Installation trat dieser Fehler nacheinander an zwei
verschiedenen Relaisausgängen (zuerst R1, dann R5) desselben NORVI-Moduls auf,
nachdem diese jeweils die Solarpumpe (300–600W) schalteten. Der
gemeinsame Faktor war **nicht** das Relais oder der Treiber, sondern die
**Pumpe selbst** als Last. Das baugleiche R0 (Poolpumpe) zeigte keine
Ausfälle — likely weil die Poolpumpe anders aufgebaut ist
(Kondensatormotor mit Sanftanlauf, seltener geschaltet, oder geringere
Leistung).

---

## Lösung: Externes Installationsschütz

Ein **Schütz (Contacteur/Contactor)** ist ein für Motorlast ausgelegtes,
robustes Relais in Hutschienenbauweise. Das NORVI-Relais schaltet nur noch
die **Steuerspule des Schützes** (einige mA bei 24V DC) — die Hauptkontakte
des Schützes schalten die 230V-Last.

```
 NORVI-Relais           Schütz (extern)            Pumpe
 ┌──────────┐          ┌──────────────────┐      ┌────────┐
 │          │  24V DC  │  A1 (+)          │      │        │
 │ COM ─────┼──────────┤  (Spule)         │      │        │
 │          │          │         A2 (-) ──┼── GND│        │
 │ NO  ─────┼──────┐   │                  │      │        │
 └──────────┘      │   │ 1 (L-Eingang) ───┤─── 2 │        │
                   │   └──────────────────┘      │  L-N  │
 230V L ───────────┘                             └────────┘
 230V N ───────────────────────────────────────────┘
```

### Vorteile

| Vorteil | Beschreibung |
|---------|-------------|
| **Kontaktlebensdauer** | Schütze sind für 1–5 Millionen Schaltungen unter Motorlast ausgelegt |
| **Tauschbar** | Defektes Schütz wird in 2 Minuten auf der Hutschiene getauscht — ohne Löten |
| **NORVI bleibt unbelastet** | Die NORVI-Relais schalten nur mA → kein Kontaktverschleiß mehr |
| **Kostengünstig** | Pro Schütz ~10–25€ |

---

## Empfohlene Schütze

### Finder 22 Serie (empfohlen)

Kompakte Hutschienen-Schütze mit integrierter Überspannungsunterdrückung.

| Typ | Spule | Kontakt | Motorlast | Breite | Preis |
|-----|:-----:|:-------:|:---------:|:-----:|:-----:|
| **22.21.0.024.4000** | 24V DC | 1× NO, 20A | 1,5 kW | 17,6mm | ~20–25€ |
| **22.32.0.024.4000** | 24V DC | 2× NO, 16A | 1,5 kW | 17,6mm | ~25–30€ |

**Vorteil Finder 22:** Eingebaute Suppressor-Diode (Varistor) — kein externer
Freilauf notwendig, aber zur Sicherheit trotzdem eine 1N4007-Diode parallel
zur Spule empfehlenswert.

### Eaton / Moeller DILM

Robuste Schütze, oft günstiger.

| Typ | Spule | Kontakt | Motorlast | Breite | Preis |
|-----|:-----:|:-------:|:---------:|:-----:|:-----:|
| **DILM7-01 (24V DC)** | 24V DC | 3× NO, 7A | 3 kW | 45mm | ~12–15€ |

### Interface-Relais (kompakt)

Wenn du maximale Platzersparnis brauchst:

| Typ | Spule | Kontakt | Breite | Preis |
|-----|:-----:|:-------:|:-----:|:-----:|
| **Finder 40.52.9.024.0040** | 24V DC | 1× Wechsler 16A | 6,2mm | ~10€ |
| + **Sockel 94.02** | — | — | — | ~3€ |

> **Hinweis:** Interface-Relais sind kleiner als Installationsschütze, aber
> immer noch deutlich robuster als die NORVI-Bordrelais. Für Pumpen bis 600W
> völlig ausreichend.

---

## Schaltplan

### 24V-Steuerkreis

```
24V DC (+) ──┬── NORVI R0 COM ── NO ──┬── Schütz Pool A1 ── A2 ──┬── GND
             │                        │                          │
             ├── NORVI R5 COM ── NO ──┤── Schütz Solar A1 ── A2 ──┤
             │                        │                          │
             └───── NORVI 24V IN ─────┘                          │
                                                         │
GND ─────────────────────────────────────────────────────┘
```

**Freilaufdioden 1N4007** parallel zu jeder Schützspule:
- Kathode (Strichseite) an **A1 (+)** 
- Anode an **A2 (GND)**

Die Dioden löschen den Spannungsrückschlag (Back-EMF) der Schützspule beim
Abschalten. Ohne Diode kann der Spannungsstoß das NORVI-Relais beschädigen.

### 230V-Leistungskreis

```
230V L (Phase) ─── RCD ─── MCB ──┬── Schütz Solar 1-2 ── Solarpumpe L
                                  │
                                  └── Schütz Pool 1-2 ─── Poolpumpe L

230V N (Neutral) ────────────────────────── Beide Pumpen N
```

Die 230V-Seite gehört **immer** hinter FI (RCD, 30mA) und
Leitungsschutzschalter (MCB, B10A).

---

## Materialliste (beide Pumpen)

| Stk | Artikel | ca. Preis |
|:---:|---------|:---------:|
| 2 | Finder 22.21.0.024.4000 (20A, 24V DC, 1× NO) | ~45€ |
| 2 | 1N4007 Diode (Freilauf, parallel zur Schützspule) | ~0,50€ |
| 5m | 3×1,5mm² Leitung (NYM-J oder H07RN-F) | ~8€ |
| 1 | Aderendhülsen-Set 0,75mm² + 1,5mm² | ~5€ |
| 2 | Wago 221-412 (Dienstklemmstellen) | ~4€ |
| — | Diverse Kabelbinder, Schrumpfschlauch, Beschriftung | ~5€ |
| **Gesamt** | | **~65–70€** |

**Kurzversion:** Such einfach "Finder 22.21.0.024.4000" und "1N4007"
auf [Reichelt.de](https://reichelt.de), [ELMEG](https://elmeg.de) oder
Amazon.

---

## Einbau-Schritte

### 1. Vorbereitung

1. **Spannungsfrei schalten** — 230V-Sicherung raus, 24V-Netzteil trennen
2. **Hutschiene prüfen** — die Schütze sind 17,6mm breit (Finder 22) oder
   45mm (Eaton). Neben dem NORVI-Platz einplanen.
3. **Kabel vorbereiten** — 1,5mm² für 230V, 0,75mm² für 24V. Aderendhülsen
   aufcrimpen.

### 2. Verdrahtung

**Schritt 1: 24V-Steuerkreis**

1. **24V DC (+) an NORVI-Relais COM** — Brücken von NORVI-24V-Klemme zu
   COM-R0 und COM-R5
2. **NORVI-Relais NO an Schütz-A1** — jeweils R0-NO → Schütz Pool A1,
   R5-NO → Schütz Solar A1
3. **Schütz-A2 an GND** — beide A2 auf GND-Sammelschiene
4. **Freilaufdiode** — 1N4007 parallel zu A1/A2 jedes Schützes
   (Kathode an A1, Anode an A2)

**Schritt 2: 230V-Leistungskreis**

1. **230V L von FI/LS** — auf Schütz-Klemme 1 (beide Schütze parallel)
2. **Schütz-Klemme 2** — zur entsprechenden Pumpe L
3. **230V N** — direkt zur Pumpe N (nicht über Schütz)
4. **Schutzleiter (PE)** — falls Pumpengehäuse, separat anschließen

**Schritt 3: Prüfen**

1. **Durchgangsprüfung** — 24V-Seite: zwischen A1 und A2 ca. 100–400Ω
   (Spulenwiderstand). Diodentest: Diode in Sperrrichtung messen
2. **Kurzschlussprüfung** — 230V-Seite: zwischen L und N > 1MΩ
3. **Widerstand zwischen 24V und 230V** — > 10MΩ (galvanische Trennung)

### 3. Funktionstest

1. 24V einschalten → Controller bootet
2. Controller zeigt Pumpen AUS → **Schütze müssen offen sein** (kein
   Durchgang zwischen Klemme 1 und 2)
3. Pumpen EIN schalten (Web UI, MQTT oder OLED) → **Schütz muss hörbar
   schalten** (deutliches Klacken)
4. Pumpe läuft an → **alles OK**

---

## Fehlersuche

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Schütz klackt nicht | NORVI-Relais schaltet nicht | GPIO-Pin im Controller prüfen, Relais-ID prüfen |
| Schütz klackt, Pumpe läuft nicht | Schütz 1-2 hat keinen Durchgang | 230V-Versorgung prüfen, Klemmen 1-2 durchmessen |
| Pumpe läuft trotz AUS | Schütz-Kontakt geschweißt **oder** NORVI-Relais hält | Schütz tauschen; NORVI-Diagnose: GPIO-Pegel messen |
| Schütz brummt | Spulenspannung zu niedrig | 24V-Versorgung prüfen (min. 21,6V DC) |
| Controller resetet beim Schalten | Keine Freilaufdiode | 1N4007 nachrüsten |

---

## Referenzen

- [SVG-Schaltplan: Schütz-Schaltung](wiring-contactor.svg)
- [NORVI AE01-R Konfiguration](norvi-ae01-r.de.md)
- [Hardware-Anleitung](hardware-guide.de.md)
- [Finder 22 Datenblatt](https://www.findernet.com/de/series/22-series-industrial-relays/)
- [Eaton DILM Datenblatt](https://www.eaton.com)
