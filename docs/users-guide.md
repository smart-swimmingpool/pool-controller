---
title: Users Guide of Pool Controller
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
    name: Users Guide
    weight: 40
---

## Setup

## Booting Controller

Booting the controller, it will give feedback on establishing WiFi connection andconnection to MQTT broker:

* "LED" ![Slowly blinking LED](led_wifi.gif)
    Slowly when connecting to the Wi-Fi
* "LED" ![Fast blinking LED](led_mqtt.gif)
    Faster when connecting to the MQTT broker

## Settings

There are some specific settings for the controller:

- **Pool max. temperature:** The maximum temperature of the water in the pool which should not be exceeded.

  - Unit: `°C`
  - Default value: `29`

- **Solar min temperature:** The minimum temerature of the heat storage tank which should not be fall below.

  - Unit: `°C`
  - Default value: `50`

- **Hysteresis:** Hysteresis in Kelvin which is used to verify if heating should be enabled or disabled to prevent fast toggeling.

  - Unit: `K`
  - Default value: `1`

- **Pump Timer:** time range when pool pump has to run.
  - start h/min
  - end h/min

- **Loop Interval:**

  - Unit: `sec`
  - Default value: `30`

## Rules

The **Smart Swimmingpool Controller** implements `Rules` to handle different situations:

### Rule: Manual

The pump for cleaning and solar heating are enabled/disabled completely manual and independent.

### Rule: Timer

This rule enables the cleaning pump based on timer settings.
Solar heating is disabled.

### Rule: Auto

This rule enables the cleaning pump based on timer settings.
Solar heating is enabled **smart** if cleaning pump is enabled by timer and the heat storage tank has enough temperature.

If the maximum temperature of the pool water is reached, the solar heating is disabled.

### Rule: Boost

Heating of pool water with all power.

## MQTT Interface

The **Smart Swimmingpool Controller** uses [MQTT](http://mqtt.org/) to communicate with your smart home. For the transmission of data the IoT standard [Homie 3.0](https://homieiot.github.io) is used.

Using Homie 3.0 it is possible to integrate **Smart Pool Controller** directly in open source smarthome server [openHAB](https://www.openhab.org/) or [Home Assistant](https://www.home-assistant.io/).


## OpenHAB Integration

The **Smart Swimmingpool Controller** could be integrated in [openHAB](https://www.openhab.org) since version 2.4.

It is possible to interact with the controller to enable/disable the pump or to swith the current rule.

Also it is possible to monitor the current values of temperatures or states.

At least the [settings](#settings) could be updated, too.

- TODO: add example of openhab config.

### Device

### Properties
