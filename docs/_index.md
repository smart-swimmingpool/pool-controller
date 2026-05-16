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

## 🏊 The Homie 3.0 compatible Smart Swimmingpool Controller 🎛️

- [GitHub Sources](https://github.com/smart-swimmingpool/pool-controller)
- [Watch on GitHub](https://github.com/smart-swimmingpool/pool-controller/subscription)
- [Star on GitHub](https://github.com/smart-swimmingpool/pool-controller)

Manage your swimming pool in a smart way to enjoy it comfortably and affordably (for less than 100€).

## Main Features

- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] [Homie 3.0](https://homieiot.github.io/) compatible MQTT messaging
- [x] Independent of specific smarthome servers
  - [x] [openHAB](https://www.openhab.org) since Version 2.4 using MQTT Homie
  - [x] [Home Assistant](https://home-assistant.io) using MQTT Homie
- [x] Time sync via NTP (europe.pool.ntp.org)
- [x] Logging-Information via Homie-Node

## OTA & Firmware Updates

- [OTA Updates Guide](./ota-updates/) for web-based update flow and initial setup
- GitHub releases: <https://github.com/smart-swimmingpool/pool-controller/releases>

## Planned Features

- [ ] Configurable NTP Server (currently hardcoded: europe.pool.ntp.org)
- [ ] Be more smart: self learning for improved pool pump timed circulation for cleaning and heating
- [ ] Two separate circulation cycles
- [ ] Store configuration changes persistently on the controller
- [ ] Temperature based cleaning circulation time (colder == shorter, hotter == longer)
- [ ] Improved sketch to work completely without WiFi connection
- [ ] Homie should run without WiFi connection
- [ ] Enhance sketch using display and buttons to setup environment.
- [ ] Use only one power supply for ESP8266 (5V) and relais (230V)
- see also the [issue list](https://github.com/smart-swimmingpool/pool-controller/issues)

[![works with MQTT Homie](https://homieiot.github.io/img/works-with-homie.svg)](https://homieiot.github.io/)

{{< figure library="true" src="pool-controller_breadboard.png" title="Breadboard Circuit" lightbox="true" >}}
