---
title: "⚠️ Sicherheitshinweise & Warnungen"
summary: "Wichtige Sicherheitshinweise für den Aufbau und Betrieb des Smart Swimmingpool Controllers – 230V-Gefahren, Erdung, RCD, und mehr"
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "safety", "warning", "230V"]
menu:
  docs:
    parent: Pool Controller
    name: Sicherheitshinweise
    weight: 15
---

## ⚠️ **WICHTIGE SICHERHEITSHINWEISE**

> **⚠️ ACHTUNG: DIESES PROJEKT ARBEITET MIT 230V WECHSELSTROM!** > **FALSCHE HANDHABUNG KANN ZU SCHWEREN VERLETZUNGEN, BRAND ODER TOD FÜHREN!** > **LESSEN SIE DIESE HINWEISE VOR DEM AUFBAU UNBEDINGT DURCH!**

---

## 🔌 **Allgemeine Sicherheitsregeln für 230V-Anlagen**

### ✅ **DOs (Was Sie TUN müssen)**

1. **Arbeiten Sie NUR an spannungsfreien Schaltkreisen!**

   - **Schalten Sie immer die Stromversorgung AB**, bevor Sie an der Verdrahtung arbeiten.
   - **Prüfen Sie mit einem Spannungsprüfer**, ob die Leitung tatsächlich spannungsfrei ist.
   - **Verwenden Sie eine zweipolige Spannungsprüfer** (Phase + Nullleiter).

2. **Verwenden Sie immer einen FI-Schalter (RCD) mit 30mA Auslösestrom!**

   - Ein **RCD (Residual Current Device)** schützt vor **Stromschlägen** durch Isolationsfehler.
   - **Mindestens 30mA Auslösestrom** (für Personenschutz).
   - **Testen Sie den RCD regelmäßig** (mit der Test-Taste).

3. **Verwenden Sie immer eine Sicherung (MCB) für den Pool-Stromkreis!**

   - **Empfohlene Sicherung:** 10A oder 16A (je nach Pumpenleistung).
   - **Typ:** B oder C (für Haushaltsanwendungen).

4. **Erden Sie alle metallischen Teile!**

   - **Pumpengehäuse, Relais-Modul, ESP32-Gehäuse** müssen geerdet sein.
   - Verwenden Sie **gelb-grüne Erdungskabel** (PE).

5. **Verwenden Sie wasserdichte Verbindungen für Outdoor-Anwendungen!**

   - **IP67 oder höher** für Sensoren und Verbindungen im Freien.
   - **Kabelgarnituren (PG-Verschraubungen)** für Kabeldurchführungen.

6. **Schützen Sie die Elektronik vor Feuchtigkeit!**

   - Verwenden Sie ein **wasserdichtes Gehäuse (IP54+)** für den Controller.
   - **Silicon-Dichtmasse** für Kabeldurchführungen.

7. **Verwenden Sie nur isolierte Werkzeuge!**

   - **Isolierte Schraubendreher, Zangen und Multimeter** (für 1000V).

8. **Führen Sie einen Funktionstest durch, BEVOR Sie die Pumpen anschließen!**
   - Testen Sie die **Relays mit einer 12V-Lampe** (statt 230V), um die Verdrahtung zu prüfen.

---

### ❌ **DON'Ts (Was Sie NICHT tun dürfen)**

1. **Arbeiten Sie NIEMALS unter Spannung!**

   - **230V ist tödlich!**

2. **Verwenden Sie KEINE unisolierten Kabel oder Verbindungen!**

   - **Keine blanken Drähte** in der Nähe von 230V!

3. **Verwenden Sie KEINE billigen, unzertifizierten Netzteile!**

   - **Nur CE/UL-zertifizierte Netzteile** verwenden.

4. **Schließen Sie KEINE Pumpen ohne RCD an!**

   - **RCD ist Pflicht für Personenschutz!**

5. **Verwenden Sie KEINE Relais-Module ohne Optokoppler!**

   - **Optokoppler trennen die Niedervolt- (ESP32) von der Hochvolt-Seite (230V)**.

6. **Platzieren Sie den Controller NICHT in der Nähe von Wasser!**

   - **Mindestens 3,5 Meter Abstand** zur Poolkante (gemäß DIN VDE 0100-702).

7. **Verwenden Sie KEINE verlöteten Verbindungen für 230V!**

   - **Nur Schraubklemmen oder Crimp-Verbindungen** für 230V-Kabel.

