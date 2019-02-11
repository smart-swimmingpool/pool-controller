#pragma once

#include <Homie.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <RCSwitch.h>

#include "CurrentValue.hpp"

#define cStatus "status"
#define cStatusName "Status"
#define cTemperature "temperature"
#define cTemperatureName "Temperature"

#define cHumidity "humidity"
#define cSwitch "switch"
#define cVoltage "voltage"

#define cDataTypFloat "float"
#define cDataTypBoolean "boolean"

#define cUnitTemperature "°C"

extern "C" {

uint8_t temprature_sens_read();
}

//Ticker
Ticker tickerTemperaturePool;
Ticker tickerTemperatureSolar;
Ticker tickerTemperatureCtrl;
