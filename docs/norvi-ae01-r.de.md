---
title: NORVI AE01-R Konfiguration
summary: Pool-Controller-Pinbelegung, Verdrahtung und Unterschiede bei Verwendung des industriellen NORVI IIOT-AE01-R anstelle eines Standard-ESP32-Dev-Boards
date: "2026-06-09"
lastmod: "2026-07-13"
draft: false
toc: true
type: docs
menu:
  docs:
    parent: Pool Controller
    name: NORVI AE01-R
    weight: 60
---

## Übersicht

Der [NORVI IIOT-AE01-R](https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/) ist ein
industrieller ESP32-WROOM32-Controller mit eingebauten Relais, Transistorausgängen,
Digitaleingängen, OLED-Display, RS-485 und Drucktastern — alles in einem
Hutschienengehäuse (IP20). Er ist CE-zertifiziert
(EN 61131-2, EN 61010-1) und für den dauerhaften Einsatz in Schaltschränken
ausgelegt.

**Hauptunterschiede zu einem Standard-ESP32-Dev-Board:**

| Merkmal                | Standard ESP32 Dev Board    | NORVI AE01-R                         |
| ---------------------- | --------------------------- | ------------------------------------ |
| Versorgungsspannung    | **5V** (USB)                | **24V DC**                           |
| Relaismodul            | Extern (benötigt 5V)        | **6 eingebaute Relais** (5A 250V AC) |
| Relais-Stromversorgung | Separates 5V-Netzteil       | Integriert — wird von 24V versorgt   |
| Statusanzeige          | Nur eingebaute LED          | **0,96" OLED-Display** + externe LED |
| Gehäuse                | Offene Platine (Breadboard) | **Hutschienengehäuse** (IP20)        |
| Zertifizierungen       | Keine                       | CE (EN 61131-2, EN 61010-1)          |
| Verdrahtung            | Schraub-/Dupont-Klemmen     | **Industrie-Schraubklemmen**         |
| Digitaleingänge        | Keine                       | **8x Sink/Source (18–32V DC)**       |
| Zusätzliche I/O        | Keine                       | 2x Transistorausgang, RS-485         |
| Benutzerschnittstelle  | Keine                       | **3 Drucktaster an der Front**       |

> **Wenn du bereits ein 230V AC → 24V DC Netzteil hast**, kann der NORVI AE01-R
> direkt daraus versorgt werden — kein separates 5V-Netzteil nötig.

---

## Pinbelegung

{{% callout warning %}}
**Wichtige Hardware-Einschränkung:** Die 8 Digitaleingänge des NORVI
(GPIO18/19/21/22/23/34/35/39) sind **optokoppler-isoliert und reine Eingänge**
— sie erwarten 18–32V DC Signale und können nicht für bidirektionale
1-Wire-Kommunikation (DS18B20) verwendet werden.
{{% /callout %}}

Der Firmware wird über das Präprozessor-Makro `NORVI_AE01_R` eine alternative
Pinbelegung ausgewählt, die die belegten und reinen-Eingang-Pins vermeidet:

| Konstante         |    NORVI GPIO     | Funktion                                    |                Anschluss                 |
| ----------------- | :---------------: | ------------------------------------------- | :--------------------------------------: |
| `PIN_DS_SOLAR`    |    **GPIO25**     | DS18B20 — Solar-Kollektortemperatur         |          Erweiterungsport Pin 1          |
| `PIN_DS_POOL`     |    **GPIO25**     | DS18B20 — Pool-Wassertemperatur             | Shared Bus auf GPIO25 (Erweiterungsport) |
| `PIN_RELAY_POOL`  |  **GPIO14** (R0)  | Relais — Pool-Umwälzpumpe                   |             Relaisausgang 0              |
| `PIN_RELAY_SOLAR` |  **GPIO13** (R2)  | Relais — Solarheizungspumpe (R1→R5→R2)      |             Relaisausgang 2              |
| `PIN_LED_STATUS`  | **GPIO27** (T0.1) | Status-LED (extern, über Transistorausgang) |          Transistorausgang 0.1           |
| `PIN_OLED_SDA`    |    **GPIO16**     | I2C SDA — eingebautes 0,96" OLED (SSD1306)  |               Intern (I2C)               |
| `PIN_OLED_SCL`    |    **GPIO17**     | I2C SCL — eingebautes 0,96" OLED (SSD1306)  |               Intern (I2C)               |
| `PIN_BUTTON_ADC`  |    **GPIO32**     | 3 Fronttaster (über Analog-ADC)             |        Intern (Widerstandsteiler)        |