8. **Lassen Sie das System NIEMALS unbeaufsichtigt im Testbetrieb!**
   - **Testen Sie immer in Sichtweite** und mit **Notfall-Abschaltung** (z. B. Stecker in Reichweite).

---

## 🛡️ **Sicherheitsausrüstung (Pflicht!)**

| **Ausstattung**                | **Zweck**                                     | **Empfehlung**                                                    |
| ------------------------------ | --------------------------------------------- | ----------------------------------------------------------------- |
| **RCD (FI-Schalter) 30mA**     | Schutz vor Stromschlag bei Isolationsfehlern. | **Pflicht!** (z. B. von Hager, ABB, oder Siemens).                |
| **MCB (Sicherung) 10A/16A**    | Schutz vor Überlastung und Kurzschluss.       | **Pflicht!** (Typ B oder C).                                      |
| **Isolierter Spannungsprüfer** | Prüfen, ob Leitungen spannungsfrei sind.      | **Pflicht!** (z. B. von Fluke, Benning, oder Beha).               |
| **Isolierte Werkzeuge**        | Schutz vor Stromschlag beim Arbeiten an 230V. | **Empfohlen** (z. B. von Wera, Knipex, oder Wiha).                |
| **Multimeter**                 | Messung von Spannung, Strom, Widerstand.      | **Empfohlen** (z. B. von Fluke, Brymen, oder UNI-T).              |
| **IP67-Gehäuse**               | Schutz vor Feuchtigkeit und Staub.            | **Empfohlen** (z. B. von Hammond, Bopla, oder OKW).               |
| **Kabelgarnituren (PG)**       | Wasserdichte Kabeldurchführungen.             | **Empfohlen** (z. B. PG7 für dünne Kabel, PG9 für dickere Kabel). |
| **Erdungskabel (gelb-grün)**   | Erdung von metallischen Teilen.               | **Pflicht!** (mind. 1,5mm² Querschnitt).                          |

---

## 🔌 **Verdrahtung & Anschlüsse**

### **📌 230V-Verdrahtung (NUR FÜR FACHLEUTE!)**

> **⚠️ ACHTUNG: FALSCHE VERDRAHTUNG KANN ZU BRAND ODER TOD FÜHREN!** > **Falls Sie sich unsicher sind, konsultieren Sie einen Elektriker!**

#### **🔹 Grundlegende Verdrahtung für Pumpen**

```text
  ── 230V VERSORGUNG ────────────────────────────────────────────────

  [Stromnetz]
   │
   ├── L (Phase) ──── RCD ──── MCB ────┬── Relay COM1 ── Pool-Pumpe
   │                                    │       NO1 ─────┘
   │                                    │
   │                                    └── Relay COM2 ── Solar-Pumpe
   │                                            NO2 ─────┘
   │
   ├── N (Nullleiter) ────────────────── Neutralleiste ──┬── Pool-Pumpe
   │                                                    └── Solar-Pumpe
   │
   └── PE (Erde) ─────────────────────── Erdungsleiste ──┬── Pool-Pumpe (Metallgehäuse)
                                                           └── Controller-Gehäuse
```

#### **🔹 Erläuterung der Anschlüsse**

| **Anschluss**            | **Beschreibung**                                                        | **Farbcodierung (EU)** |
| ------------------------ | ----------------------------------------------------------------------- | ---------------------- |
| **L (Phase)**            | **Achtung: 230V!** – Führt Strom.                                       | **Braun**              |
| **N (Nullleiter)**       | Rückleiter (keine Spannung im Normalbetrieb, aber **nicht berühren!**). | **Blau**               |
| **PE (Erde)**            | Schutzleiter (Erdung).                                                  | **Gelb-Grün**          |
| **COM (Common)**         | Gemeinsamer Anschluss des Relays (verbinden mit **L**).                 | –                      |
| **NO (Normally Open)**   | Schließer-Kontakt (verbinden mit **Pumpe**).                            | –                      |
| **NC (Normally Closed)** | Öffner-Kontakt (nicht verwendet).                                       | –                      |

#### **🔹 Wichtige Hinweise zur 230V-Verdrahtung**

1. **Relay-Kontakte:**

   - **COM** → **Phase (L)** der Pumpe.
   - **NO** → **Pumpe** (andere Seite der Pumpe → **Nullleiter (N)**).
   - **NC** → **Nicht verwenden!**

2. **Pumpenanschluss:**

   - **Phase (L)** → **Relay COM**.
   - **Nullleiter (N)** → **Direkt zur Pumpe** (ohne Relay!).
   - **Erde (PE)** → **Pumpengehäuse** (falls metallisch).

