# Pool Controller 2.0 | 🏊 Smart Swimmingpool

## Hardware Guide


### Parts List (BOM)

- 1 * ESP8266 NodeMCU Controller ([Amazon](https://amzn.to/2Ze9DSh))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2ZlfZ2c))
- 1 * Relais-Module 5V ([Amazon](https://amzn.to/31RBd5s))

### Circuit

![Wireing of Pool Controller](../pool-controller_Steckplatine.png)

see: [pool-controller.fzz](../pool-controller.fzz)

#### ESP8266 PIN Usage

- PIN_DS_SOLAR = D5  -> Pin of temperature sensor for solar
- PIN_DS_POOL  = D6  -> Pin of temperature sensor for pool water
- PIN_RELAY_POOL  = D1 -> pin to connect relais for pool pump
- PIN_RELAY_SOLAR = D2 -> pin to connect relais for solar pump

| TODO: see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
