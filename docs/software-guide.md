# 🏊 Smart Swimmingpool Controller 2.0 - Software Guide

## Development Environment

## Required Libraries


- [Homie-ESP8266 (develop-v3)](https://github.com/homieiot/homie-esp8266)
- [RelayModule](https://github.com/YuriiSalimov/RelayModule)
- [Vector](https://github.com/tomstewart89/Vector)
- DallasTemperature
- Adafruit Unified Sensor
- DHT sensor library
- NTPClient
- TimeZone
- [Time](https://github.com/xoseperez/Time)

## Defines

```cpp
const uint8_t PIN_DS_SOLAR = D5;  // Pin of Temp-Sensor Solar
const uint8_t PIN_DS_POOL  = D6;  // Pin of Temp-Sensor Pool

const uint8_t PIN_RELAY_POOL  = D1;
const uint8_t PIN_RELAY_SOLAR = D2;

const uint8_t TEMP_READ_INTERVALL = 30;
```
