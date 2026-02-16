# Zusammenfassung: Edge-Case-Analyse

## Pool Controller - Potenzielle Fehlerquellen und Randfälle

**Datum**: 16.02.2026  
**Version**: 3.1.0  
**Analysiert von**: GitHub Copilot Agent

---

## Überblick

Diese Analyse identifiziert potenzielle Edge Cases (Randfälle) und
Fehlerszenarien im Pool-Controller-System, die zu Fehlern oder unerwartetem
Verhalten führen könnten.

Insgesamt wurden **38 potenzielle Edge Cases** in **12 Kategorien**
identifiziert.

---

## Kritische Probleme (Sofort beheben) 🔴

### 1. Sensor-Trennung während des Betriebs

**Problem**: Wenn ein Temperatursensor getrennt wird, verwendet das System
weiterhin die alten Temperaturwerte. Die Automatik könnte falsche
Heizentscheidungen treffen.

**Auswirkung**: Pool könnte überhitzen oder nicht richtig heizen.

**Empfehlung**:

- Sichere Standardwerte bei Sensorfehlern setzen
- Automatisches Abschalten der Pumpen bei anhaltenden Sensorfehlern
- Timeout-Mechanismus implementieren

### 2. NTP-Zeitsynchronisationsfehler

**Problem**: Bei fehlgeschlagener NTP-Synchronisation gibt die Funktion `0`
zurück (Unix-Epoche: 1.1.1970), was zu falschen Timer-Berechnungen führt.

**Auswirkung**: Pool-Pumpe läuft zur falschen Zeit oder gar nicht.

**Empfehlung**:

- Letzte gültige Zeit zwischenspeichern
- RTC oder millis() zur Zeitbeibehaltung nutzen
- Zeitsynchronisationsfehler erkennen und Timer-Modus deaktivieren
- Benutzer via MQTT alarmieren

### 3. Ungültige Temperaturvergleiche in Regeln

**Problem**: Die Auto-Regel vergleicht Temperaturen ohne Validierung. Ein
getrennter Sensor (-127°C) wird mit gültigen Werten verglichen.

**Auswirkung**: Falsche Pumpenschaltungen basierend auf ungültigen Daten.

**Empfehlung**:

- Temperaturwert-Validierung vor Regelausführung
- Auto-Modus bei ungültigen Sensoren deaktivieren
- In sicheren Manuell-Modus wechseln

---

## Hohe Priorität (Bald beheben) 🟡

### 4. Keine Sensoren beim Start gefunden

**Problem**: System läuft mit ungültigen Temperaturdaten weiter, wenn keine
Sensoren erkannt werden.

**Empfehlung**:

- Temperatur auf Sentinel-Wert initialisieren
- Auto-Modus deaktivieren wenn kritische Sensoren fehlen
- Exponentielles Backoff für Sensor-Wiedererkennung

### 5. Timer-Mitternachtsüberschreitung

**Problem**: Timer-Logik funktioniert nicht bei Überschreitung der
Mitternacht (z.B. Start 22:00, Ende 02:00).

**Auswirkung**: Nacht-Timer funktionieren nicht - Pumpe läuft nie.

**Empfehlung**: Mitternachts-bewusste Timer-Logik implementieren.

### 6. ESP8266 Zustandsspeicherung nicht implementiert

**Problem**: Nur ESP32 speichert Relay-Zustände. ESP8266 verliert sie bei
Neustart.

**Auswirkung**: Pumpenzustand nach Stromausfall undefiniert.

**Empfehlung**:

- EEPROM-basierte Persistenz für ESP8266 implementieren
- Oder: Dokumentieren dass ESP8266 keine Zustandsspeicherung unterstützt

### 7. Null-Regel-Pointer

**Problem**: Wenn keine Regel zum aktuellen Modus passt, wird nichts
ausgeführt.

**Empfehlung**: Auf sicheren Modus zurückfallen, Fehler via MQTT melden.

### 8. Pin-Konfigurationskonflikte

**Problem**: Keine Validierung, ob Pin-Nummern zwischen Nodes kollidieren.

**Empfehlung**: Pin-Konflikt-Erkennung beim Start, Validierung der
Pin-Konfiguration.

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
Dennoch bleiben mehrere Edge Cases, die in Produktionsumgebungen Probleme
verursachen könnten.

**Stärken**:

- ✅ Millis-Überlauf korrekt behandelt
- ✅ Speicherüberwachung und Auto-Neustart
- ✅ Hardware-Watchdog auf ESP32
- ✅ Zustandsspeicherung auf ESP32

**Schwächen**:

- ❌ Unzureichende Sensorfehlerbehandlung
- ❌ Zeitsynchronisationsfehler nicht behandelt
- ❌ Timer-Mitternachtsüberschreitung fehlerhaft
- ❌ ESP8266-Feature-Parität unvollständig
- ❌ Eingabevalidierung fehlt

**Empfehlung**: Die Behebung der **kritischen** und **hohen Priorität**
Punkte wird die Systemzuverlässigkeit und Benutzererfahrung erheblich
verbessern.

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