### DS18B20-Sensoren anschließen — Shared OneWire Bus

Beide Sensoren teilen sich einen **gemeinsamen OneWire-Bus auf GPIO25** am
**Erweiterungsport** (10-polige Stiftleiste seitlich am NORVI-Gehäuse,
2,54 mm Raster). DATA, 3,3V und GND sind alle auf diesem Stecker verfügbar
— kein Löten nötig.

```
   NORVI AE01-R — Erweiterungsport (10-polige Stiftleiste, 2,54 mm)
   ┌─────────────────────────────────────────────────────────────────┐
   │                                                                 │
   │  Pin 1  ◄─────── IO25 ──── DS18B20 DATA (gelb)                 │
   │  Pin 2         TXD0                                            │
   │  Pin 3         NC                                              │
   │  Pin 4         RXD0                                            │
   │  Pin 5         IO0 (BOOT)                                      │
   │  Pin 6         IO32 (Taster)                                    │
   │  Pin 7  ◄─────── 3,3V ──── DS18B20 VCC (rot)                   │
   │  Pin 8         SCL (OLED)                                       │
   │  Pin 9  ◄─────── GND  ──── DS18B20 GND (schwarz)               │
   │  Pin 10        SDA (OLED)                                       │
   │                                                                 │
   └─────────────────────────────────────────────────────────────────┘
                              ↑
                  3× Female-Female Dupont-Kabel
```

**Anschlusstabelle:**

| Erweiterungsport | Dupont (Farbe) | DS18B20 | Auch verbunden mit |
| ---------------- | -------------- | ------- | ------------------ |
| Pin 1 (IO25)     | Gelb           | DATA    | 4,7 kΩ → 3,3V      |
| Pin 7 (3,3V)     | Rot            | VCC     | 4,7 kΩ → DATA      |
| Pin 9 (GND)      | Schwarz        | GND     | —                  |

Beide Sensoren teilen sich **dieselben drei Pins** — beide DATA an Pin 1,
beide VCC an Pin 7, beide GND an Pin 9. Ein einziger **4,7-kΩ-Pull-up**
zwischen Pin 1 und Pin 7 reicht für beide Sensoren.

> **Farbkodierung Pull-up Widerstand:** Gelb – Violett – Rot – Gold (±5%)
>
> ```
>   Gelb  Violett  Rot   Gold
>   ┌──┐  ┌──┐  ┌──┐  ┌──┐
>   │  │  │  │  │  │  │  │
>   └──┘  └──┘  └──┘  └──┘
>    4     7    ×100  ±5%
>   = 47 × 100 = 4700 Ω = 4,7 kΩ
> ```

{{% callout note %}}
**Shared-Bus-Details:** Die Firmware verwendet das Präprozessor-Makro
`NORVI_AE01_R`, um eine gemeinsame `DallasTemperature`-Instanz zu erstellen,
die von beiden Sensorknoten genutzt wird. Der Solarknoten (Geräteindex 0)
steuert `requestTemperatures()`; der Poolknoten (Geräteindex 1) liest aus
derselben Messung. Beide Sensoren benötigen einen 4,7-kΩ-Pull-up-Widerstand
zwischen DATA und 3,3V — ein Widerstand reicht, wenn sie denselben Bus teilen.
{{% /callout %}}

{{% callout tip %}}
**Bereits auf GPIO5 gelötet?** Falls der Pool-Sensor bereits am ESP32-Modulpin
angelötet ist, verwendet die Firmware den Einzelbus-Modus, wenn `NORVI_AE01_R`
**nicht** definiert ist. Du kannst die bestehende Verkabelung behalten und
einfach das Build-Flag weglassen — oder beide Sensoren auf den gemeinsamen
GPIO25 umziehen.
{{% /callout %}}

