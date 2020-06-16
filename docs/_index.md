---
linktitle: Pool Controller
summary: Control your Smart Swimming Pool smart
weight: 2

# page metadata.
title: Pool Controller 2.0
summary: Control your Smart Swimming Pool smart
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
    name: Overview
    weight: 10
---

**🏊 The Homie 3.0 compatible Smart Swimmingpool Controller 🎛️**

<span style="text-shadow: none;">
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller/subscription" data-size="large" data-show-count="true" aria-label="Watch smart-swimmingpool/pool-controller on GitHub">Watch</a>
<a class="github-button" href="https://github.com/smart-swimmingpool/pool-controller" data-icon="octicon-star" data-size="large" data-show-count="true" aria-label="Star this on GitHub">Star</a><script async defer src="https://buttons.github.io/buttons.js"></script>

[GitHub Sources](https://github.com/smart-swimmingpool/pool-controller)
</span>

Manage your swmming pool on the smart way to enjoy it in confortable and cheap (less than 100€) way.

## Main Features

- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] [Homie 3.0](https://homieiot.github.io/) compatible MQTT messaging
- [x] Independent of specific smarthome servers
  - [x] [openHAB](https://www.openhab.org) since Version 2.4 using MQTT Homie
  - [x] [Home Assistant](home-assistant.io) using MQTT Homie
- [x] Timesync via NTP (europe.pool.ntp.org)
- [x] Logging-Information via Homie-Node

## Planned Features

- [ ] Configurable NTP Server (currently hardcoded: europe.pool.ntp.org)
- [ ] Be more smart: self learning for improved pool pump timed circulation for cleaning and heating
- [ ] Two separate circulation cycles
- [ ] Store configuration changes persistent on conroller
- [ ] Temperature based cleaning circulation time (colder == shorter, hotter == longer)
- [ ] Improved sketch to work completly without WiFi connection
- [ ] Homie should run without WiFi connection
- [ ] Enhance sketch using display and buttons to setup environment.
- [ ] Use only one power supply for ESP8266 (5V) and relais (230V)
- see also the [issue list](https://github.com/smart-swimmingpool/pool-controller/issues)

[![works with MQTT Homie](https://homieiot.github.io/img/works-with-homie.svg "works with MQTT Homie")](https://homieiot.github.io/)

{{< figure library="true" src="pool-controller_breadboard.png" title="Breadboard Circuit of Pool Controller" lightbox="true" >}}
