---
linktitle: Pool Controller
summary: Die smarte Steuerung deines Swimmingpools
weight: 2

# page metadata.
title: Pool Controller 3.3
summary: Steuer deinen Swimmingpool smart
date: "2020-05-28"
lastmod: "2026-06-07"
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
  - [x] [openHAB](https://www.openhab.org) via MQTT
- [x] Automatische Zeitsynchronisierung mit NTP (europe.pool.ntp.org)
- [x] System-Logging und Diagnose-Ereignisse

## Geplante Funktionen

- [ ] Konfigurierbarer NTP-Server (aktuell hardcoded: europe.pool.ntp.org)
- [ ] noch smarter: selbstanpassendes für eine verbesserte zeitgesteuerte Zirkulation der Poolpumpe zur
      Reinigung und Erwärmung
- [ ] Zwei getrennte Zirkulationszeiten
- [ ] Konfigurationsänderungen dauerhaft auf dem Controller speichern
- [ ] Temperaturabhängige Zirkulationszeiten (kühleres Wetter == kürzere Pumpenzeit, wäremer == längere Zeit)
- [ ] Verbesserte Schaltung die komplett ohne WiFi-Verbindung funktioniert
- [ ] Verbesserung der Schaltung mit Anzeige und Tasten zur Einrichtung der Konfiguration.
- [ ] Eine Stromversorgung für die ganze Schaltung (ESP32 und Relais)
- siehe auch: [Issue-Liste](https://github.com/smart-swimmingpool/pool-controller/issues)

<!-- markdownlint-disable-next-line MD013 -->

{{< figure library="true" src="pool-controller_breadboard.png" title="Breadboard Circuid of Pool Controller" lightbox="true" >}}