### Sensorzuordnung — Erster Start

Da beide Sensoren denselben Bus teilen, bestimmt die Scan-Reihenfolge, welcher
Sensor Solar (Index 0) und welcher Pool (Index 1) zugeordnet wird. Mit dem
**OLED-Setup-Assistenten** weist du die Sensoren dauerhaft zu.

#### OLED Setup-Assistent (empfohlen)

1. **Setup starten:** **B1** (linker Taster) ca. 2 Sekunden gedrückt halten.
2. Das Display zeigt beide gefundenen DS18B20-Sensoren mit ROM-Adresse und
   aktueller Temperatur.
3. **Sensor identifizieren:** Einen Sensor mit dem Finger erwärmen — die
   Temperatur steigt. **B2** drücken, um den markierten Sensor als **Solar**
   zuzuweisen, oder **B3** für **Pool**.
4. Beim zweiten Sensor wiederholen. Die Badges `SOLAR` bzw. `POOL` erscheinen.
5. **Speichern & Neustart:** Sind beide zugewiesen, **B2** lange drücken, um
   die Zuordnung im NVS zu speichern. Nach dem Neustart bleibt die Zuordnung
   dauerhaft erhalten.

> **Abbrechen:** **B1** erneut lange drücken, um ohne Speichern zu beenden.

#### Serielle Konsole

Beim ersten Start (ohne gespeicherte Zuordnung) erscheint:

```
• Sensor mapping: no addresses configured — using default device indices
  ℹ To configure: long-press Button 1 → Sensor Setup page → assign sensors
```

Die ROM-Adressen findest du im Boot-Log. Durch Erwärmen eines Sensors
identifizierst du, welche Adresse zu Solar und welche zu Pool gehört.

#### Funktionsweise

Die 8-Byte-ROM-Adresse jedes Sensors wird im NVS gespeichert (Preferences-Namespace
`ds18b20`, Keys `solar_adr` und `pool_adr`). Bei jedem folgenden Boot sucht die
Firmware auf dem Bus nach einem Gerät mit der gespeicherten Adresse. Wird es
gefunden, wird es unabhängig von der Scan-Reihenfolge verwendet. Wird es nicht
gefunden (z. B. nach Austausch), erscheint eine Warnung und der Standard-Index
kommt zum Einsatz.

---

## Schaltplan

