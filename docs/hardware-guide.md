# Pool Controller 2.0 | 🏊 Smart Swimmingpool

## Hardware Guide


### Parts List (BOM)

- ESP8266 NodeMCU Controller ([Amazon](https://amzn.to/2DPf0LJ))
- 2 * DS18B20 Temperature Sensors ([Amazon](https://amzn.to/2HJHdrL))
- Relais-Module 5V ([Amazon](https://amzn.to/2DWCVJw))

### Circuit

#### PCB Layout

see: [data/pool-controller.fzz](pool-controller.fzz)

#### ESP8266 PIN Usage

TODO: see https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/ 
- PIN_DS_SOLAR = D5  -> Pin of temperature sensor for solar
- PIN_DS_POOL  = D6  -> Pin of temperature sensor for pool water
- PIN_RELAY_POOL  = D1 -> pin to connect relais for pool pump
- PIN_RELAY_SOLAR = D2 -> pin to connect relais for solar pump
