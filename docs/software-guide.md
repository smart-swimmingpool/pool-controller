---
title: Software Guide of Pool Controller
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
    name: Software Guide
    weight: 30
---

## Development Environment

## Required Libraries

- [Homie-ESP8266](https://github.com/homieiot/homie-esp8266)
- [RelayModule](https://github.com/YuriiSalimov/RelayModule)
- [Vector](https://github.com/tomstewart89/Vector)
- DallasTemperature
- Adafruit Unified Sensor
- DHT sensor library
- NTPClient
- TimeZone
- [Time](https://github.com/xoseperez/Time)

Many thanks to maintainers of these libraries!

## Defines

Within the sources at `main.cpp` there are someconstant defined settings. For the PIN assignment
see also at [hardware guide](../hardware-guide/#esp8266-pin-usage).

```cpp
const uint8_t PIN_DS_SOLAR = D5;  // Pin of Temp-Sensor Solar
const uint8_t PIN_DS_POOL  = D6;  // Pin of Temp-Sensor Pool

const uint8_t PIN_RELAY_POOL  = D1;
const uint8_t PIN_RELAY_SOLAR = D2;

const uint8_t TEMP_READ_INTERVALL = 30;
```

## Configuration

Homie-ESP8266 supports configuration (e.g. WiFi credentials) using JSON-files.
How to upload JSON config files see [Homie-esp8266 docu](https://homieiot.github.io/homie-esp8266/docs/develop/configuration/json-configuration-file/).

### Example `config.json`

```json
{
  "name": "Pool Controller",
  "device_id": "pool-controller",
  "wifi": {
    "ssid": "<SSID>",
    "password": "<XXX>"
  },
  "mqtt": {
    "host": "<MQTT_HOST>",
    "port": 1883
  },
  "ota": {
    "enabled": true
  },
  "settings": {
    "loop-interval": 60,
    "temperature-max-pool": 28,
    "temperature-min-solar": 50,
    "temperature-hysteresis": 0.5
  }
}
```

## MQTT Communication

### Clearing retained messages

In some cases some retained messages can be wanted and we don’t want to clear all the retained messages.

The messages will have to be cleared one by one using the topic

To clear a specific message:

```bash
mosquitto_pub -h hostname -t homie -n -r -d
```
