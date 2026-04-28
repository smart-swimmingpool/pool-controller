---
linktitle: Pool Controller
summary: Die smarte Steuerung deines Swimmingpools
weight: 2

# page metadata.
title: Pool Controller 2.0
summary: Steuer deinen Swimmingpool smart
date: "2020-05-28"
lastmod: "2020-06-02"
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

**🏊 Der Homie 3.0 kompaible Smart Swimmingpool Controller 🎛️**

<span style="text-shadow: none;">
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller/subscription" data-size="large" data-show-count="true" aria-label="Watch smart-swimmingpool/pool-controller on GitHub">Watch</a>
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller" data-icon="octicon-star" data-size="large" data-show-count="true" aria-label="Star this on GitHub">Star</a><script async defer src="https://buttons.github.io/buttons.js"></script>

[GitHub Quellcode](https://github.com/smart-swimmingpool/pool-controller)
</span>

Steuer deinen Swimming-Pool auf smarte Art und Weise, um diesen bequem und günstig (weniger als 100€) zu genießen.

## Haupteigenschaften

- [x] Verwaltung der zeitgesteuerten Wasserzirkulation für die Reinigung
- [x] Steuerung der Wassererwärmung durch eine zusätzliche Pumpe für den Sonnenkreislauf
- [x] [Homie 3.0](https://homieiot.github.io/) kompatibles MQTT Nachrichtenformat
- [x] Unabhängig von einzelnen Smarthome-Servern
  - [x] [openHAB](https://www.openhab.org) seit Version 2.4 unter Verwendung von MQTT Homie
  - [x] [Home Assistant](https://home-assistant.io) unter Verwendung von MQTT Homie
- [x] Automatische Zeitsynchroisierung mit NTP (europe.pool.ntp.org)
- [x] Logging-Informationen via Homie-Node

## Geplante Funktionen

- [ ] Konfigurierbarer NTP-Server (aktuell hardcoded: europe.pool.ntp.org)
- [ ] noch smarter: selbstanpassendes für eine verbesserte zeitgesteuerte Zirkulation der Poolpumpe zur
      Reinigung und Erwärmung
- [ ] Zwei getrennte Zirkulationszeiten
- [ ] Konfigurationsänderungen dauerhaft auf dem Controller speichern
- [ ] Temperaturabhängige Zirkulationszeiten (kühleres Wetter == kürzere Pumpenzeit, wäremer == längere Zeit)
- [ ] Verbesserte Schaltung die komplett ohne WiFi-Verbindung funktioniert
- [ ] Verbesserung der Schaltung mit Anzeige und Tasten zur Einrichtung der Konfiguration.
- [ ] Eine Stromversorgung für die ganze Schaltung (ESP8266 und Relais)
- siehe auch: [Issue-Liste](https://github.com/smart-swimmingpool/pool-controller/issues)

[![works with MQTT Homie](https://homieiot.github.io/img/works-with-homie.svg "works with MQTT Homie")](https://homieiot.github.io/)

{{< figure library="true" src="pool-controller_breadboard.png" title="Breadboard Circuid of Pool Controller" lightbox="true" >}}
