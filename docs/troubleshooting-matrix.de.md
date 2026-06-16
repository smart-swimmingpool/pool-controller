---
title: Fehlerbehebungs-Matrix
summary: Kurzreferenz-Fehlerbehebungsmatrix für den Pool-Controller — häufige Symptome, Ursachen und Lösungen im kompakten Tabellenformat
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
tags: ["docs", "fehlerbehebung", "kurzreferenz", "matrix"]
menu:
  docs:
    parent: Pool Controller
    name: Fehlerbehebungs-Matrix
    weight: 121
---

> ⚠️ **WARNUNG**: Einige Schritte zur Fehlerbehebung umfassen Arbeiten an
> **230V AC-Netzspannung** (Relais-Test, Pumpen-Verdrahtung). **Vor Arbeiten
> an der Schaltung immer stromlos schalten.** Im Zweifel eine qualifizierte
> Elektrofachkraft beauftragen.

## Fehlerbehebungs-Matrix

| #   | Symptom                                             | Ursache                                                                       | Lösung                                                                                                                 |
| --- | --------------------------------------------------- | ----------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| 1   | **-127 °C** Temperaturwert                          | DS18B20 nicht am OneWire-Bus erkannt                                          | 4,7 kΩ Pull-up zwischen DATA und 3,3V prüfen; Verkabelung kontrollieren (VCC, GND, DATA); Lötstellen prüfen            |
| 2   | **85 °C** Temperaturwert                            | DS18B20 im Parasitärversorgungsmodus ohne ausreichende Spannung               | VCC-Pin an 3,3V anschließen (externe Versorgung, nicht parasitär)                                                      |
| 3   | HA findet kein Gerät                                | MQTT-Discovery-Payload nicht gesendet oder Broker nicht erreichbar            | MQTT-Broker-Einstellungen (Host, Port, Zugangsdaten) im Web-UI prüfen; Broker-Konnektivität mit MQTT Explorer prüfen   |
| 4   | HA-Entitäten zeigen "nicht verfügbar"               | MQTT-Verbindung unterbrochen oder Broker neugestartet                         | Controller-MQTT-Status im Web-UI prüfen; HA-MQTT-Integration neustarten; Controller neustarten                         |
| 5   | Relais-Verhalten invertiert                         | Active-High-Relais mit Firmware, die Active-Low-Logik verwendet (fest codiert)| Die Firmware steuert Relais mit Active-Low-Logik (`LOW` = an). Erwartet Ihr Relaismodul Active-High (d.h. HIGH = an), auf ein Active-Low-Modul wechseln oder externen Transistor-Inverter hinzufügen |
| 6   | Relais klickt, aber Pumpe läuft nicht               | Falsche COM/NO-Verdrahtung oder Pumpensicherung defekt                        | COM → Netz, NO → Pumpe prüfen; Pumpensicherung prüfen; Pumpe unabhängig testen                                         |
| 7   | Relais klickt nicht                                 | GPIO-Pin defekt, Relaismodul nicht versorgt oder Optokoppler defekt           | GPIO-Spannung messen (sollte zwischen ~0 V und ~3,3 V wechseln); Relais VCC (5V) und GND prüfen; **IN-Leitung vom ESP32-GPIO trennen** und Relais-Eingang mit 3,3 V (nicht 5 V) testen |
| 8   | Safe Mode aktiv                                     | Bootloop erkannt: 3 aufeinanderfolgende Boots < 5 Minuten                     | Serielles Log auf Absturzursache prüfen (Panic, Assert, Exception); Ursache beheben; neustarten                        |
| 9   | Zufällige Neustarts alle paar Stunden               | Netzteil zu schwach oder Heap-Erschöpfung                                     | >1A Netzteil verwenden; `free_heap_space` prüfen (sollte >20 KB sein); Serielle Ausgabe auf Abstürze überwachen        |
| 10  | OTA-Update fehlgeschlagen                           | TLS-Handshake-Fehler oder zu wenig Flash-Speicher                             | USB-Flash als Fallback versuchen; Speicher durch Löschen nicht benötigter Daten freigeben; kleinere Firmware versuchen |
| 11  | OTA hochgeladen, aber Gerät startet nicht           | Korrupte oder inkompatible Firmware                                           | Bekannt funktionierende Version über USB flashen; Firmware mit Board-Typ abgleichen                                    |
| 12  | Web-UI lädt nicht                                   | ESP32 nicht im Netzwerk oder falsche IP                                       | Router-DHCP-Liste prüfen; WLAN-Verbindungsstatus prüfen; `pool-controller.local` (mDNS) versuchen                      |
| 13  | Web-UI-Passwort wird nicht akzeptiert               | Passwort geändert, aber Browser hat alte Session gecacht                      | Browser-Cookies und Cache löschen; Inkognito-/Privatfenster verwenden                                                  |
| 14  | MQTT-Verbindung verweigert                          | Falsche Broker-Zugangsdaten oder Broker nicht erreichbar                      | Broker-IP, Port, Benutzername und Passwort in MQTT-Einstellungen prüfen; Prüfen, ob anonyme Verbindungen erlaubt sind  |
| 15  | Temperaturwerte schwanken                           | Lose Verbindung, Interferenz oder langes, ungeschirmtes Kabel                 | Sensor-Verkabelung prüfen; geschirmte, verdrillte Zweidrahtleitung verwenden; Pull-up-Widerstand prüfen (4,7 kΩ)       |
| 16  | Konfiguration nach Neustart weg                     | NVS-Korruption oder Konfiguration nicht gespeichert                           | Konfiguration über Web-UI erneut speichern; bei Beständigkeit Werksreset und Neukonfiguration                          |
| 17  | ESP32 startet nicht im AP-Modus                     | Alte WLAN-Konfiguration noch gespeichert                                      | Werksreset über Web-UI → System → Factory Reset durchführen, um Zugangsdaten zu löschen; AP-Modus startet automatisch, wenn keine SSID konfiguriert ist |
| 18  | WLAN-Scan findet keine Netzwerke                    | ESP32-Antennenproblem, Interferenz oder falsche Region                        | Gerät näher an Router bringen; Antennenanschluss prüfen; WLAN-Regionseinstellungen prüfen                              |
| 19  | mDNS wird nicht aufgelöst (`pool-controller.local`) | Netzwerk unterstützt kein mDNS oder benötigt Reflektor                        | IP-Adresse direkt verwenden; mDNS-Reflektor installieren (avahi-daemon unter Linux)                                    |
| 20  | Serielles Monitor zeigt Zeichensalat                | Falsche Baudrate oder serieller Port                                          | 115200 Baud verwenden; korrekten seriellen Port prüfen (`/dev/ttyUSB0`, `COM3`, etc.)                                  |
| 21  | DS18B20 funktioniert anfangs, fällt dann aus        | Intermittierende Verbindung oder kalte Lötstelle                              | Sensorverbindungen nachlöten; Zugentlastung am Sensorkabel anbringen; auf Feuchtigkeitseintritt prüfen                 |
| 22  | Beide Pumpen laufen gleichzeitig unerwartet         | Timer- oder Moduskonflikt                                                     | Betriebsart prüfen (sollte Auto sein, nicht Manual); Timer-Start-/Endzeiten auf Überlappung prüfen                     |
| 23  | Pumpe läuft bei ausgeschaltetem Controller          | Relais verschweißt/klemmt oder Verdrahtung umgeht Relais                      | **SOFORT STROM ABSCHALTEN**; Relaismodul ersetzen; Verdrahtung prüfen                                                  |
| 24  | Home Assistant zeigt falsche Temperatureinheit      | HA-Temperatureinheiten-Konfiguration falsch                                   | HA → Einstellungen → System → Einheiten prüfen; HA neustarten                                                          |
| 25  | Firmware-Version zeigt "unbekannt"                  | Versionsstring nicht in Firmware eingebunden                                  | Aus sauberem Build neu kompilieren (`pio run --target clean && pio run`); `version.h` auf Korrektheit prüfen           |

