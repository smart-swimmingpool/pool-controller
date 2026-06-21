---
title: OTA-Updates
summary: Over-The-Air Firmware-Update-Anleitung für den ESP32 Pool Controller — WebUI-Upload, GitHub-Release-Auto-Updates, PlatformIO-Seriell-Flashen und Sicherheitshinweise
date: "2026-06-11"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
tags: ["docs", "ota", "firmware", "update", "esp32"]
menu:
  docs:
    parent: Pool Controller
    name: OTA-Updates
    weight: 55
---

# Over-The-Air (OTA) Updates

## Übersicht

Der Pool Controller unterstützt Over-The-Air (OTA) Firmware-Updates, sodass
Sie das Gerät remote ohne physischen Zugriff aktualisieren können.

## Funktionen

- **WebUI-Firmware-Upload**: Eine signierte `.bin`-Datei über das Dashboard flashen
- **GitHub-Release-Updates**: Nach neuen Versionen suchen und automatisch installieren
- **PlatformIO-Seriell-Upload**: Per USB flashen für die Entwicklung
- **Status-Rückmeldung**: Fortschrittsanzeige über WebUI + MQTT

## Update-Methoden

### Methode 1: WebUI (Empfohlen)

Die einfachste Methode — keine Tools außer einem Browser nötig:

1. Pool Controller Dashboard im Browser öffnen
2. Zum Tab **System** gehen
3. Eine der Optionen wählen:
   - **Manueller Firmware-Upload**: Eine `.bin`-Datei auswählen und flashen
   - **Nach Updates suchen**: Lädt automatisch die neueste Version von GitHub
4. Dem Fortschrittsbalken folgen — das Gerät startet nach Abschluss neu

### Methode 2: GitHub Release (Automatisch)

Die Firmware wird bei jedem GitHub-Release automatisch gebaut und veröffentlicht:

1. Im Tab **System** auf **Nach Updates suchen** klicken
2. Wenn eine neuere Version gefunden wird, erscheint **Update installieren**
3. Klicken, um die neueste Firmware direkt von GitHub herunterzuladen und zu flashen
4. Der Fortschritt wird in der UI angezeigt; das Gerät startet nach Abschluss neu

### Methode 3: PlatformIO Seriell-Upload

Für die Entwicklung:

```bash
# Bauen und per USB flashen
pio run -e esp32dev --target upload

# Oder mit Projekt-Venv
./venv/bin/pio run -e esp32dev --target upload
```

## Firmware für OTA bauen

### Firmware-Binärdatei erstellen

```bash
# Firmware bauen ohne hochzuladen
pio run -e esp32dev

# Speicherort der Binärdatei
.pio/build/esp32dev/firmware.bin
```

## Sicherheitshinweise

### 1. Netzwerksicherheit

- WPA2/WPA3 WLAN-Verschlüsselung verwenden
- IoT-Geräte nach Möglichkeit in einem separaten VLAN isolieren

### 2. Firmware-Überprüfung

- Nur Firmware aus vertrauenswürdigen Quellen flashen (offizielle GitHub-Releases)
- Firmware-Builds vor dem Upload in die Produktion testen
- Zuerst auf einem Entwicklungsgerät testen
- Backup der funktionierenden Firmware-Version behalten

## Fehlerbehebung

### OTA-Upload fehlschlägt

**Problem**: Upload bricht mit Timeout-Fehler ab

**Lösungen**:

- Prüfen, ob das Gerät online ist: `ping pool-controller.local`
- Sicherstellen, dass das Gerät ausreichend freien Speicher hat (>50 KB)
- Timeout in `upload_flags` erhöhen

### Gerät nicht gefunden

**Problem**: Gerät in Netzwerkports nicht sichtbar

**Lösungen**:

- mDNS-Funktion prüfen: `avahi-browse -a` (Linux) oder Bonjour (Windows)
- IP-Adresse statt mDNS-Hostname verwenden
- Gerät neu starten und auf WLAN-Verbindung warten
- Prüfen, ob das Gerät im selben Netzwerk/Subnetz ist

### Upload erfolgreich, aber Gerät antwortet nicht