3. **Kabelquerschnitt:**

   - **Mindestens 1,5mm²** für Pumpen bis 16A.
   - **2,5mm²** für Pumpen mit höherer Leistung.

4. **Kabeltyp:**
   - **NYM-J 3x1,5mm²** für feste Installation.
   - **H07RN-F 3G1,5mm²** für flexible Anwendung (z. B. für Pool-Pumpen).

---

### **📌 Niedervolt-Verdrahtung (ESP32 & Sensoren)**

#### **🔹 Stromversorgung**

```text
  ── STROMVERSORGUNG ─────────────────────────────────────────────────

  [USB-Netzteil 5V/2A+]       [ESP32 Board]
   ┌──────────┐              ┌─────────────┐
   │ 5V (+) ──┼──────────────┤ VIN         │
   │          │              │             │
   │ GND ─────┼──────────────┤ GND         │
   └──────────┘              └────────┘

  [ESP32 Board]               [Relay-Modul]
   ┌─────────────┐              ┌────────────┐
   │ 5V ─────────┼──────────────┤ VCC        │
   │             │              │            │
   │ GND ────────┼──────────────┤ GND        │
   └─────────────┘              └────────────┘
```

#### **🔹 Sensoren (DS18B20)**

```text
  [ESP32 Board]               [DS18B20 Sensor]
   ┌─────────────┐              ┌───────────┐
   │ 3.3V ───────┼──────────────┤ VDD (rot)  │
   │             │              │           │
   │ GND ────────┼──────────────┤ GND (schwarz)│
   │             │              │           │
   │ GPIO32/33 ──┼──[4.7kΩ]──────┤ DATA (gelb)│
   └─────────────┘              └───────────┘
```

#### **🔹 Wichtige Hinweise zur Niedervolt-Verdrahtung**

1. **DS18B20:**

   - **VDD** → **3.3V** (nicht 5V!).
   - **GND** → **GND** (gemeinsam mit ESP32).
   - **DATA** → **GPIO32/33** + **4.7kΩ Pull-Up zu 3.3V**.

2. **Relay-Modul:**

   - **VCC** → **5V** (vom ESP32 VIN oder externem Netzteil).
   - **GND** → **GND** (gemeinsam mit ESP32).
   - **IN1/IN2** → **GPIO25/26** (Aktiv-High!).

3. **Stromversorgung:**
   - **USB-Netzteil:** Mindestens **5V/1A** (für ESP32 + 1 Relay).
   - **Für 2 Relays:** Mindestens **5V/2A** (z. B. USB-Netzteil eines Tablets).

---

## 🔥 **Brandschutz & Überhitzung**

### **🔹 Überhitzung vermeiden**

1. **Relay-Modul:**

   - **Relays können heiß werden** (besonders bei hoher Last).
   - **Lösung:**
     - Verwenden Sie ein **Relay-Modul mit Optokoppler und Kühlkörper**.
     - **Belüftung** im Gehäuse sicherstellen.

2. **ESP32:**

   - **ESP32 wird normalerweise nicht heiß**, aber bei hoher Last (z. B. viele Sensoren) kann er warm werden.
   - **Lösung:**
     - **Gehäuse mit Lüftungsschlitzen** verwenden.
     - **Nicht in der direkten Sonne** platzieren.

3. **Netzteil:**
   - **Billige Netzteile können überhitzen**.
   - **Lösung:**
     - Verwenden Sie ein **qualitativ hochwertiges Netzteil** (z. B. von Mean Well).
     - **Mindestens 5V/2A** für ESP32 + 2 Relays.

### **🔹 Brandschutzmaßnahmen**

1. **Feuerfeste Unterlage:**

   - Platzieren Sie den Controller auf einer **nicht brennbaren Unterlage** (z. B. Metall, Keramik).

2. **Rauchmelder:**

   - Installieren Sie einen **Rauchmelder** in der Nähe des Controllers.

3. **Notfall-Abschaltung:**
   - **Stecker in Reichweite** halten, um den Controller im Notfall schnell vom Strom zu trennen.

---

## ⚡ **Blitzschutz & Überspannungsschutz**

### **🔹 Überspannungsschutz für 230V**

1. **Überspannungsableiter (SPD):**

   - **Empfohlen für Outdoor-Anlagen** (z. B. Pool-Pumpen im Freien).
   - **Typ 2 SPD** (für Haushaltsanwendungen).
   - **Anschluss:** Zwischen **L/N und PE** (vor dem RCD).

