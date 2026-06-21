---
title: Benutzerhandbuch
summary: Pool-Controller-Benutzerhandbuch — WLAN-Einrichtung über AP-Modus und WPS, Web-Dashboard-Erklärung, alle Betriebsmodi (Auto/Timer/Boost/Manuell), MQTT Discovery und openHAB-Integration
date: "2020-05-28"
lastmod: "2026-06-11"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "controller", "tutorial"]
menu:
  docs:
    parent: Pool Controller
    name: Benutzerhandbuch
    weight: 40
---

## Einrichtung

### Erste WLAN-Einrichtung über die Weboberfläche (AP-Modus)

Wenn der Controller kein WLAN konfiguriert hat, startet er im **Access-Point-Modus**:

1. Controller einschalten
2. Telefon/Laptop mit dem WLAN **`Pool-Controller-Setup`** verbinden (offenes Netzwerk)
3. Browser öffnen und **`http://192.168.4.1`** aufrufen — der Captive Portal leitet automatisch weiter
4. Zum Tab **WLAN-Einrichtung** gehen, Netzwerke scannen, das eigene auswählen und Passwort eingeben
5. Der Controller speichert die Konfiguration und startet im **STA-Modus** neu
6. Die IP des Controllers in der DHCP-Liste des Routers finden und neu verbinden

> **Hinweis:** Im AP-Modus hat die Weboberfläche kein Passwort (absichtlich für die Ersteinrichtung).
> Sobald der Controller mit Ihrem WLAN verbunden ist, ist ein Login erforderlich (Standardpasswort: `admin`).

### Erste WLAN-Einrichtung per WPS

Sie können WPS während des Bootvorgangs auslösen, um den Controller ohne manuelle
Eingabe der WLAN-Zugangsdaten mit Ihrem Router zu koppeln:

1. Controller normal einschalten (oder zurücksetzen) und warten, bis das Firmware-Setup beginnt.
2. Dann die **BOOT**-Taste mindestens 2 Sekunden lang gedrückt halten.
3. **WPS Push Button Connect (PBC)** auf Ihrem Router starten.
4. Warten, bis die Kopplung abgeschlossen ist.

Bei erfolgreichem WPS aktualisiert der Controller die WLAN-Zugangsdaten und speichert
sie dauerhaft in `/config.json` auf LittleFS.

### Controller im Netzwerk finden

Nach erfolgreicher Verbindung die IP des Controllers ermitteln:

- DHCP-Client-Liste des Routers prüfen
- Oder einen Netzwerkscanner wie `nmap` verwenden:

  ```bash
  nmap -p 80 192.168.1.0/24  # Subnetz nach offenem Port 80 scannen
  ```

- Die Weboberfläche zeigt die zugewiesene IP in der Dashboard-Kopfzeile an

## OLED-Display (NORVI AE01-R Variante)

Der industrielle NORVI AE01-R Controller verfügt über ein **0,96" OLED-Display**
(128×64 Pixel) auf der Frontseite. Wenn Ihre Firmware mit dem Flag
`NORVI_AE01_R` gebaut wurde, zeigt das Display Systeminformationen auf vier
umschaltbaren Seiten:

| Seite | Anzeige |
|:-----:|---------|
| **1** | **Hauptseite** — Pool- und Solar-Wassertemperaturen mit Pumpen-EIN/AUS-Anzeige (gefüllter/leerer Kreis), Betriebsmodus (auto/manu/boost/timer) und Abkürzungshinweise für die drei Fronttasten |
| **2** | **Netzwerk** — Verbundene WLAN-SSID, IP-Adresse, MQTT- Verbindungsstatus. Im AP-Modus werden Setup-Hotspot-Name und URL angezeigt |
| **3** | **System** — Betriebszeit (z.B. 3d 12h 30m), aktueller und minimaler freier Heap-Speicher, Firmware-Version |
| **4** | **QR-Code** — Erzeugt einen scannbaren QR-Code der Web-Dashboard-URL, zum Öffnen auf dem Smartphone ohne IP-Eingabe |

**Fußzeile:** Jede Seite hat eine untere Leiste mit der Ortszeit (HH:MM, mit
automatischer Sommerzeitumstellung), der Firmware-Version (`vX.Y.Z`) und der
aktuellen Seitenzahl. Auf Seite 1 wird auch der aktive Betriebsmodus in der
Fußzeile angezeigt.

**Auto-WLAN-Seite:** Wenn noch kein WLAN konfiguriert ist (AP-Modus), springt
das Display beim Start direkt zu Seite 2, damit die Einrichtungshinweise sofort
sichtbar sind.

