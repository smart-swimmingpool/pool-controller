---
title: Hardware Guide of Pool Controller
summary:
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
    name: Hardware Guide
    weight: 20
---

This Hardware Guide describes how to set up the hardware of the controller.

{{% alert note %}}
For an ESP32-focused wiring analysis and optimization (sensors and relays), see
`docs/esp32-schematic-optimization-de.md` (German / DE).

For a complete ESP32 wiring schematic including resistors and optional RTC clock
module, see `docs/esp32-complete-wiring-schematic-de.md` (German / DE).
{{% /alert %}}

## Parts List (BOM)

- 1 * ESP32 Development Board (e.g. ESP32 DevKit V1) ([Amazon](https://amzn.to/2Ze9DSh))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2ZlfZ2c))
- 1 * Relais-Module 5V ([Amazon](https://amzn.to/31RBd5s))
- 1 * Breadboard and wires to connect (alternatively soldering of the circuit)

## Circuit

The circuit of the controller can be found in the following image based on a breadboard wiring:

{{< figure
  library="true"
  src="../pool-controller_breadboard.png"
  title="Breadboard Circuit of Pool Controller"
  lightbox="true" >}}

The source [Fritzing](https://fritzing.org/) file can be found in the GitHub project: [pool-controller.fzz](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)

### ESP32 Pin Usage

The ESP32 is connected using the following pins. The constant values are defined in `src/Config.hpp`.

| Constant | GPIO Pin | Description |
|----------|:--------:|-------------|
| PIN_DS_SOLAR | GPIO15 | Pin of temperature sensor (DS18B20) for solar storage |
| PIN_DS_POOL | GPIO16 | Pin of temperature sensor (DS18B20) for pool water |
| PIN_RELAY_POOL | GPIO18 | Pin to connect relay for pool pump |
| PIN_RELAY_SOLAR | GPIO19 | Pin to connect relay for solar pump |

## Power Supply

In my environment, I use USB to power the ESP32 via a small USB power adapter
and an additional 230V power plug as the source for the power of the pumps,
which are switched via the relays.
