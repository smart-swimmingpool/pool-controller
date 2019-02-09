#pragma once

#include <Homie.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <RCSwitch.h>

#include "CurrentValue.hpp"

#define PIN_DS_SOLAR 16 // Temp-Sensor Solar
#define PIN_DS_POOL 17  // Temp-Sensor Pool
#define PIN_RSSWITCH 18 // für 433MHz Sender

#define TEMP_READ_INTERVALL 60   //Sekunden zwischen Updates der Temperaturen.

extern "C" {

uint8_t temprature_sens_read();
}

//Ticker
Ticker tickerTemperaturePool;
Ticker tickerTemperatureSolar;
Ticker tickerTemperatureCtrl;
