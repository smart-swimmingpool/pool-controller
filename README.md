# 🏊 Smart Swimmingpool Controller 2.0

[![Smart Swimmingpool](https://img.shields.io/badge/%F0%9F%8F%8A%20-Smart%20Swimmingpool-blue.svg)](https://github.com/smart-swimmingpool)
[![PlatformIO CI](https://github.com/smart-swimmingpool/pool-controller/workflows/PlatformIO%20CI/badge.svg)](https://github.com/smart-swimmingpool/pool-controller/actions?query=workflow%3A%22PlatformIO+CI%22)
[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-v1.4%20adopted-ff69b4.svg)](code-of-conduct.md)
[![All Contributors](https://img.shields.io/badge/all_contributors-1-orange.svg?style=flat-square)](#contributors)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

[![works with MQTT Homie](https://homieiot.github.io/img/works-with-homie.svg "[works with MQTT Homie")](https://homieiot.github.io/)

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/J3J33A8DT)

**🏊 The Homie 3.0 compatible Smart Swimmingpool Controller 🎛️**

Manage your swmming pool on the smart way to enjoy it in confortable and cheap (less than 100€) way.

Discussions: <https://github.com/smart-swimmingpool/smart-swimmingpool.github.io/discussions>

## Main Features

- [x] Manage water timed circulation for cleaning
- [x] Manage water heating by additional pump for solar circuit
- [x] [Homie 3.0](https://homieiot.github.io/) compatible MQTT messaging
- [x] Independent of specific smarthome servers
  - [x] [openHAB](https://www.openhab.org) since Version 2.4 using MQTT Homie
  - [x] [Home Assistant](home-assistant.io) using MQTT Homie
- [x] Timesync via NTP (europe.pool.ntp.org)
- [x] Logging-Information via Homie-Node

## Planned Features

- [ ] Configurable NTP Server (currently hardcoded: europe.pool.ntp.org)
- [ ] be more smart: self learning for improved pool pump timed circulation for cleaning and heating
- [ ] two separate circulation cycles
- [ ] store configuration changes persistent on conroller
- [ ] temperature based cleaning circulation time (colder == shorter, hotter == longer)
- [ ] Improved sketch to work completly without WiFi connection
      - Homie should run without WiFi connection
      - enhance sketch using display and buttons to setup environment.
- see also the [issue list](https://github.com/smart-swimmingpool/pool-controller/issues)

## Guides

- [Users Guide](docs/users-guide.md)
- [Hardware Guide](docs/hardware-guide.md)
- [Software Guide](docs/software-guide.md)

## Contributors

Thanks goes to these wonderful people
([emoji key](https://github.com/all-contributors/all-contributors#emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the
[all-contributors](https://github.com/all-contributors/all-contributors)
specification. Contributions of any kind welcome!

## Credits

- [Community of Homie-ESP8266](https://gitter.im/homie-iot/ESP8266)
- [Lübbe Onken](http://github.com/luebbe) for `TimeClientHelper`
- [Ian Hubbertz](https://github.com/euphi) for [HomieLoggerNode](https://github.com/euphi/HomieLoggerNode)

## License

[LICENSE](LICENSE)

---

DIY My Smart Home: (https://medium.com/diy-my-smart-home)
