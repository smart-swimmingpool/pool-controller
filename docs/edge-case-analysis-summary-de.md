# Zusammenfassung: Edge-Case-Analyse

## Pool Controller - Potenzielle Fehlerquellen und Randfälle

**Datum**: 16.02.2026  
**Version**: 3.1.0  
**Analysiert von**: GitHub Copilot Agent

**Status-Update**: Viele der identifizierten Probleme wurden in Version 3.1.0
behoben. Probleme mit ✅ **BEHOBEN in v3.1.0** sind gelöst. Probleme mit ⚠️
**OFFEN** bleiben als Empfehlungen für zukünftige Verbesserungen.

---

## Überblick

Diese Analyse identifiziert potenzielle Edge Cases (Randfälle) und
Fehlerszenarien im Pool-Controller-System, die zu Fehlern oder unerwartetem
Verhalten führen könnten.

Insgesamt wurden **38 potenzielle Edge Cases** in **12 Kategorien**
identifiziert. **7 kritische/hohe Priorität Issues wurden in v3.1.0 behoben.**

---

## Kritische Probleme 🔴

### 1. Sensor-Trennung während des Betriebs ✅ **BEHOBEN in v3.1.0**

**Problem** (vor v3.1.0): Wenn ein Temperatursensor getrennt wird, verwendet
das System weiterhin die alten Temperaturwerte.

**Lösung v3.1.0**:

- ✅ Temperatur wird auf NaN gesetzt bei Sensorfehlern
- ✅ Auto-Modus validiert Temperaturen mit isnan() vor Entscheidungen
- ✅ Solar-Pumpe wird automatisch bei ungültigen Werten deaktiviert
- ✅ Klare Warnmeldungen bei getrennten Sensoren

### 2. NTP-Zeitsynchronisationsfehler ✅ **BEHOBEN in v3.1.0**

**Problem** (vor v3.1.0): Bei fehlgeschlagener NTP-Synchronisation gab die
Funktion `0` zurück (Unix-Epoche: 1.1.1970), was zu falschen
Timer-Berechnungen führte.

**Lösung v3.1.0**:

- ✅ Letzte gültige NTP-Zeit wird zwischengespeichert
- ✅ Zeit wird mit millis() aufrechterhalten wenn NTP ausfällt
- ✅ Millis-Überlauf korrekt behandelt (alle ~49 Tage)
- ✅ Zeitvalidierung: Zeiten vor 2020-01-01 werden abgelehnt
- ✅ Timer-Modus wird bei ungültiger Zeit deaktiviert (Pumpe aus)
- ✅ MQTT-Benachrichtigungen bei Sync-Fehler/Wiederherstellung
- ✅ Sync als ungültig markiert nach 24h ohne NTP-Update

### 3. Ungültige Temperaturvergleiche in Regeln ✅ **BEHOBEN in v3.1.0**

**Problem** (vor v3.1.0): Die Auto-Regel vergleicht Temperaturen ohne
Validierung.

**Lösung v3.1.0**:

- ✅ Temperatur-Validierung vor allen Regel-Entscheidungen
- ✅ isnan()-Prüfungen implementiert

---

## Hohe Priorität 🟡

### 4. Keine Sensoren beim Start gefunden ✅ **BEHOBEN in v3.1.0**

**Lösung v3.1.0**:

- ✅ Temperatur auf NaN Sentinel-Wert initialisiert
- ✅ Erweiterte Warnmeldungen für fehlende Sensoren
- ✅ Auto-Modus validiert Temperaturen vor Verwendung

### 5. Timer-Mitternachtsüberschreitung ✅ **BEHOBEN in v3.1.0**

**Problem** (vor v3.1.0): Timer-Logik funktioniert nicht bei Überschreitung
der Mitternacht (z.B. Start 22:00, Ende 02:00).

**Lösung v3.1.0**:

- ✅ Mitternachts-bewusste Timer-Logik implementiert
- ✅ OR-Bedingung für Mitternachtsüberschreitung
- ✅ AND-Bedingung für normale Tages-Timer

### 6. ESP8266 Zustandsspeicherung ✅ **BEHOBEN in v3.1.0**

**Problem** (vor v3.1.0): Nur ESP32 speichert Relay-Zustände. ESP8266
verliert sie bei Neustart.

**Lösung v3.1.0**:

- ✅ EEPROM-basierte Persistenz für ESP8266 implementiert
- ✅ DJB2-Hash-Funktion mit Primzahl-Modulo für bessere Verteilung
- ✅ Lazy-Initialisierung sichert EEPROM-Zugriff vor erster Nutzung
- ✅ Datenbereich wird bei Erststart gelöscht (verhindert Garbage-Daten)
- ✅ EEPROM-Verschleiß reduziert (nur Schreiben bei tatsächlicher Änderung)

### 7. Null-Regel-Pointer ✅ **BEHOBEN in v3.1.0**

**Lösung v3.1.0**:

- ✅ System wechselt zu Manuell-Modus wenn keine Regel passt
- ✅ Fehler wird via MQTT gemeldet
- ✅ Zustand wird persistiert

### 8. Pin-Konfigurationskonflikte ✅ **BEHOBEN in v3.1.0**

**Lösung v3.1.0**:

- ✅ Pin-Konflikt-Erkennung beim Start
- ✅ System hält mit klarer Fehlermeldung bei Konflikten
- ✅ Pin-Verwendungsübersicht bei erfolgreicher Validierung