---

## Nach Kategorie

### Sensoren

| #   | Symptom             | Wahrscheinliche Ursache | Schnelle Lösung          |
| --- | ------------------- | ----------------------- | ------------------------ |
| 1   | -127 °C             | Kein Sensor erkannt     | 4,7 kΩ Pull-up prüfen    |
| 2   | 85 °C               | Parasitärversorgung     | VCC-Anschluss hinzufügen |
| 15  | Schwankende Werte   | Interferenz             | Geschirmtes Kabel        |
| 21  | Aussetzender Fehler | Schlechte Verbindung    | Nachlöten                |

### MQTT / Home Assistant

| #   | Symptom               | Wahrscheinliche Ursache  | Schnelle Lösung             |
| --- | --------------------- | ------------------------ | --------------------------- |
| 3   | Nicht entdeckt        | Discovery nicht gesendet | Broker-Einstellungen prüfen |
| 4   | Nicht verfügbar       | Verbindung verloren      | Controller neustarten       |
| 14  | Verbindung verweigert | Falsche Zugangsdaten     | MQTT-Einstellungen prüfen   |

### Relais

| #   | Symptom                        | Wahrscheinliche Ursache  | Schnelle Lösung           |
| --- | ------------------------------ | ------------------------ | ------------------------- |
| 5   | Invertiertes Verhalten         | Falsche Polarität        | Active-Low-Modul verwenden|
| 6   | Klickt, aber Pumpe läuft nicht | Verdrahtungsfehler       | COM/NO prüfen             |
| 7   | Kein Klick                     | Keine Spannung am Relais | 5V-Versorgung prüfen      |
| 23  | Pumpe läuft bei AUS            | Relais verschweißt       | Relaismodul ersetzen      |