2. **Netzfilter:**
   - **Reduziert Störungen** durch andere Geräte (z. B. Motorsteuerungen).
   - **Empfohlen für empfindliche Elektronik** (z. B. ESP32).

---

## 🌧️ **Outdoor-Einsatz & Witterungsschutz**

### **🔹 Schutz vor Feuchtigkeit**

1. **Gehäuse:**

   - **Mindestens IP54** (staubgeschützt, spritzwassergeschützt).
   - **Empfohlen: IP65 oder höher** für Outdoor-Einsatz.
   - **Beispiele:**
     - [Hammond 1591XXSS](https://www.hammfg.com/) (IP65, Kunststoff).
     - [Bopla 7130](https://www.bopla.de/) (IP65, Polycarbonat).

2. **Kabeldurchführungen:**

   - **Kabelgarnituren (PG-Verschraubungen)** verwenden.
   - **IP68 für Unterwasser-Anwendungen** (z. B. für Sensoren im Pool).

3. **Dichtungen:**
   - **Silicon-Dichtmasse** für Kabeldurchführungen.
   - **Dichtungsringe** für Gehäuseverschlüsse.

### **🔹 Schutz vor Hitze & Kälte**

1. **Temperaturbereich:**

   - **ESP32:** -40°C bis +85°C (aber **nicht für direkte Sonneneinstrahlung** geeignet).
   - **Relay-Modul:** -20°C bis +70°C.
   - **DS18B20:** -55°C bis +125°C.

2. **Kühlung:**

   - **Belüftung** im Gehäuse sicherstellen.
   - **Keine direkte Sonneneinstrahlung** (z. B. durch Schatten oder Abdeckung).

3. **Heizung (für kalte Umgebungen):**
   - **Nicht erforderlich** für normale Anwendungen (Pool-Temperatur > 0°C).
   - **Für Extrembedingungen:** Verwenden Sie ein **beheiztes Gehäuse**.

---

## 🏊 **Sicherheit am Pool**

### **🔹 Allgemeine Pool-Sicherheit**

1. **Elektrische Geräte am Pool:**

   - **Mindestens 3,5m Abstand** von der Poolkante (gemäß DIN VDE 0100-702).
   - **Ausnahme:** Geräte mit **IPX8-Schutz** (z. B. Unterwasser-Pumpen).

2. **Pumpen & Filter:**

   - **Immer mit RCD (30mA) absichern!**
   - **Regelmäßig auf Undichtigkeiten prüfen!**

3. **Kabel & Stecker:**
   - **Keine Steckdosen in Reichweite des Pools!**
   - **Verwenden Sie fest installierte Kabel** (keine Verlängerungskabel!).

### **🔹 Sensoren im Pool**

1. **DS18B20-Sensoren:**

   - **Wasserdicht (IP68)** für Pool-Anwendungen.
   - **Kabel mit PG-Verschraubung** ins Gehäuse führen.

2. **Kabelverlegung:**
   - **Keine Kabel im Pool verlegen!**
   - **Kabel in Rohren oder Kabelkanälen** verlegen.

---

## 📜 **Rechtliche Hinweise & Haftungsausschluss**

### **🔹 Wichtig: Dieses Projekt ist NUR für den Eigenbau!**

- **Keine CE-Kennzeichnung:** Dieses Projekt ist **nicht für den Verkauf oder die kommerzielle Nutzung** bestimmt.
- **Keine Gewährleistung:** Der Autor übernimmt **keinerlei Haftung** für Schäden, Verletzungen oder Tod durch falsche Handhabung.
- **Eigenverantwortung:** Der Aufbau und Betrieb dieses Projekts erfolgt **auf eigene Gefahr**.

### **🔹 Empfehlung für Unsichere**

Falls Sie sich **unsicher sind** oder **keine Erfahrung mit 230V-Elektronik haben**, empfehlen wir:

1. **Konsultieren Sie einen Elektriker!**

   - Ein **Fachmann** kann die Verdrahtung prüfen und sicherstellen, dass alles den **VDE-Bestimmungen** entspricht.

2. **Verwenden Sie fertige Lösungen:**

   - Es gibt **kommerzielle Pool-Steuerungen** (z. B. von **Hayward, Pentair,
     oder AstralPool**), die **CE-zertifiziert** sind.

3. **Bilden Sie sich weiter:**
   - **Kurse für Elektroinstallation** (z. B. bei der **Handwerkskammer**).
   - **Bücher über Elektrotechnik** (z. B. "Elektroinstallation für Dummies").

---

## 📞 **Notfallmaßnahmen**

### **🔹 Bei Stromschlag:**

1. **Strom abschalten!**

   - **FI-Schalter (RCD) auslösen** oder **Sicherung (MCB) herausdrehen**.

2. **Erste Hilfe leisten:**
   - **Nicht den Verletzten anfassen!** (könnte selbst unter Spannung stehen).
   - **Notruf absetzen:** **112 (Europa)** oder **lokale Notrufnummer**.
   - **Erste-Hilfe-Maßnahmen** (falls kein Strom mehr anliegt).

### **🔹 Bei Brand:**

1. **Strom abschalten!**

   - **FI-Schalter (RCD) auslösen** oder **Hauptsicherung abschalten**.

2. **Löschen:**

   - **NICHT mit Wasser löschen!** (230V-Gefahr!).
   - **Verwenden Sie einen CO₂- oder Pulverlöscher** (für Elektronikbrände).

3. **Notruf absetzen:**
   - **112 (Europa)** oder **lokale Notrufnummer**.

---

## 📚 **Weiterführende Links & Normen**

### **🔹 Normen & Richtlinien**

| **Norm**             | **Beschreibung**                                         | **Link**                                          |
| -------------------- | -------------------------------------------------------- | ------------------------------------------------- |
| **DIN VDE 0100-702** | Elektrische Anlagen in Schwimmbädern und anderen Becken. | [DIN VDE 0100-702](https://www.vde.com/de/normen) |
| **DIN VDE 0100-410** | Schutz gegen elektrischen Schlag.                        | [DIN VDE 0100-410](https://www.vde.com/de/normen) |
| **DIN VDE 0100-530** | Auswahl und Errichtung elektrischer Betriebsmittel.      | [DIN VDE 0100-530](https://www.vde.com/de/normen) |
| **DIN EN 60335-1**   | Sicherheit elektrischer Geräte für den Hausgebrauch.     | [DIN EN 60335-1](https://www.din.de/)             |

### **🔹 Nützliche Ressourcen**

- [Elektroinstallation für Laien (VDE)](https://www.vde.com/de/verbraucher)
- [Sicherheit im Haushalt (DGS)](https://www.dgs.de/)
- [Erste Hilfe bei Stromunfällen (DRK)](https://www.drk.de/)

---

## ✅ **Checkliste vor der Inbetriebnahme**

| **Prüfpunkt**                                | **Erledigt?** | **Hinweise**                                      |
| -------------------------------------------- | ------------- | ------------------------------------------------- |
| **RCD (FI-Schalter) 30mA installiert**       | ☐             | **Pflicht für Personenschutz!**                   |
| **MCB (Sicherung) installiert**              | ☐             | **10A oder 16A, Typ B oder C.**                   |
| **Alle 230V-Verbindungen geprüft**           | ☐             | **Keine blanken Drähte, nur Schraubklemmen.**     |
| **Erdung aller metallischen Teile**          | ☐             | **Gelb-grüne Kabel verwenden.**                   |
| **IP67-Gehäuse für Outdoor-Einsatz**         | ☐             | **Nur bei Einsatz im Freien.**                    |
| **Kabelgarnituren für Kabeldurchführungen**  | ☐             | **PG7/PG9 für wasserdichte Durchführungen.**      |
| **Isolierter Spannungsprüfer getestet**      | ☐             | **Prüfen, ob alle Leitungen spannungsfrei sind.** |
| **Funktionstest mit 12V-Lampe durchgeführt** | ☐             | **Vor dem Anschließen der Pumpen!**               |
| **RCD-Test durchzuführen**                   | ☐             | **Test-Taste am RCD drücken.**                    |
| **Notfall-Abschaltung in Reichweite**        | ☐             | **Stecker oder Sicherung schnell erreichbar.**    |

---

**🚨 WICHTIG: NUR FORTFAHREN, WENN ALLE PUNKTE ABGEHAKT SIND!**

---

## 🎯 **Zusammenfassung**

- **230V ist tödlich!** → **Immer RCD + MCB verwenden!**
- **Nur an spannungsfreien Leitungen arbeiten!**
- **Immer Erdung (PE) für metallische Teile!**
- **Outdoor-Einsatz nur mit IP67+!**
- **Bei Unsicherheit: Elektriker konsultieren!**

**Viel Erfolg und bleiben Sie sicher!** 🛡️

_Falls Sie Fragen haben, zögern Sie nicht, in den [GitHub Discussions](https://github.com/smart-swimmingpool/pool-controller/discussions) nachzufragen._
