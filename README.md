# Pool Control 2.0

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)


Homie 3.0 compatible pool controller.

## Hardware

- ESP32 Controller ([Amazon](https://amzn.to/2CVjDCI))
- Temperature Sensor Pool ([Amazon](https://amzn.to/2HJHdrL))
- Temperature Sensor Solar ([Amazon](https://amzn.to/2HJHdrL))
- 433MHz-Receiver Module ([Amazon](https://amzn.to/2HXrbLl))
- 433MHz Radio sockets for Solar- and Pool-Pumps ([Amazon](https://amzn.to/2G3VONo))

### Configuration

PIN Usage:
* PIN_DS_SOLAR = 16; // Temp Solar
* PIN_DS_POOL  = 17; // Temp Pool
* PIN_RSSWITCH = 18; // für 433MHz Sender

### Layout

see: [data/pool-controller.fzz](data/pool-controller.fzz)


## Implementation

## Used Libraries

* Homie-esp8266
* DallasTemperature
* DHT
* rc-switch
