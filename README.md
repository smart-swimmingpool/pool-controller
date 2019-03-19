# 🏊 Smart Swimmingpool Controller 2.0

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

🏊 Homie 3.0 compatible smart swimmingpool controller 🎛️:

- [ ] Manage water timed circulation for cleaning
- [ ] Manage water heating by additional pump for solar circuit
- [ ] [Homie 3](https://homieiot.github.io/) compatible MQTT messaging
- [ ] Independent of smarthome servers
- [ ] Integration for OpenHab > 2.4

## Hardware / BOM

- ESP32 Controller ([Amazon](https://amzn.to/2CVjDCI))
- Temperature Sensor Pool ([Amazon](https://amzn.to/2HJHdrL))
- Temperature Sensor Solar ([Amazon](https://amzn.to/2HJHdrL))
- 433MHz-Receiver Module ([Amazon](https://amzn.to/2HXrbLl))
- 433MHz Radio sockets for Solar- and Pool-Pumps ([Amazon](https://amzn.to/2G3VONo))

### Configuration

PIN Usage:

- PIN_DS_SOLAR = 16; // Temp Solar
- PIN_DS_POOL = 17; // Temp Pool
- PIN_RSSWITCH = 18; // für 433MHz Sender

### Layout

see: [data/pool-controller.fzz](data/pool-controller.fzz)

## Implementation

## Used Libraries

- Homie-esp8266
- DallasTemperature
- DHT
- rc-switch
- [RelayModule](https://github.com/YuriiSalimov/RelayModule)
- [Vector](https://github.com/tomstewart89/Vector)

# License

[LICENSE](LICENSE)

---

DIY My Smart Home: (https://medium.com/diy-my-smart-home)