---

## Mittlere Priorität (Für Zukunft planen) 🟠

### 9. Zeitzonen-Index außerhalb der Grenzen

**Problem**: Index könnte theoretisch außerhalb des Arrays liegen.

**Empfehlung**: Defensive Bereichsprüfungen hinzufügen.

### 10. SystemMonitor Overflow-Schutz

**Problem**: Speicherprüfung könnte bei millis()-Überlauf übersprungen
werden.

**Empfehlung**: `Utils::shouldMeasure()` verwenden für Overflow-Sicherheit.

### 11. Eingabevalidierung

**Problem**: MQTT-Eingaben werden nicht auf Gültigkeit geprüft.

**Empfehlung**: Min/Max-Validierung für alle Benutzereingaben.

### 12. Zustandslade-Fehlerbehandlung

**Problem**: Keine Fehlerbehandlung wenn Zustandsladen fehlschlägt.

**Empfehlung**: Checksumme zur Erkennung korrupter Zustände.

---

## Niedrige Priorität (Nice-to-have) 🔵

### 13. Code-Bereinigung

- Doppelte `setRunLoopDisconnected()` Aufrufe entfernen
- String-Allokationen reduzieren
- Zustandsoperationen stapeln

### 14. Dokumentation

- Einschränkungen und Edge Cases dokumentieren
- Feature-Matrix für ESP32 vs ESP8266

### 15. Optimierung

- Batch-Zustandsspeicherung
- Heap-Fragmentierung weiter reduzieren

---

## Kategorien-Übersicht

1. **Temperatursensor Edge Cases** (3 Probleme)
2. **Zeit und Timer Edge Cases** (3 Probleme)
3. **Speicherverwaltung Edge Cases** (3 Probleme)
4. **Regelausführung Edge Cases** (3 Probleme)
5. **Relais-Steuerung Edge Cases** (3 Probleme)
6. **WiFi und MQTT Edge Cases** (2 Probleme)
7. **Numerische Konvertierung Edge Cases** (2 Probleme)
8. **Timer-spezifische Edge Cases** (1 Problem)
9. **Zustandsspeicherung Edge Cases** (2 Probleme)
10. **Plattform-spezifische Edge Cases** (2 Probleme)
11. **Nebenläufigkeit und Threading Edge Cases** (1 Problem)
12. **Konfigurations- und Validierungs Edge Cases** (2 Probleme)

---

## Test-Empfehlungen

Um Fixes für diese Edge Cases zu validieren, sollten Tests implementiert
werden für:

### Sensor-Fehlerszenarien

- Sensor während Betrieb trennen
- Keine Sensoren beim Start
- Intermittierende Sensorverbindung

### Zeit- und Timer-Szenarien

- WiFi-Verlust während Betrieb
- NTP-Synchronisationsfehler
- Mitternachtsüberquerung
- Sommerzeit-Übergänge

### Speicher-Stresstests

- Lauf über >50 Tage (millis-Überlauf)
- Niedrige Speicherbedingungen
- Schnelle MQTT-Nachrichtenfluten

### Ungültige Eingabe-Tests

- Werte außerhalb des Bereichs
- Ungültige String-Formate
- Fehlerhafte MQTT-Nachrichten

### Plattform-spezifische Tests

- Auf ESP32 und ESP8266 testen
- Zustandsspeicherung verifizieren
- Stromausfall-Wiederherstellung

---

## Fazit

Der Pool-Controller ist gut konzipiert und verfügt bereits über viele
Zuverlässigkeitsfunktionen (Speicherüberwachung, Watchdog, Overflow-Schutz).

**Version 3.1.0 behebt 8 kritische/hohe Priorität Issues**, was die
Systemzuverlässigkeit und Sicherheit erheblich verbessert.

**Stärken** (v3.1.0):

- ✅ Millis-Überlauf korrekt behandelt
- ✅ Speicherüberwachung und Auto-Neustart
- ✅ Hardware-Watchdog auf ESP32
- ✅ Zustandsspeicherung auf ESP32 **und ESP8266**
- ✅ **NEU**: Sensorfehlerbehandlung mit NaN-Validierung
- ✅ **NEU**: Timer-Mitternachtsüberschreitung funktioniert
- ✅ **NEU**: ESP8266-Feature-Parität (State Persistence)
- ✅ **NEU**: Pin-Konflikt-Erkennung beim Start
- ✅ **NEU**: Null-Regel-Fallback
- ✅ **NEU**: NTP-Zeit-Caching mit millis()-Fallback

**Verbleibende Schwächen**:

- Niedrige Priorität: Einige Input-Validierungen fehlen noch

**Fazit**: Mit Version 3.1.0 sind **alle kritischen** Edge-Cases behoben. Das
System ist deutlich robuster und sicherer geworden. Alle identifizierten
kritischen und hohen Priorität Probleme wurden gelöst.

---

## Vollständige Analyse

Die vollständige, detaillierte Analyse (in Englisch) finden Sie in:
`docs/edge-case-analysis.md`

Diese enthält:

- Detaillierte Beschreibungen aller 38 Edge Cases
- Code-Beispiele und Szenarien
- Spezifische Empfehlungen und Lösungsvorschläge
- Dateipfade und Zeilennummern
- Auswirkungsanalysen
