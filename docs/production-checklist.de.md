---
title: Produktions-Checkliste
summary: Checkliste für die Inbetriebnahme und jährliche Wartung des Pool-Controllers — Hardware-Prüfung, Firmware-Checks, Sicherheitsinspektionen und Abnahme
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
tags: ["docs", "checkliste", "inbetriebnahme", "produktion"]
menu:
  docs:
    parent: Pool Controller
    name: Produktions-Checkliste
    weight: 150
---

> ⚠️ **WARNUNG: Diese Checkliste betrifft die Prüfung von 230V AC-Netzspannung.**
> Fehlerhafte Verdrahtung kann zu Stromschlag, Brand oder Geräteschäden führen.
>
> - Nur fortfahren, wenn du Erfahrung mit Netzspannungsinstallationen hast
> - **Vor Arbeiten an der Schaltung immer stromlos schalten**
> - Dies ist ein DIY-Projekt — nicht CE/UL-zertifiziert für den kommerziellen Einsatz

## Überblick

Diese Checkliste umfasst alle Prüfungen, die vor der Inbetriebnahme des
Pool-Controllers durchgeführt werden sollten. Verwende sie bei der ersten
Inbetriebnahme und als jährliche Wartungsreferenz.

---

## Vorbereitungs-Checkliste

### Hardware

- [ ] ESP32 sicher im Gehäuse montiert
- [ ] Relaismodul mit ausreichender Belüftung montiert
- [ ] Netzteil gesichert und für die Last ausgelegt
- [ ] Alle Kabelverschraubungen angezogen und abgedichtet
- [ ] Zugentlastung an allen externen Kabeln angebracht
- [ ] Netz- und Niederspannungsleitungen getrennt geführt
- [ ] Keine losen Drähte oder abstehenden Litzen
- [ ] Alle Schraubklemmen festgezogen
- [ ] Sicherung installiert (korrekte Nennstromstärke für Pumpenlast)
- [ ] Gehäuse-IP-Schutzart entspricht den Standortanforderungen

### Verdrahtung

- [ ] DS18B20-Sensoren: VCC → 3,3V, GND → GND, DATA → GPIO (4,7 kΩ Pull-up)
- [ ] Relaismodul: VCC → 5V, GND → GND, IN1 → GPIO18, IN2 → GPIO19
- [ ] Relais COM → Netz-Außenleiter
- [ ] Relais NO → Pumpen-Außenleiter
- [ ] Neutralleiter → Pumpen-Neutralleiter (direkt, nicht über Relais)
- [ ] Schutzleiter an Pumpe und Gehäuse (falls Metall) angeschlossen
- [ ] Erdungsdurchgang mit Multimeter geprüft

### Sensoren

- [ ] Pool-Temperatur zeigt plausiblen Wert (nicht -127 °C)
- [ ] Solar-Temperatur zeigt plausiblen Wert (nicht -127 °C)
- [ ] Temperaturwerte stabil (keine starken Schwankungen)
- [ ] Sensorkabel unbeschädigt und korrekt verlegt

### Firmware

- [ ] Aktuelle Firmware-Version geflasht
- [ ] WLAN konfiguriert und Verbindung stabil
- [ ] mDNS-Name wird aufgelöst (`pool-controller.local`)
- [ ] Web-UI unter Geräte-IP erreichbar
- [ ] Web-UI-Passwort geändert (nicht mehr Standard)
- [ ] MQTT-Broker konfiguriert und verbunden
- [ ] Home Assistant Discovery aktiv (Controller in HA sichtbar)
- [ ] Alle HA-Entitäten funktionsfähig

### Relais-Test (ohne Last)

- [ ] Poolpumpen-Relais schaltet beim Umschalten
- [ ] Solarpumpen-Relais schaltet beim Umschalten
- [ ] Relais-Verhalten korrekt (nicht invertiert)
- [ ] Multimeter bestätigt COM–NO-Durchgang bei ON
- [ ] Multimeter bestätigt COM–NO-Unterbrechung bei OFF

### Relais-Test (mit Last)

- [ ] Poolpumpe startet und läuft korrekt
- [ ] Poolpumpe stoppt beim Ausschalten
- [ ] Solarpumpe startet und läuft korrekt
- [ ] Solarpumpe stoppt beim Ausschalten
- [ ] Keine ungewöhnlichen Geräusche aus den Relais (Brummen, Überschläge)
- [ ] Keine übermäßige Erwärmung an Relais-Klemmen nach 5 Min.

### Konfiguration

- [ ] Zeitzone korrekt eingestellt
- [ ] Timer-Einstellungen entsprechen dem gewünschten Zeitplan
- [ ] Temperatur-Schwellwerte konfiguriert
- [ ] Betriebsarten getestet (Auto, Manual, Boost, Timer)
- [ ] `relay-invert` passend zum Relaismodul-Typ (active-low vs active-high)

---

## Erste 24 Stunden Überwachung

- [ ] Controller bleibt online (keine unerwarteten Neustarts)
- [ ] Temperaturen werden konsistent aktualisiert
- [ ] MQTT-Verbindung bleibt aktiv
- [ ] Heap bleibt über 20 KB
- [ ] WLAN-Signal stabil (RSSI > -70 dBm empfohlen)
- [ ] Keine Safe-Mode-Aktivierung
- [ ] Relais arbeiten nach Zeitplan (Auto-Modus)

---

## Jährliche Wartungs-Checkliste

- [ ] Alle Schraubklemmen auf festen Sitz prüfen
- [ ] Kabel auf Beschädigung, Abnutzung oder Korrosion prüfen
- [ ] Gehäusedichtung und Kabelverschraubungen prüfen
- [ ] RCD/FI-Schutzschalter testen
- [ ] Gehäuse-Belüftungsöffnungen reinigen
- [ ] Erdungsdurchgang prüfen
- [ ] Firmware aktualisieren, falls neuere Version verfügbar
- [ ] Konfiguration auf notwendige Änderungen prüfen
- [ ] Batterie des RCD-Prüfgeräts prüfen (falls vorhanden)
- [ ] Alle Sensorwerte auf Genauigkeit prüfen

---

## Abnahme

```text
Geräte-ID / MAC: ______________________________
Installationsdatum: ___________________________
Installiert von: ______________________________
Firmware-Version: _____________________________
Letzte Wartung: _______________________________
Nächste Wartung fällig: _______________________
```

---

## Verwandte Dokumente

- [Von Null aufgebaut](/docs/build-from-zero/) — Komplette Bauanleitung
- [Elektrische Sicherheit](/docs/electrical-safety/) — Sicherheitsinformationen
- [Sicherheitsmodell](/docs/safety-model/) — System-Sicherheitsarchitektur
- [Sicherheits-Checkliste](/docs/security-checklist/) — Sicherheitshärtung
- [Fehlerbehebung](/docs/troubleshooting/) — Häufige Probleme und Lösungen