**QR-Schnellzugriff:** Mit der Kamera-App des Smartphones den QR-Code auf Seite 4
scannen — die meisten modernen Telefone erkennen die URL und bieten an, sie im
Browser zu öffnen. Keine zusätzliche App erforderlich.

Das Display aktualisiert sich alle 2 Sekunden. Die Seiten werden mit den
Fronttasten umgeschaltet (siehe [NORVI AE01-R Dokumentation](norvi-ae01-r.de.md)
für Details).

## LED-Statuscodes (Homie-Konvention)

Der Controller verwendet die **eingebaute LED** zur Signalisierung des aktuellen
Systemzustands, nach der [Homie Convention](https://homieiot.github.io/) — einem
Industriestandard für IoT-Statusanzeigen.

| LED-Muster | Wann? | Bedeutung |
|------------|-------|-----------|
| **Langsames Blinken** (1x/Sek.) | **WLAN-Verbindung** | Der Controller versucht, sich mit Ihrem Heim-WLAN zu verbinden. Dauert 5–20 Sekunden. |
| **Meist an, kurzes Ausblinken alle 2s** | **WLAN OK, MQTT verbindet/getrennt** | Netzwerk steht, aber der MQTT-Broker ist noch nicht verbunden. Broker-Adresse oder Netzwerk prüfen. |
| **Schnelles Blinken** (5x/Sek.) | **AP-Modus / Setup** | Keine WLAN-Zugangsdaten gespeichert. Der Controller hostet ein eigenes `Pool-Controller-Setup`-WLAN. |
| **Dauerhaft an** | **Voll verbunden** | Alles läuft normal — WLAN + MQTT verbunden. |
| **Sehr schnelles Blinken** (10x/Sek.) | **OTA-Update läuft** | Firmware wird heruntergeladen und geflasht. Nicht ausschalten! |
| **Doppelblinken** (zwei Blitze, Pause) | **Safe Mode / Fehler** | Boot-Loop erkannt oder kritische Systemverschlechterung. Relais sind ausgeschaltet. |

### Was beim ersten Einschalten zu erwarten ist

1. **Einschalten** des Controllers
2. **Schnelles Blinken** — AP-Modus (noch kein WLAN konfiguriert)
3. Netzwerk `Pool-Controller-Setup` öffnen und WLAN konfigurieren
4. **Langsames Blinken** — Verbindung zum Heim-WLAN
5. **Meist an** — WLAN verbunden, MQTT verbindet
6. **Dauerhaft an** — alles läuft normal

> **Fehlersuche**: Wenn die LED **niemals das schnelle Blinken verlässt**, hat der
> Controller keine WLAN-Zugangsdaten oder kann Ihr Netzwerk nicht finden. Mit dem
> `Pool-Controller-Setup`-Zugangspunkt verbinden, um ihn zu konfigurieren. Bleibt
> es beim **Doppelblinken**, ist ein kritischer Fehler aufgetreten — das
> serielle Log prüfen.

## Web-Dashboard

Der Controller bietet ein modernes Web-Dashboard unter `http://<controller-ip>/`
mit den folgenden Tabs:

- **Dashboard** — Live-Telemetrie: Pool-/Solar-/Controller-Temperatur, Pumpenzustände, freier Heap, RSSI, Betriebsmodus
- **WLAN-Einrichtung** — WLAN-Zugangsdaten scannen und konfigurieren
- **MQTT-Einstellungen** — Broker-Host, Port, Benutzername, Passwort
- **Konfiguration** — Betriebsmodus, Temperaturgrenzen, Hysterese, Timer, Zeitzone
- **Sicherheit & Update** — Admin-Passwort ändern, OTA-Firmware-Update, Neustart, Werksreset

### Einstellungen über die Weboberfläche ändern

1. Mit Ihrem Admin-Passwort anmelden (Standard: `admin`)
2. Zum Tab **Konfiguration** navigieren
3. Einstellungen nach Bedarf anpassen:

    - **Betriebsmodus**: Automatik (Solar), Manuelle Steuerung, Boost-Pumpe, Timer-Zeitplan
    - **Max. Pool-Temp.**: Zielwassertemperatur (°C)
    - **Min. Solar-Temp.**: Minimale Solarkollektortemperatur (°C)
    - **Temperatur-Hysterese**: Schaltdifferenz zur Vermeidung schnellen Toggeln (K)
    - **Loop-Intervall**: Wie oft der Controller Regeln auswertet (Sekunden)
    - **Zeitzone**: Auswahl aus 10 unterstützten Zeitzonen mit Sommerzeit

4. **Parameter speichern** klicken — Änderungen sind **sofort aktiv** und **überdauern Neustarts**
5. Wenn MQTT/Home Assistant verbunden ist, werden die neuen Werte automatisch veröffentlicht

> **Tipp:** Sie können Einstellungen auch programmatisch über die REST-API ändern —
> siehe [Software-Entwicklung](software-guide.de.md#weboberfläche--direktzugriff).

## Einstellungen

Es gibt einige spezifische Einstellungen für den Controller:

- **Pool max. Temperatur:** Die maximale Wassertemperatur im Pool, die nicht überschritten werden soll.

  - Einheit: `°C`
  - Standardwert: `29`

- **Solar min. Temperatur:** Die minimale Temperatur des Wärmespeichers, die nicht unterschritten werden soll.

  - Einheit: `°C`
  - Standardwert: `50`

- **Hysterese:** Hysterese in Kelvin, die prüft, ob die Heizung ein- oder ausgeschaltet werden soll, um schnelles Umschalten zu verhindern.

  - Einheit: `K`
  - Standardwert: `1`

- **Pumpen-Timer:** Zeitbereich, in dem die Poolpumpe laufen soll.

  - Start h/min
  - Ende h/min

- **Loop-Intervall:**

  - Einheit: `sec`
  - Standardwert: `30`

- **Zeitzone:** Wählen Sie die Zeitzone für den Controller zur korrekten lokalen Zeit und Sommerzeitumstellung (DST).

  - Verfügbare Zeitzonen:
    - `0` - Mitteleuropäische Zeit (Berlin, Paris) mit CEST/CET DST
    - `1` - Osteuropäische Zeit (Helsinki, Athen) mit EEST/EET DST
    - `2` - Westeuropäische Zeit (London, Lissabon) mit BST/GMT DST
    - `3` - US Eastern Time (New York, Washington) mit EDT/EST DST
    - `4` - US Central Time (Chicago, Houston) mit CDT/CST DST
    - `5` - US Mountain Time (Denver) mit MDT/MST DST
    - `6` - US Pacific Time (Los Angeles, San Francisco) mit PDT/PST DST
    - `7` - Australian Eastern Time (Sydney, Melbourne) mit AEDT/AEST DST
    - `8` - Japan Time (Tokio) - Keine DST
    - `9` - China Time (Peking) - Keine DST
  - Standardwert: `0` (Mitteleuropäische Zeit)
  - Diese Einstellung kann bei der Ersteinrichtung konfiguriert oder zur Laufzeit über MQTT geändert werden
  - **Hinweis:** Laufzeitänderungen über MQTT (Betriebsmodus/Zeitzone) sind temporär.
    Um die Zeitzoneneinstellung über Neustarts hinweg zu speichern, die `timezone`-
    Konfiguration in den Geräteeinstellungen aktualisieren.

## Regeln

Der **Smart Swimmingpool Controller** implementiert `Regeln` für verschiedene Situationen:

### Regel: Manuell

Die Pumpe für Reinigung und Solarheizung werden vollständig manuell und unabhängig ein- und ausgeschaltet.

### Regel: Timer

Diese Regel schaltet die Reinigungspumpe basierend auf den Timer-Einstellungen. Die Solarheizung ist deaktiviert.

### Regel: Auto

Diese Regel schaltet die Reinigungspumpe basierend auf den Timer-Einstellungen. Die Solarheizung wird
**intelligent** zugeschaltet, wenn die Reinigungspumpe durch den Timer eingeschaltet ist und der
Wärmespeicher eine ausreichende Temperatur hat.

Wenn die maximale Temperatur des Poolwassers erreicht ist, wird die Solarheizung deaktiviert.

### Regel: Boost

Heizung des Poolwassers mit voller Leistung.

## MQTT-Schnittstelle

Der **Smart Swimmingpool Controller** verwendet [MQTT](http://mqtt.org/) zur
Kommunikation mit Ihrem Smart Home. Er unterstützt
[Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
zur automatischen Geräteregistrierung — Geräte erscheinen automatisch in Home Assistant
ohne manuelle Konfiguration.

## openHAB-Integration

Der **Smart Swimmingpool Controller** kann seit Version 2.4 in [openHAB](https://www.openhab.org) integriert werden.

Es ist möglich, mit dem Controller zu interagieren, um die Pumpe ein-/auszuschalten oder die aktuelle Regel zu wechseln.

Auch die aktuellen Temperatur- oder Zustandswerte können überwacht werden.

Schließlich können auch die [Einstellungen](#einstellungen) aktualisiert werden.

- TODO: Beispiel für openHAB-Konfiguration hinzufügen.

### Gerät

### Eigenschaften
