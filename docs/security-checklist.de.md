---
title: Sicherheits-Checkliste
summary: Checkliste für die Sicherheitshärtung des Pool-Controllers im Produktionseinsatz — Standard-Passwortänderung, OTA-Sicherheit, MQTT-Konfiguration, Netzwerkisolierung und Firmware-Integrität
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
tags: ["docs", "sicherheit", "checkliste", "härtung"]
menu:
  docs:
    parent: Pool Controller
    name: Sicherheits-Checkliste
    weight: 160
---

> ⚠️ **WARNUNG**: Einige Sicherheitsmaßnahmen beziehen sich auf **230V AC-
> Netzspannung** (Relais-Test, Netzteilprüfung). **Vor Arbeiten an der
> Schaltung immer stromlos schalten.** Im Zweifel eine qualifizierte
> Elektrofachkraft beauftragen.

## Überblick

Diese Checkliste umfasst Sicherheitsmaßnahmen zur Härtung des Pool-Controllers.
Obwohl der Controller im lokalen Netzwerk läuft und Pumpen (keine
sicherheitskritischen Systeme) steuert, verhindern diese Empfehlungen
unbefugten Zugriff und reduzieren die Angriffsfläche.

---

## Kritisch (Muss umgesetzt werden)

### 1. Standard-Passwort ändern

Das werksseitige Web-Interface-Passwort **muss** vor dem Anschluss an ein
Netzwerk geändert werden.

- **Standard**: `admin`
- **Aktion**: Web-UI → Security & Update → Passwort ändern
- **Anforderungen**:
  - Mindestens 8 Zeichen
  - Mischung aus Groß-/Kleinbuchstaben, Ziffern und Sonderzeichen
  - Kein wiederverwendetes Passwort aus anderen Diensten

### 2. OTA-Update-Passwort ändern

Das OTA-Update-Passwort ist getrennt vom Web-UI-Passwort.

- **Standard**: Wie Web-UI-Passwort
- **Aktion**: Web-UI → Security & Update → OTA-Passwort
- **Warum**: Verhindert unbefugte Firmware-Uploads

### 3. MQTT-Authentifizierung verwenden

- **Aktion**: MQTT-Benutzername und -Passwort in den Controller-Einstellungen
  konfigurieren
- **Warum**: Unauthentifiziertes MQTT erlaubt jedem im Netzwerk, auf
  Controller-Topics zu publizieren
- **Empfehlung**: Dedizierten MQTT-Benutzer für den Controller mit minimalen
  Berechtigungen anlegen

### 4. MQTT-Broker nur intern erreichbar

- **Aktion**: MQTT-Broker nur an lokale Netzwerkschnittstelle binden
- **Warum**: Ein öffentlich erreichbarer MQTT-Broker kann gefunden und
  missbraucht werden
- **Mosquitto-Konfiguration**:

  ```ini
  listener 1883 192.168.1.0/24
  allow_anonymous false
  password_file /etc/mosquitto/passwd
  ```

- **Port 1883 oder 8883 NIEMALS** für das Internet freigeben

---

## Hoch (Dringend empfohlen)

### 5. Web-Interface nur im LAN

- Das Web-Interface hört standardmäßig auf allen Schnittstellen
- **Aktion**: Sicherstellen, dass sich der Controller in einem vertrauenswürdigen
  lokalen Netzwerk befindet
- **Port 80/443 des Controllers NIEMALS** für das Internet freigeben
- Bei Bedarf nach Fernzugriff: VPN (WireGuard, Tailscale, OpenVPN) oder
  Home Assistant sicheren Fernzugriff verwenden

### 6. Nur offizielle Firmware verwenden

- **Aktion**: Ausschließlich Firmware aus offiziellen GitHub-Releases flashen
- **Warum**: Inoffizielle Firmware könnte Hintertüren oder unsichere
  Modifikationen enthalten
