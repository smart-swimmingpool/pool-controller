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
{{% /alert %}}

## Parts List (BOM)

- 1 * ESP8266 NodeMCU Controller ([Amazon](https://amzn.to/2Ze9DSh))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2ZlfZ2c))
- 1 * Relais-Module 5V ([Amazon](https://amzn.to/31RBd5s))
- 1 * Breadboard and wires to connect (alternatively soldering of the circuit)

## Circuit

The circuit of the controller can be found in the following image based on a
breadboard wiring:

{{< figure library="true" src="../pool-controller_breadboard.png" title="Pool Controller Circuit" lightbox="true" >}}

The source [Fritzing](https://fritzing.org/) file can be found in the GitHub
project:
[pool-controller.fzz](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)

### ESP8266 PIN Usage

The ESP8266 is connected using following PINs. You can find the constant values within the sources
of `src/Config.hpp` (first column of table) which are associated to the pins.

| Constant in Source | PIN of ESP8266 | Description                                           |
| ------------------ | :------------: | ----------------------------------------------------- |
| PIN_DS_SOLAR       |       D5       | Pin of temperature sensor (DS18B20) for solar storage |
| PIN_DS_POOL        |       D6       | Pin of temperature sensor (DS18B20) for pool water    |
| PIN_RELAY_POOL     |       D1       | Pin to connect relais for pool pump                   |
| PIN_RELAY_SOLAR    |       D2       | Pin to connect relais for solar pump                  |

{{% alert note %}}
TODO: improve PIN usage (see
[ESP8266 GPIO Reference](https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/))
{{% /alert %}}

### ESP32 PIN Usage (current default)

For ESP32 defaults, constants are defined in `src/Config.hpp`.

| Constant in Source | PIN of ESP32 | Description                                           |
| ------------------ | :----------: | ----------------------------------------------------- |
| PIN_DS_SOLAR       |    GPIO32    | Pin of temperature sensor (DS18B20) for solar storage |
| PIN_DS_POOL        |    GPIO33    | Pin of temperature sensor (DS18B20) for pool water    |
| PIN_RELAY_POOL     |    GPIO25    | Pin to connect relais for pool pump                   |
| PIN_RELAY_SOLAR    |    GPIO26    | Pin to connect relais for solar pump                  |

For decision background and wiring visualization, see
`docs/esp32-schematic-optimization-de.md` (German / DE).

### ESP32 Variant Compatibility

The defaults above are valid for ESP32 variants that provide GPIO32, GPIO33,
GPIO25 and GPIO26 (for example classic ESP32 modules).

Some variants (for example ESP32-C3) do not provide these pins. In this case:

- update the pin constants in `src/Config.hpp`
- avoid boot/strapping-sensitive pins for OneWire and relays
- verify relay behavior during boot/reset after rewiring

## Power Supply

In my environment, I use USB to power the ESP8266 via a small USB power
adapter and an additional 230V power plug as the source for the power of the
pumps, which are switched via the relays.