### Boot / Stabilität

| #   | Symptom             | Wahrscheinliche Ursache      | Schnelle Lösung      |
| --- | ------------------- | ---------------------------- | -------------------- |
| 8   | Safe Mode           | Bootloop                     | Serielles Log prüfen |
| 9   | Zufällige Neustarts | Schwaches Netzteil oder Heap | Besseres Netzteil    |
| 16  | Konfiguration weg   | NVS-Korruption               | Werksreset           |

### OTA / Firmware

| #   | Symptom                | Wahrscheinliche Ursache | Schnelle Lösung          |
| --- | ---------------------- | ----------------------- | ------------------------ |
| 10  | OTA fehlgeschlagen     | TLS/Speicher            | USB-Flash                |
| 11  | Startet nicht nach OTA | Fehlerhafte Firmware    | Bekannte Version flashen |
| 25  | Unbekannte Version     | Build-Problem           | Sauber neu bauen         |

---

## Serielles Log — Kurzreferenz

Auf diese Schlüsselwörter im seriellen Monitor achten:

| Log-Meldung                  | Bedeutung                                                 |
| ---------------------------- | --------------------------------------------------------- |
| `Safe Mode ACTIVE`           | Bootloop erkannt, Relais ausgeschaltet                    |
| `WDT reset`                  | Hardware-Watchdog ausgelöst (30s-Timeout)                 |
| `heap critical`              | Freier Heap unter 8 KB, Neustart bevorstehend             |
| `DS18B20 read failed`        | Temperatursensor-Kommunikationsfehler                     |
| `MQTT connection failed`     | Broker nicht erreichbar oder falsche Zugangsdaten         |
| `NTP sync failed`            | Zeitserver nicht erreichbar                               |
| `Configuration CRC mismatch` | NVS-Konfiguration korrupt, Standardwerte werden verwendet |
| `WiFi disconnected`          | Netzwerkverbindung verloren                               |
| `OTA update failed`          | Firmware-Upload-Fehler                                    |

---

## Siehe auch

- [Hardware-Guide](/docs/hardware-guide/) — Bau- und Verdrahtungsanleitung
- [Software-Guide](/docs/software-guide/) — Ersteinrichtung und Konfiguration
- [Sicherheits-Checkliste](/docs/security-checklist/) — Sicherheitshärtung