- **Prüfung**:
  - Von [offiziellen Releases](https://github.com/smart-swimmingpool/pool-controller/releases) herunterladen
  - Release-Tag mit der erwarteten Version vergleichen
  - Bei Bedarf selbst aus dem Quellcode bauen

### 7. Backup vor OTA-Update

- **Aktion**: Screenshot oder Export der aktuellen Konfiguration vor dem
  Firmware-Update speichern
- **Warum**: OTA-Fehler können das Dateisystem beschädigen und ein
  erneutes Flashen erforderlich machen
- **Hinweis**: Die Konfiguration des Controllers wird im NVS gespeichert und
  überlebt in den meisten Fällen ein erneutes Flashen

---

## Mittel (Empfohlen)

### 8. Relais vor Lastanschluss testen

- **Aktion**: Relais-Funktion mit Multimeter prüfen, bevor 230V AC-Pumpen
  angeschlossen werden
- **Warum**: Ein defektes Relais könnte eine Pumpe permanent eingeschaltet
  lassen, was ein Sicherheitsrisiko darstellt
- **Vorgehen**: Siehe [Von Null aufgebaut → Relais-Test](/docs/build-from-zero/#schritt-7-relais-test-ohne-last)

### 9. Dediziertes Netzwerk-VLAN verwenden

Für fortgeschrittene Aufbauten:

- ESP32 in ein **IoT-VLAN** mit eingeschränktem Internetzugriff legen
- Nur erlauben: MQTT-Broker, NTP-Server, Home Assistant
- Alle eingehenden Verbindungen aus dem Internet blockieren

### 10. Unbenutzte Funktionen deaktivieren

- **Aktion**: Wenn OTA nicht genutzt wird, in der Konfiguration deaktivieren
- **Aktion**: Wenn das Web-Interface nicht benötigt wird, Zugriff über
  Netzwerk-Firewall-Regeln einschränken
- **Warum**: Weniger Angriffsfläche reduziert Risiken

---

## Netzwerksicherheit-Übersicht

```text
Internet
    │
    │ (blockiert)
    ▼
┌──────────────┐     ┌────────────┐     ┌──────────────────┐
│ Heimrouter   │─────│ Lokales LAN│─────│ Pool Controller   │
│ (Firewall)   │     │ 192.168.x.x│     │ ESP32             │
└──────────────┘     └────────────┘     └──────────────────┘
                            │                    │
                            ▼                    ▼
                      ┌──────────────┐     ┌──────────────┐
                      │ MQTT-Broker  │     │ Home Assistant│
                      │ (nur lokal)  │     │ (nur lokal)   │
                      └──────────────┘     └──────────────┘
```

### Empfohlene Firewall-Regeln

| Quelle     | Ziel        | Port     | Protokoll | Aktion     | Zweck                    |
| ---------- | ----------- | -------- | --------- | ---------- | ------------------------ |
| Controller | MQTT-Broker | 1883     | TCP       | Erlauben   | MQTT-Kommunikation       |
| Controller | NTP-Server  | 123      | UDP       | Erlauben   | Zeitsynchronisation      |
| Controller | Internet    | 80       | TCP       | Blockieren | Kein Webzugriff nötig    |
| Controller | Internet    | 443      | TCP       | Blockieren | Kein Webzugriff nötig    |
| Internet   | Controller  | beliebig | beliebig  | Blockieren | Kein eingehender Zugriff |
| HA-Server  | Controller  | 80       | TCP       | Erlauben   | Web-UI / API             |

---

## Zugangsdaten-Verwaltung

### Speicherort der Zugangsdaten

| Zugangsdaten    | Speicher | Verschlüsselt?                           |
| --------------- | -------- | ---------------------------------------- |
| WLAN-Passwort   | NVS      | Nein (NVS standardmäßig unverschlüsselt) |
| MQTT-Passwort   | NVS      | Nein                                     |
| Web-UI-Passwort | NVS      | SHA-256-gehasht                          |
| OTA-Passwort    | NVS      | SHA-256-gehasht                          |

> **Hinweis**: NVS-Werte werden im Klartext auf dem Flash-Chip gespeichert.
> Physischer Zugriff auf das Gerät gefährdet alle Zugangsdaten.
> **Flash-Verschlüsselung** für Produktionsumgebungen aktivieren.

### Flash-Verschlüsselung (ESP32)

Für erweiterte Sicherheit die ESP32-Flash-Verschlüsselung aktivieren:

1. In menuconfig aktivieren: `Security features → Enable flash encryption`
2. Efuse brennen, um Verschlüsselung dauerhaft zu aktivieren
3. Firmware neu flashen
4. Alle NVS-Daten werden verschlüsselt gespeichert

> ⚠️ **Warnung**: Flash-Verschlüsselung ist auf den meisten ESP32-Modulen
> nicht umkehrbar. Nach der Aktivierung kann der Flash ohne
> Verschlüsselungsschlüssel nicht mehr gelesen werden.

---

## Verwandte Dokumente

- [Von Null aufgebaut](/docs/build-from-zero/) — Komplette Bauanleitung
- [Elektrische Sicherheit](/docs/electrical-safety/) — Sicherheitsinformationen
- [Sicherheitsmodell](/docs/safety-model/) — System-Sicherheitsarchitektur
- [Produktions-Checkliste](/docs/production-checklist/) — Prüfungen vor Inbetriebnahme
- [Fehlerbehebung](/docs/troubleshooting/) — Häufige Probleme und Lösungen