```
 ─── STROMVERSORGUNG ──────────────────────────────────────────────────

  [230V AC → 24V DC Netzteil]     [NORVI AE01-R]
   ┌──────────────────┐          ┌──────────────────────────┐
   │ 24V (+) ─────────┼──────────┤ 24V DC (+)              │
   │                  │          │                          │
   │ GND  ────────────┼──────────┤ GND (0V)                │
   └──────────────────┘          │                          │
                                 │  Der NORVI versorgt sich │
                                 │  und die Relais aus 24V  │
                                 └──────────────────────────┘

  ─── SENSOREN (Shared OneWire Bus am Erweiterungsport) ─────────────

   ┌── DS18B20 Solar ──┐    ┌── DS18B20 Pool ───┐    NORVI Erweiterungsport
   │                   │    │                   │    ┌──────────────────────┐
   │ VDD (rot) ────────┼────┼──── VDD (rot) ────┼────┤ Pin 7 (3,3V)        │
   │                   │    │                   │    │                      │
   │ GND (sw)  ────────┼────┼──── GND (sw) ─────┼────┤ Pin 9 (GND)         │
   │                   │    │                   │    │                      │
   │DATA (gel) ────────┼────┼──── DATA (gel) ───┼────┤ Pin 1 (GPIO25)      │
   │                   │    │    │              │    └──────────────────────┘
   └───────────────────┘    │    │              │
                             │    │              │
                             │    └──[4,7kΩ]─────┼── 3,3V (ein Pull-up)
                             │                  │
                             └──────────────────┘

   Ein 4,7kΩ-Widerstand zwischen DATA (Pin 1) und 3,3V (Pin 7) reicht.
   Dupont-Kabel: 3× Female-Female (rot/gelb/schwarz).

   ─── RELAISAUSGÄNGE (eingebaut) ───────────────────────────────────────

   ╔══════════════════════════════════════════════════════════════════════╗
   ║  ⚠ WICHTIG — Motorlast beachten                                    ║
   ║                                                                     ║
   ║  Die NORVI-Bordrelais sind für **5A ohmsche Last** ausgelegt.       ║
   ║  Eine Pumpe ist eine Motorlast mit Einschaltstrom, der Relais-      ║
   ║  kontakte nach wiederholtem Schalten verschweißen kann.             ║
   ║                                                                     ║
   ║  **Standard-Konfiguration:** NORVI-Relais als **Steuerrelais für    ║
   ║  externe Hutschienen-Schütze** verwenden (siehe Schütz-Schaltung    ║
   ║  unten). Das ist verscheißfrei für die gesamte Lebensdauer.         ║
   ║                                                                     ║
   ║  **Für Kleinstpumpen (<100W, z. B. ECM-Solarpumpen):** direkt       ║
   ║  verdrahten mit **RC-Snubber** (100nF + 100Ω, X2) dämpft Abschalt-  ║
   ║  Lichtbögen, begrenzt aber **nicht** den Einschaltstrom — zur       ║
   ║  Vermeidung von Verschweißung das **Schütz** (oder NTC) verwenden.  ║
   ║                                                                     ║
   ║  Siehe → **[Schütz-Schaltung für Pumpen](contactor-guide.de.md)**   ║
   ╚══════════════════════════════════════════════════════════════════════╝

   NORVI Relaisausgang 0 (GPIO14):  Steuerung → Pool-Pumpe (via Schütz oder direkt <100W)
   NORVI Relaisausgang 2 (GPIO13):  Steuerung → Solar-Pumpe (via Schütz oder direkt mit Einschaltstrombegrenzer)

   L (Außenleiter) ─── RCD ─── MCB ──┬── Relais COM0 ── NO0 ── Pumpe L
                                      └── Relais COM2 ── NO2 ── Pumpe L
   N (Neutral) ──────────────────────────── Neutralleiter ── Pumpen N

   ── Standard-Verdrahtung (mit Schützen) ─────────────────────────────

   24V DC (+) ──┬── NORVI R0 COM ── NO ──┬── Ext. Schütz Pool A1 ── A2 ──┬── GND
                 │                        │                               │
                 ├── NORVI R2 COM ── NO ──┤── Ext. Schütz Solar A1 ── A2 ──┤
                 │                        │                               │
                 └───── NORVI 24V IN ─────┘                               │
                                                                          │
   GND ──────────────────────────────────────────────────────────────────┘

   Jeweils eine 1N4007-Freilaufdiode parallel zur Schützspule
   (Kathode an A1, Anode an A2). Die 230V-Lastseite:

   L (Außenleiter) ─── RCD ─── MCB ──┬── Schütz Solar 1-2 ── Solarpumpe L
                                      └── Schütz Pool 1-2 ─── Poolpumpe L
   N (Neutral) ──────────────────────────── Neutralleiter ── Beide Pumpen N

   ── Alternative: Direktverdrahtung (ohmsche / Kleinstlast) ──────────

   Für **Nicht-Motorlasten** (Heizstäbe, Ventile, Signallampen) oder
   **Kleinst-EcmPumpen (<100W) mit Einschaltstrombegrenzer** können die
   NORVI-Relais direkt verdrahtet werden. Sie sind Schließer (SPST), 5A/250V AC.

   L (Außenleiter) ─── RCD ─── MCB ──┬── NORVI COM0 ── NO0 ── Last
                                      └── NORVI COM2 ── NO2 ── Last
   N (Neutral) ──────────────────────────── Neutralleiter ── Last N

   > **Für Pumpen (Motorlasten):** Externe Schütze wie oben verwenden.
   > Direktverdrahtung von Motorlasten führt zu Kontaktverschweißung;
   > ein RC-Snubber verhindert das nicht.

   ── Feldbericht ──────────────────────────────────────────────────────

   In einer Feldinstallation wurde eine Solarpumpe an Relaisausgang 1
   (GPIO12, R1) betrieben. Nach einigen hundert Schaltspielen hatte R1
   einen verschweißten Kontakt — Relais hörbar, aber NO-Kontakt öffnete
   nie. Umverdrahtung auf R5 (GPIO33) zeigte denselben Fehler. R0
   (GPIO14, Poolpumpe) lief weiter (andere Motorkennlinie). Die
   Solarpumpe ist jetzt auf R2 (GPIO13) konfiguriert. Für diese Last ist
   ein **Schütz** (oder Einschaltstrombegrenzer) empfohlen — ein RC-Snubber
   parallel zu den R2-Kontakten verhindert das Verschweißen nicht.
   Im Gegensatz zu externen Relaismodulen (die
   typischerweise
   active-LOW sind: LOW = EIN, HIGH = AUS) sind die eingebauten NORVI-Relais
   **active-HIGH**: `HIGH` auf dem GPIO-Pin erregt die Relaisspule (Schließer
   schließt), `LOW` entregt sie (Schließer öffnet).

   Die Firmware handhabt dies automatisch über den `activeLow`-Parameter des
   `RelayModuleNode`-Konstruktors. NORVI-Builds erstellen die Relais-Instanzen
   mit `activeLow = false` (siehe `PoolController.cpp`). Bei der Portierung auf
   ein anderes Board mit eingebauten Relais sollte die Relais-Treiber-Polarität
   geprüft werden.

 ─── STATUS-LED (extern) ──────────────────────────────────────────────

   NORVI Transistorausgang 0.1 (GPIO27):
     GPIO27 ─── 330Ω ─── LED (Anode) ─── LED (Kathode) ─── GND

   Der Transistorausgang ist Open-Collector (100mA max). Wenn der Pin
   auf HIGH geht, schaltet der interne Transistor auf GND und schließt
   den Stromkreis. Verwende eine Standard-5mm-LED mit 330Ω Vorwiderstand.
```