**Problem**: Upload abgeschlossen, aber das Gerät startet nicht neu oder läuft nicht mit neuer Firmware

**Lösungen**:

- Serielle Konsole auf Boot-Fehler prüfen
- Sicherstellen, dass die Firmware für die richtige Plattform gebaut wurde (ESP32)
- Prüfen, ob die Firmware in den Flash-Speicher passt
- Auf Speicherprobleme im seriellen Log prüfen
- Manuellen Neustart durchführen

### Speicherprobleme während OTA

**Problem**: OTA fehlschlägt wegen unzureichendem Speicher

**Lösungen**:

- Freier Speicher ist kritisch für OTA
- Systemmonitor verhindert OTA bei Speicher < 8 KB
- Gerät vor dem OTA-Versuch neu starten
- Logging während des Updates reduzieren

## OTA-Architektur

### Funktionsweise

1. **WebUI ODER GitHub**: Benutzer löst Update über Dashboard oder automatisierte Prüfung aus
2. **Firmware-Download**: Binärdatei wird von lokalem Upload oder GitHub-Release geholt
3. **Flash-Schreiben**: Neue Firmware wird auf die OTA-Partition geschrieben (ESP32 Update-Bibliothek)
4. **Verifizierung**: Firmware-Signatur und Integrität werden geprüft
5. **Neustart**: Automatischer Neustart mit neuer Firmware

### Speicheranforderungen

- **ESP32**: Mindestens 100 KB freier Heap für OTA
- **Flash**: Ausreichend Platz für Dual-Boot-Partitionen

### LWIP-Konfiguration

Das Projekt verwendet das Flag `PIO_FRAMEWORK_ARDUINO_LWIP2_LOW_MEMORY`, um den
Netzwerk-Stack-Speicherverbrauch zu optimieren und zuverlässige OTA-Updates zu gewährleisten.

## OTA-Status überwachen

Der OTA-Fortschritt ist in der WebUI-Fortschrittsanzeige und über die serielle Konsole sichtbar.

```bash
# PlatformIO Monitor
pio device monitor

# Auf Log-Meldungen achten
[OTA] Start
[OTA] Progress: 25%
[OTA] Progress: 50%
[OTA] Progress: 75%
[OTA] Success
```

## Wiederherstellungsverfahren

### Wiederherstellung nach fehlgeschlagenem OTA-Update

Wenn ein OTA-Update fehlschlägt und das Gerät nicht mehr reagiert:

1. **Wiederherstellung mit physischem Zugriff**:

   - Per USB-Seriell verbinden
   - Firmware per Seriell flashen: `pio run -e esp32dev --target upload`

2. **Bootloader-Wiederherstellung**:

   - ESP32-Bootloader ermöglicht serielle Wiederherstellung
   - BOOT-Taste beim Einschalten gedrückt halten
   - Firmware per esptool flashen

3. **Werksreset**:

   - Im WebUI: System-Tab → **Factory Reset**
   - Oder seriell: NVS + LittleFS löschen

## Zukünftige Erweiterungen

- [x] Webbasiertes OTA-Update-Interface (System-Tab)
- [x] Automatische Update-Prüfung von GitHub-Releases
- [ ] Rollback-Funktion auf vorherige Firmware
- [ ] A/B-Partitions-Updates für sicherere Updates
- [ ] Update-Planung über MQTT-Befehle

## Referenzen

- [PlatformIO OTA Guide](https://docs.platformio.org/en/latest/platforms/espressif32.html#over-the-air-ota-update)
- [ESP32 Arduino OTA](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ota.html)
- [GitHub Releases](https://github.com/smart-swimmingpool/pool-controller/releases)

## Support

Bei OTA-bezogenen Problemen:

- Issue öffnen: <https://github.com/smart-swimmingpool/pool-controller/issues>
- Diskussionen: <https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions>

---

**Hinweis**: OTA wird mit der ESP32 Arduino Update-Bibliothek implementiert, die
in das WebPortal integriert ist. GitHub-Release-Builds werden von der CI-Pipeline
(`release.yml`) erstellt.
