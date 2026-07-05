---
linktitle: Pool Controller
title: Pool Controller 3.3
summary: ESP32-based MQTT smart swimming pool controller with Home Assistant Discovery, solar heating regulation, temperature-based circulation, and pump timer — build your own for under 100€
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
    name: Overview
    weight: 10
---

## 🏊 The MQTT-enabled Smart Swimmingpool Controller 🎛️

<span style="text-shadow: none;">
<!-- markdownlint-disable MD013 -->
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller/subscription" data-size="large" data-show-count="true" aria-label="Watch smart-swimmingpool/pool-controller on GitHub">Watch</a>
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller" data-icon="octicon-star" data-size="large" data-show-count="true" aria-label="Star this on GitHub">Star</a><script async defer src="https://buttons.github.io/buttons.js"></script>
<!-- markdownlint-enable MD013 -->

[GitHub Sources](https://github.com/smart-swimmingpool/pool-controller)
</span>

Manage your swimming pool in a smart way to enjoy it comfortably and affordably (for less than 100€).

## Main Features

- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] [Home Assistant MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) - Native HA integration
- [x] Independent of specific smarthome servers
  - [x] [Home Assistant](https://home-assistant.io) via native MQTT Discovery
  - [x] [openHAB](https://www.openhab.org) via MQTT (manual configuration required)
- [x] Timesync via NTP (europe.pool.ntp.org)
- [x] Logging of system events and diagnostics

## Roadmap

✔️ State persistence across reboots (v3.1.0+)
✔️ Web-based OTA firmware updates (v3.2.0+)
✔️ Home Assistant MQTT Discovery (v3.3.0)
✔️ Automatic update checking from GitHub releases (v3.3.0)
✔️ Temperature-based circulation time adjustment (v3.4.0)

- [ ] Configurable NTP server
- [ ] Dual circulation cycles (pool + solar independent)
- [ ] Offline operation mode (no WiFi required)
- [ ] Unified power supply (ESP32 + relays from single source)
- [ ] Rollback capability for OTA firmware updates
- [ ] A/B partition updates for safer firmware upgrades
- see also the [issue list](https://github.com/smart-swimmingpool/pool-controller/issues)

<!-- markdownlint-disable-next-line MD013 -->

{{< figure library="true" src="pool-controller_breadboard.png" title="ESP32 Pool Controller breadboard circuit with DS18B20 temperature sensors and relay modules" lightbox="true" >}}