---

## Integriertes OLED-Display

Der NORVI AE01-R hat ein **0,96" SSD1306 OLED-Display** (128×64, I2C-Adresse
0x3C). Die Firmware zeigt vier Informationsseiten:

| Seite | Inhalt                                                                                             |
| :---: | -------------------------------------------------------------------------------------------------- |
| **1** | Pool- & Solar-Temperatur, Pumpenstatus, Betriebsmodus und Tasterhinweise (▲ Seite ▼ Pumpe ■ Modus) |
| **2** | WLAN-SSID, IP-Adresse, MQTT-Verbindungsstatus, AP-Modus-Hinweis                                    |
| **3** | System-Betriebszeit, freier Heap, minimaler Heap (Low-Watermark), Firmware-Version                 |
| **4** | QR-Code mit Link zum Web-Dashboard unter `http://<geräte-ip>/`                                     |

**Gemeinsame Fußzeile:** Alle Seiten zeigen unten eine Trennlinie mit dem
aktuellen Betriebsmodus (nur Seite 1), der lokalen Uhrzeit (HH:MM), der
Firmware-Version (`vX.Y.Z`) und der Seitennummer. Die Uhrzeit wird bei jeder
Aktualisierung neu gesetzt und verwendet die konfigurierte Zeitzone mit
automatischer Sommer-/Winterzeit (DST).

**Auto-WiFi-Seite:** Wenn kein WLAN konfiguriert ist (AP-Modus) oder die
Verbindung unterbrochen wurde, schaltet das Display beim Start automatisch auf
die **Netzwerkstatus-Seite** (Seite 2). So sieht der Benutzer sofort den
AP-Namen und die Setup-URL.

**QR-Code-Seite:** Die letzte Seite (Seite 4) generiert einen QR-Code für die
Web-Dashboard-URL (`http://<ip>/`). Scanne ihn mit der Smartphone-Kamera, um
das Dashboard zu öffnen, ohne die IP-Adresse eingeben zu müssen. Der QR-Code
wird direkt auf dem Gerät mit der Bibliothek `ricmoo/QRCode` erzeugt.

Das Display aktualisiert sich alle 2 Sekunden und wird über die Fronttaster
gesteuert (siehe unten).

---

## Fronttaster

Die drei Drucktaster an der Front werden über den Analog-ADC auf GPIO32
ausgelesen. Jeder Taster erzeugt durch einen integrierten Widerstandsteiler
einen eigenen Spannungspegel:

|     Taster     | Aktion                                                      |
| :------------: | ----------------------------------------------------------- |
| **1** (links)  | OLED-Seite vorwärts blättern (1 → 2 → 3 → 4 → 1)            |
| **2** (mitte)  | Im **manuellen** Modus: Pool-Pumpe EIN/AUS schalten         |
| **3** (rechts) | Betriebsmodus wechseln (auto → manu → boost → timer → auto) |

Die ADC-Schwellwerte sind in `NorviButtonHandler.hpp` konfiguriert. Die
Rohwerte werden beim Start über die serielle Schnittstelle ausgegeben.

---

## Stromversorgung

Der NORVI AE01-R arbeitet mit **24V DC** (±10 %). Die eingebauten Relais und
der ESP32 werden gemeinsam aus dieser Spannung versorgt.

| Parameter            | Wert                         |
| -------------------- | ---------------------------- |
| Nennspannung         | **24V DC**                   |
| Stromaufnahme        | **400 mA** (alle Relais aus) |
| Empfohlenes Netzteil | **1A 24V DC** (oder mehr)    |

> **Hinweis:** Im Gegensatz zum Standard-ESP32-Dev-Board-Setup benötigt der
> NORVI **kein** separates 5V-USB-Netzteil oder 5V-Relaismodul-Netzteil. Alles
> wird aus der einzelnen 24V-DC-Einspeisung versorgt.

---

## Unterschiede zum Standard-Setup

| Aspekt           | Standard ESP32 Dev Board | NORVI AE01-R                                        |
| ---------------- | ------------------------ | --------------------------------------------------- |
| **Strom**        | 5V USB oder 5V Netzteil  | 24V DC (eine Versorgung)                            |
| **Relais**       | Externes 2-Kanal-Modul        | 6 eingebaut (wir nutzen 2)                          |
| **Relais-Strom** | 5V an Relaismodul             | Integriert (24V → Relaisspulen)                     |
| **Relais-Polarität** | Active-LOW (LOW = EIN)   | **Active-HIGH** (HIGH = EIN)                        |
| **Sensor-Pins**  | GPIO32, GPIO33           | GPIO25 (Exp Port), GPIO5 (löten)                    |
| **Relais-Pins**  | GPIO25, GPIO26           | GPIO14 (R0), GPIO13 (R2)                            |
| **Status-LED**   | Eingebaut (GPIO2)        | Extern über GPIO27 (T0.1)                           |
| **OLED-Display** | Keines                   | Eingebaut 0,96" (SSD1306) — 4 Info-Seiten + QR-Code |
| **Drucktaster**  | BOOT-Taster (GPIO0)      | 3 Fronttaster — Seiten & Modi                       |
| **Montage**      | Breadboard / Gehäuse     | **Hutschiene** (Standard-Schaltschrank)             |

### Nicht mehr benötigt

- **Externes Relaismodul** — entfallen (eingebaut)
- **Separates 5V-Netzteil für Relais** — entfallen (24V integriert)
- **5V-USB-Netzteil** — entfallen (ein 24V-Netzteil)
- **Breadboard / Lochrasterplatine** — entfallen (Schraubklemmen)

### Weiterhin gleich

- DS18B20-Sensoren mit 4,7kΩ-Pull-up-Widerständen
- 230V AC Verdrahtung (RCD/MCB-Schutz, Pumpenanschlüsse)
- MQTT / Home Assistant / Homie Convention — gesamte Software identisch

---

## Firmware bauen

Die NORVI-Variante wird zur Compile-Zeit über das Präprozessor-Makro
`NORVI_AE01_R` ausgewählt. Nutze die dedizierte PlatformIO-Umgebung:

```bash
pio run -e norvi_ae01_r -t upload
```

Diese Umgebung:

- Definiert `-D NORVI_AE01_R` für die alternative Pinbelegung (Shared OneWire Bus)
- Bindet `Adafruit SSD1306` und `Adafruit GFX Library` für das OLED ein
- Initialisiert I2C auf GPIO16/GPIO17 und den Taster-ADC auf GPIO32
- Erzeugt Relais-Instanzen mit `activeLow = false` für die eingebauten active-HIGH-Relais

