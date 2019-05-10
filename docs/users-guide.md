# 🏊 Smart Swimmingpool Controller 2.0 - Users Guide



## Settings

There are some specific settings for the controller:

- Pool max. temperature: The maximum temperature of the water in the pool which should not be exceeded.
  
  - Unit: `°C`
  - Default: `29`

- Solar min temperature: The minimum temerature of the heat storage tank which should not be fall below.

  - Unit: `°C`
  - Default: `50`

- Hysteresis: Hysteresis which is used to verify if heating should be enabled or disabled to prevent fast toggeling.

  - Unit: `K`
  - Default: `1`

- pump timer 
  - start h/min
  - end h/min

## Rules

The smart swimmingpool controller implements `Rules` to handle different situations:

### Rule: Manual

The pump for cleaning and solar heating are enabled/disabled completely manual and independent.

### Rule: Timer

This rule enables the cleaning pump based on timer settings. 
Solar heating is disabled.

### Rule: Auto

This rule enables the cleaning pump based on timer settings. 
Solar heating is enabled if cleaning pump is enabled and the heat storage tank has enough temperature. If the maximum temperature of the pool water is reached, the solar heating is disabled.


### Rule Boost

Heating of pool water with all power.

## OpenHAB

TBD.

## MQTT Interface

The Smart Swimmingpool Controller uses MQTT to communicate with your smarthome. For the transmission of data the standard [Homie 3.0](https://homieiot.github.io) is used.

Using Homie 3.0 it is possible to integrate Smart Pool Controller directly in open source smarthome server [openHAB](https://www.openhab.org/).

### Device

### Properties
