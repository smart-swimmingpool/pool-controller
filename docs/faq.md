---
title: "❓ FAQ: Häufige Probleme & Lösungen"
summary: "Antworten auf häufig gestellte Fragen und Lösungen für typische Probleme mit dem Smart Swimmingpool Controller"
date: "2026-06-14"
lastmod: "2026-06-14"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "faq", "troubleshooting"]
menu:
  docs:
    parent: Pool Controller
    name: FAQ
    weight: 50
---

## 🔍 Häufige Probleme & Lösungen

Hier findest du **Antworten auf häufig gestellte Fragen** und **Lösungen für typische Probleme** mit dem **Smart Swimmingpool Controller**. 
Falls dein Problem hier nicht aufgelistet ist, erstelle bitte ein [Issue auf GitHub](https://github.com/smart-swimmingpool/pool-controller/issues) oder frage in den [Discussions](https://github.com/smart-swimmingpool/pool-controller/discussions).

---

## 📡 **WiFi & Netzwerk**

### ❌ **Controller verbindet sich nicht mit WiFi**

#### **Symptome:**
- Status-LED blinkt **langsam (1x pro Sekunde)** oder **bleibt im AP-Modus (schnelles Blinken)**.
- Web-Interface nicht über lokale IP erreichbar.

#### **Lösungen:**
1. **Starte im AP-Modus neu:**
   - Der Controller startet automatisch im AP-Modus, wenn keine WiFi-Zugangsdaten
     gespeichert sind oder die Verbindung fehlschlägt.
   - Falls du ihn manuell in den AP-Modus zwingen möchtest, setze die
     WiFi-Konfiguration zurück: Web-UI → System → Factory Reset (sofern erreichbar)
     oder flashe die Firmware neu über USB.
   - Verbinde dich mit **`Pool-Controller-Setup`** (offenes Netzwerk).
   - Öffne `http://192.168.4.1` und prüfe die **WiFi-Einstellungen**.

2. **Prüfe die WiFi-Signalstärke:**
   - Gehe zum **Dashboard**-Tab im Web-Interface.
   - Prüfe den **RSSI-Wert** (dB).
     - **> -70 dB:** Gute Verbindung.
     - **-70 bis -80 dB:** Akzeptabel, aber möglicherweise instabil.
     - **< -80 dB:** Schlechte Verbindung – Controller näher zum Router platzieren.

3. **Manuelle IP-Adresse zuweisen (falls DHCP fehlschlägt):**
   - Gehe zu **WiFi Setup** → **Static IP**.
   - Trage eine **freie IP-Adresse** in deinem Netzwerk ein (z. B. `192.168.1.200`).

---

### ❌ **Controller verliert WiFi-Verbindung**

#### **Symptome:**
- Controller funktioniert einige Zeit und **verliert dann die Verbindung**.

#### **Lösungen:**
1. **Controller näher zum Router platzieren** oder **WiFi-Repeater** verwenden.
2. **Konfiguriere statische IP** für den Controller im Router.
3. **Verwende ein stabileres Netzteil (5V/2A+)**.

---

## 🌐 **MQTT & Smart Home Integration**

### ❌ **MQTT verbindet sich nicht mit dem Broker**

#### **Symptome:**
- Status-LED leuchtet **dauerhaft**, aber MQTT-Geräte werden **nicht in Home Assistant/openHAB angezeigt**.

#### **Lösungen:**
1. **Prüfe die MQTT-Einstellungen:**
   - **MQTT Host:** `IP deines MQTT-Brokers` (z. B. `192.168.1.50`).
   - **MQTT Port:** `1883`.
   - **MQTT Username/Password:** Falls aktiviert.

2. **Teste MQTT manuell:**
   ```bash
   mosquitto_pub -h 192.168.1.50 -t "test" -m "hello" -u mqtt_user -P secret
   ```

3. **Prüfe MQTT-Discovery in Home Assistant:**
   - Aktiviere **MQTT Discovery** in Home Assistant.
   - Lösche alte Homie-Nachrichten vom Broker:
     ```bash
     mosquitto_pub -h 192.168.1.50 -t "homie" -n -r
     ```

---

### ❌ **Home Assistant erkennt den Controller nicht**

#### **Symptome:**
- Controller ist mit MQTT verbunden, aber **keine Geräte werden in Home Assistant angezeigt**.

#### **Lösungen:**
1. **Aktualisiere auf Firmware v3.3.0+** (Homie wird nicht mehr unterstützt).
2. **Lösche alte Homie-Nachrichten** vom Broker (siehe oben).
3. **Aktiviere Discovery in Home Assistant:**
   - Gehe zu **Einstellungen → Geräte & Dienste → MQTT**.
   - Aktiviere **Discovery**.

---

## 🌡️ **Sensoren (DS18B20)**

### ❌ **Sensor zeigt `-127°C` an**

#### **Symptome:**
- Sensor wird im **Web-Interface** oder **MQTT** mit `-127°C` angezeigt.

#### **Lösungen:**
1. **Füge einen 4.7kΩ-Widerstand zwischen DATA und 3.3V hinzu** (Pull-Up).
2. **Prüfe die GPIO-Pins** in `Config.hpp` (Standard: GPIO32/33).
3. **Teste den Sensor mit einem einfachen Arduino-Sketch.**

---

### ❌ **Sensor zeigt `85°C` oder andere unplausible Werte an**

#### **Symptome:**
- Sensor zeigt **konstant 85°C** oder **sehr hohe Werte** an.

#### **Lösungen:**
1. **Prüfe auf Kurzschlüsse** zwischen DATA und VDD.
2. **Teste mit einem anderen Sensor.**
3. **Verwende kürzere Kabel** (max. 30m mit Pull-Up).

---

### ❌ **Sensor wird nicht erkannt**

#### **Symptome:**
- Im **Web-Interface** oder **Seriellen Logs** wird **"Sensor not found"** angezeigt.

#### **Lösungen:**
1. **Prüfe die Serielle Ausgabe** (115200 Baud) auf Fehlermeldungen.
2. **Prüfe die Verkabelung:**
   - **VDD (rot)** → **3.3V**.
   - **GND (schwarz)** → **GND**.
   - **DATA (gelb/weiß)** → **GPIO32/33** + **4.7kΩ zu 3.3V**.

---

## 🔌 **Relay-Modul & Pumpen**

### ❌ **Relay klickt nicht**

#### **Symptome:**
- Relay schaltet **nicht** (kein Klick-Geräusch) beim Aktivieren über das Web-Interface.

#### **Lösungen:**
1. **Prüfe die Logik des Relay-Moduls:**
   - **Aktiv-High:** GPIO **HIGH (3.3V)** → Relay **EIN**.
   - **Aktiv-Low:** GPIO **LOW (0V)** → Relay **EIN**.
   - Teste mit einem einfachen Arduino-Sketch.

2. **Prüfe die Stromversorgung:**
   - **VCC** → **5V** (vom ESP32 VIN oder externem Netzteil).
   - **GND** → **GND** (gemeinsam mit ESP32).

---

### ❌ **Relay klickt, aber Pumpe läuft nicht**

#### **Symptome:**
- Relay **klickt**, aber die **Pumpe startet nicht**.

#### **Lösungen:**
1. **Prüfe die 230V-Verdrahtung:**
   - **ACHTUNG: Nur für Fachleute!**
   - **COM** → **Phase (L)** der Pumpe.
   - **NO** → **Pumpe**.
   - **Nullleiter (N)** → **Direkt zur Pumpe** (ohne Relay!).

2. **Teste die Pumpe direkt** (über eine Steckdose).

---

### ❌ **ESP32 startet neu beim Relay-Schalten**

#### **Symptome:**
- Der **ESP32 startet neu**, wenn das Relay schaltet.

#### **Lösungen:**
1. **Verwende ein stärkeres Netzteil (5V/2A+)**.
2. **Füge einen 100µF-Kondensator** zwischen **VIN und GND** hinzu.

---

## 🔄 **Firmware & Updates**

### ❌ **Firmware-Update schlägt fehl (OTA)**

#### **Symptome:**
- OTA-Update **bricht ab** oder **startet nicht**.

#### **Lösungen:**
1. **Lade die richtige `.bin`-Datei** aus den [Releases](https://github.com/smart-swimmingpool/pool-controller/releases) herunter.
2. **Führe das Update manuell durch:**
   ```bash
   esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x10000 pool-controller-v3.3.0.bin
   ```
3. **Prüfe das OTA-Passwort** im Web-Interface (**Security & Update**).

---

## 🔋 **Stromversorgung**

### ❌ **ESP32 startet nicht oder startet neu**

#### **Symptome:**
- ESP32 **startet nicht** oder **startet neu** (Brownout).

#### **Lösungen:**
1. **Verwende ein stabileres Netzteil (5V/2A+)**.
2. **Füge einen 100µF-Kondensator** zwischen **VIN und GND** hinzu.
3. **Prüfe die Spannung** (sollte stabil bei 5V liegen).

---

## 📞 **Hilfe & Support**

Falls dein Problem hier nicht aufgelistet ist:
1. **GitHub Discussions:** [smart-swimmingpool/pool-controller/discussions](https://github.com/smart-swimmingpool/pool-controller/discussions)
2. **Issue erstellen:** [smart-swimmingpool/pool-controller/issues](https://github.com/smart-swimmingpool/pool-controller/issues)
   - **Bitte gib folgende Informationen an:**
     - Firmware-Version (z. B. v3.3.0).
     - Hardware (ESP32-Modell, Relay-Modul, Sensoren).
     - Fehlerbeschreibung (Was passiert? Was hast du bereits ausprobiert?).
     - Serielle Logs (falls verfügbar).

---

**Viel Erfolg bei der Fehlerbehebung!** 🛠️

*Falls du eine Lösung für ein Problem findest, das hier nicht aufgelistet ist, erstelle bitte einen **Pull Request**, um diese FAQ zu erweitern!*