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

This Hardware Guide will describe how to setup the hardware of the controller.

## Parts List (BOM)

- 1 * ESP8266 NodeMCU Controller ([Amazon](https://amzn.to/2Ze9DSh))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2ZlfZ2c))
- 1 * Relais-Module 5V ([Amazon](https://amzn.to/31RBd5s))
- 1 * Breadboard and wires to connect (alternativly soldering of the circuit)

## Circuit

The circuit of the controller could be found on following image based on a breadboard wireing:

{{< figure library="true" src="../pool-controller_breadboard.png" title="Breadboard Circuit of Pool Controller" lightbox="true" >}}

The source [Fritzing](https://fritzing.org/) file could be found in GitHub project: [pool-controller.fzz](https://github.com/smart-swimmingpool/pool-controller/raw/main/docs/pool-controller.fzz)

### ESP8266 PIN Usage

The ESP8266 is connected using following PINs. You can find the constant values within the sources
of `main.cpp` (first column of table) which are associated to the pins.

| Constant in Source | PIN of ESP8266 | Description                                           |
|--------------------|:--------------:|-------------------------------------------------------|
| PIN_DS_SOLAR       | D5             | Pin of temperature sensor (DS18B20) for solar storage |
| PIN_DS_POOL        | D6             | Pin of temperature sensor (DS18B20) for pool water    |
| PIN_RELAY_POOL     | D1             | Pin to connect relais for pool pump                   |
| PIN_RELAY_SOLAR    | D2             | Pin to connect relais for solar pump                  |

{{% alert note %}}
TODO: improve PIN usage (see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
{{% /alert %}}

## Power Supply

In my environment I use the USB to power the ESP8266 via small USB-Power-Adapter andan additional
230V power plug to be used as source for the power of the pumps which are switched via the relais.
