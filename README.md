# 🏊 Smart Swimmingpool Controller 2.0

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)](https://github.com/smart-swimmingpool)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v1.4%20adopted-ff69b4.svg)](code-of-conduct.md)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**🏊 The Homie 3.0 compatible smart swimmingpool controller 🎛️**

## Main Features
- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] [Homie 3](https://homieiot.github.io/) compatible MQTT messaging
- [x] Independent of smarthome servers
- [x] Integration for OpenHab > 2.4 using Homie

## Planned Features
- [ ] Improvements to work without WiFi
- [ ] be more smart: self learning for improved pool pump timed circulation for cleaning and heating

# Guides

- [Users Guide](docs/users-guide.md)

# Development

## Hardware / BOM

- ESP8266 NodeMCU Controller ([Amazon](https://amzn.to/2DPf0LJ))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2HJHdrL))
- Relais-Module 5V ([Amazon](https://amzn.to/2DWCVJw))


### PCB Layout

see: [data/pool-controller.fzz](data/pool-controller.fzz) //TODO: update to new version

### Configuration

PIN Usage:

TODO: see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/ 
- PIN_DS_SOLAR = D5  -> Pin of temperature sensor for solar
- PIN_DS_POOL  = D6  -> Pin of temperature sensor for pool water
- PIN_RELAY_POOL  = D1 -> pin to connect relais for pool pump
- PIN_RELAY_SOLAR = D2 -> pin to connect relais for solar pump

## Implementation

### Used Libraries

- [Homie-esp8266 (develop-v3)](https://github.com/homieiot/homie-esp8266)
- DallasTemperature
- DHT
- [RelayModule](https://github.com/YuriiSalimov/RelayModule)
- [Vector](https://github.com/tomstewart89/Vector)


### Clearing retained messages
In some cases some retained messages can be wanted and we don’t want to clear all the retained messages.

The messages will have to be cleared one by one using the topic

To clear a specific message:

```bash
mosquitto_pub -h hostname -t homie -n -r -d
```

# Credits

- [Community of Homie-ESP8266](https://gitter.im/homie-iot/ESP8266)
- Lübbe Onken (http://github.com/luebbe) for `TimeClientHelper`

# License

[LICENSE](LICENSE)

---

DIY My Smart Home: (https://medium.com/diy-my-smart-home)