Zum Hochladen des LittleFS-Dateisystems (Web-Oberfläche):

```bash
pio run -e norvi_ae01_r -t uploadfs
```

> **Hinweis:** Der USB-Anschluss des NORVI dient der Programmierung, teilt
> sich aber die UART mit RS-485. `Serial.print()` kann die RS-485-
> Kommunikation stören — nutze bei aktivem RS-485 das OLED-Display zur
> Debug-Ausgabe.

---

## NORVI AE01-R Pin-Referenz

|  GPIO  | NORVI-Funktion                             | Vom Pool-Controller genutzt? |
| :----: | ------------------------------------------ | :--------------------------: |
|   0    | NRST (PWM beim Boot)                       |              ❌              |
|   1    | RS-485 TX (mit USB geteilt)                |              ❌              |
| **2**  | **Relaisausgang 4 / Eingebaute LED**    |              ❌              |
|   3    | RS-485 RX (mit USB geteilt)                |              ❌              |
|   4    | RS-485 Flusskontrolle                      |              ❌              |
| **5**  | **— (freier GPIO, kein NORVI-Peripherie)** | ✅ **DS18B20 Pool (löten)**  |
|   12   | Relaisausgang 1 (verschweißt — R2 verwenden) |              ❌              |
| **13** | **Relaisausgang 2**                        |   ✅ **Solar-Pumpe (R2)**    |
|   14   | **Relaisausgang 0**                        |        ✅ Pool-Pumpe         |
|   15   | Relaisausgang 3                            |              ❌              |
|   16   | **I2C SDA** (OLED-Display)                 |       ✅ OLED-Display        |
|   17   | **I2C SCL** (OLED-Display)                 |       ✅ OLED-Display        |
|   18   | Digitaleingang 0 (nur Eingang)             |              ❌              |
|   19   | Digitaleingang 4 (nur Eingang)             |              ❌              |
|   20   | —                                          |              ❌              |
|   21   | Digitaleingang 5 (nur Eingang)             |              ❌              |
|   22   | Digitaleingang 6 (nur Eingang)             |              ❌              |
|   23   | Digitaleingang 7 (nur Eingang)             |              ❌              |
| **25** | **Erweiterungsport Pin 1**                 |     ✅ **DS18B20 Solar**     |
|   26   | Transistorausgang 0.0                      |          ❌ (frei)           |
|   27   | **Transistorausgang 0.1**                  |    ✅ Status-LED (extern)    |
| **32** | **Analogeingang / Taster**                 |       ✅ 3 Fronttaster       |
|   33   | Relaisausgang 5                            |              ❌              |
|   34   | Digitaleingang 2 (nur Eingang)             |              ❌              |
|   35   | Digitaleingang 3 (nur Eingang)             |              ❌              |
|   38   | —                                          |              ❌              |
|   39   | Digitaleingang 1 (nur Eingang)             |              ❌              |

> **Hinweis:** Als "nur Eingang" markierte GPIOs (GPIO34, 35, 39) haben keine
> internen Pull-ups und sind an Optokoppler-Schaltungen angeschlossen. Sie
> können nicht für 1-Wire oder andere bidirektionale Protokolle verwendet
> werden.

---

## Referenzen

- [NORVI IIOT-AE01-R Datenblatt](https://norvi.io/docs/norvi-iiot-ae01-r-datasheet/)
- [NORVI IIOT-AE01-R Benutzerhandbuch](https://norvi.io/docs/norvi-iiot-ae01-r-user-guide/)
- [KiCad-Schaltplan: NORVI AE01-R](kicad/norvi-ae01-r/norvi-ae01-r-schematic.pdf) — KiCad 9.0 PDF-Export des Schaltplans
- [Haupt-Hardware-Anleitung](hardware-guide.de.md)
- [Schütz-Schaltung für Pumpen](contactor-guide.de.md) — Externe Schütze für dauerhaft zuverlässiges Pumpenschalten
- [SVG-Schaltplan: Schütz-Schaltung](wiring-contactor.svg)
- [Config.hpp Pin-Quelle](https://github.com/smart-swimmingpool/pool-controller/blob/main/src/Config.hpp)
