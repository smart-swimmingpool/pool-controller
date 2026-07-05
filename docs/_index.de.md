---
linktitle: Pool Controller
title: Pool Controller 3.3
summary: ESP32-basierte MQTT-Steuerung für den Swimmingpool mit Home Assistant Discovery, Solarheizungsregelung, temperaturabhängiger Filterlaufzeit und Pumpen-Timer — Eigenbau für unter 100€
date: "2020-05-28"
lastmod: "2026-06-08"
draft: false
toc: true
type: docs
featured: true
tags: ["docs", "controller", "tutorial"]

menu:
  docs:
    parent: Pool Controller
    name: Überblick
    weight: 10
---

## 🏊 Der MQTT-fähige Smart Swimmingpool Controller 🎛️

<span style="text-shadow: none;">
<!-- markdownlint-disable MD013 -->
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller/subscription" data-size="large" data-show-count="true" aria-label="Watch smart-swimmingpool/pool-controller on GitHub">Watch</a>
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller" data-icon="octicon-star" data-size="large" data-show-count="true" aria-label="Star this on GitHub">Star</a><script async defer src="https://buttons.github.io/buttons.js"></script>
<!-- markdownlint-enable MD013 -->

[GitHub Quellcode](https://github.com/smart-swimmingpool/pool-controller)
</span>

Steuer deinen Swimming-Pool auf smarte Art und Weise, um diesen bequem und günstig (weniger als 100€) zu genießen.

## Haupteigenschaften

- [x] Verwaltung der zeitgesteuerten Wasserzirkulation für die Reinigung
- [x] Steuerung der Wassererwärmung durch eine zusätzliche Pumpe für den Sonnenkreislauf
- [x] [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) - Native HA-Integration
- [x] Unabhängig von einzelnen Smarthome-Servern
  - [x] [Home Assistant](https://home-assistant.io) via nativer MQTT Discovery
  - [x] [openHAB](https://www.openhab.org) via MQTT (manuelle Konfiguration erforderlich)
- [x] Automatische Zeitsynchronisierung mit NTP (europe.pool.ntp.org)
- [x] System-Logging und Diagnose-Ereignisse

## Roadmap

✔️ Zustandsspeicherung über Neustarts hinweg (v3.1.0+)
✔️ Web-basierte OTA-Firmware-Updates (v3.2.0+)
✔️ Home Assistant MQTT Discovery (v3.3.0)
✔️ Automatische Update-Prüfung via GitHub Releases (v3.3.0)
✔️ Temperaturabhängige Filterlaufzeit (v3.4.0)

- [ ] Konfigurierbarer NTP-Server
- [ ] Zwei getrennte Zirkulationszyklen (Pool + Solar unabhängig)
- [ ] Offline-Betriebsmodus (ohne WiFi)
- [ ] Einheitliche Stromversorgung (ESP32 + Relais aus einer Quelle)
- [ ] Rollback-Funktion für OTA-Updates
- [ ] A/B-Partition-Updates für sicherere Firmware-Upgrades
- siehe auch: [Issue-Liste](https://github.com/smart-swimmingpool/pool-controller/issues)

<!-- markdownlint-disable-next-line MD013 -->

{{< figure library="true" src="pool-controller_breadboard.png" title="ESP32 Pool Controller Breadboard-Schaltung mit DS18B20-Temperatursensoren und Relaismodulen" lightbox="true" >}}
