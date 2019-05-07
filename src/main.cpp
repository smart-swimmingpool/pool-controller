/**
 * ESP32 zur Steuerung des Pools:
 * - 2 Temperaturfühler
 * - Interner Temperatur-Sensor
 * - 433MHz Sender für Pumpnsteuerung
 *
 * Wird über openHAB gesteuert.
 */

#include <Arduino.h>
#include <Homie.h>

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"
#include "Rule.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

#include "TimeClientHelper.hpp"

#ifdef ESP32
//#include <WiFi.h>
#elif defined(ESP8266)
//#include <ESP8266WiFi.h>
#endif

#ifdef ESP32
const uint8_t PIN_DS_SOLAR = 15;  // Pin of Temp-Sensor Solar
const uint8_t PIN_DS_POOL  = 16;  // Pin of Temp-Sensor Pool
const uint8_t PIN_DHT11    = 17;

const uint8_t PIN_RSSWITCH = 18;  // Data-Pin of 433MHz Sender

const uint8_t PIN_RELAY_POOL  = 18;
const uint8_t PIN_RELAY_SOLAR = 19;
#elif defined(ESP8266)
const uint8_t PIN_DS_SOLAR = D5;  // Pin of Temp-Sensor Solar
const uint8_t PIN_DS_POOL  = D6;  // Pin of Temp-Sensor Pool

const uint8_t PIN_RELAY_POOL  = D1;
const uint8_t PIN_RELAY_SOLAR = D2;
#endif
const uint8_t TEMP_READ_INTERVALL = 30;  //Sekunden zwischen Updates der Temperaturen.


HomieSetting<long> loopIntervalSetting("loop-interval", "The processing interval in seconds");

HomieSetting<double> temperatureMaxPoolSetting("temperature-max-pool", "Maximum temperature of solar");
HomieSetting<double> temperatureMinSolarSetting("temperature-min-solar", "Minimum temperature of solar");
HomieSetting<double> temperatureHysteresisSetting("temperature-hysteresis", "Temperature hysteresis");

HomieSetting<const char*> operationModeSetting("operation-mode", "Operational Mode");

DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVALL);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVALL);
#ifdef ESP32
ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVALL);
#endif
RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

OperationModeNode operationModeNode("operation-mode", "Operation Mode");

unsigned long _measurementInterval = 10;
unsigned long _lastMeasurement;

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::STANDALONE_MODE:
      // Do whatever you want when standalone mode is started
      break;
    case HomieEventType::CONFIGURATION_MODE:
      // Do whatever you want when configuration mode is started
      break;
    case HomieEventType::NORMAL_MODE:
      // Do whatever you want when normal mode is started
      break;
    case HomieEventType::OTA_STARTED:
      // Do whatever you want when OTA is started
      break;
    case HomieEventType::OTA_PROGRESS:
      // Do whatever you want when OTA is in progress

      // You can use event.sizeDone and event.sizeTotal
      break;
    case HomieEventType::OTA_FAILED:
      // Do whatever you want when OTA is failed
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      // Do whatever you want when OTA is successful
      break;
    case HomieEventType::ABOUT_TO_RESET:
      // Do whatever you want when the device is about to reset
      break;
    case HomieEventType::WIFI_CONNECTED:
      // Do whatever you want when Wi-Fi is connected in normal mode
      Homie.getLogger() << F("WIFI_CONNECTED: timeClientSetup.") << endl;
      timeClientSetup();
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      // Do whatever you want when Wi-Fi is disconnected in normal mode
      Homie.getLogger() << F("WIFI_DISCONNECTED: reason: ") << event.wifiReason << endl;
      // You can use event.wifiReason
      break;
    case HomieEventType::MQTT_READY:
      // Do whatever you want when MQTT is connected in normal mode
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      // Do whatever you want when MQTT is disconnected in normal mode

      // You can use event.mqttReason
      break;
    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED:
      // Do whatever you want when an MQTT packet with QoS > 0 is acknowledged by the broker

      // You can use event.packetId
      break;
    case HomieEventType::READY_TO_SLEEP:
      // After you've called `prepareToSleep()`, the event is triggered when MQTT is disconnected
      break;
    case HomieEventType::SENDING_STATISTICS:
      // Do whatever you want when statistics are sent in normal mode
      break;
  }
}

/**
 * Homie Setup handler.
 * Only called when wifi and mqtt are connected.
 */
void setupHandler() {

  // set mesurement intervals
  long _loopInterval = loopIntervalSetting.get();

  solarTemperatureNode.setMeasurementInterval(_loopInterval);
  poolTemperatureNode.setMeasurementInterval(_loopInterval);

  poolPumpNode.setMeasurementInterval(_loopInterval);
  solarPumpNode.setMeasurementInterval(_loopInterval);

#ifdef ESP32
  ctrlTemperatureNode.setMeasurementInterval(_loopInterval);
#endif

  operationModeNode.setMode(operationModeSetting.get());
  operationModeNode.setPoolMaxTemperatur(temperatureMaxPoolSetting.get());
  operationModeNode.setSolarMinTemperature(temperatureMinSolarSetting.get());
  operationModeNode.setTemperaturHysteresis(temperatureHysteresisSetting.get());
  TimerSetting ts = operationModeNode.getTimerSetting(); //TODO: Configurable
  ts.timerStartHour = 10;
  ts.timerStartMinutes = 0;
  ts.timerEndHour = 17;
  ts.timerEndMinutes = 30;
  operationModeNode.setTimerSetting(ts);

  // add the rules
  RuleAuto* autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);

  RuleManu* manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);

  RuleBoost* boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(boostRule);

  RuleTimer* timerRule = new RuleTimer(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(timerRule);

  _lastMeasurement = 0;
}

/**
 * Startup of controller.
 */
void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Homie_setFirmware("pool-controller", "1.0.0");  // The underscore is not a typo! See Magic bytes
  Homie_setBrand("smart-swimmingpool");

  //default intervall of sending Temperature values
  loopIntervalSetting.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](long candidate) {
    return (candidate >= 0) && (candidate <= 300);
  });

  temperatureMaxPoolSetting.setDefaultValue(28.5).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 30); });

  temperatureMinSolarSetting.setDefaultValue(25.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 100); });

  temperatureHysteresisSetting.setDefaultValue(1.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 10); });

  operationModeSetting.setDefaultValue("auto").setValidator([](const char* candidate) {
    return (strcmp(candidate, "auto")) || (strcmp(candidate, "manu")) || (strcmp(candidate, "boost"));
  });

  //WiFi.disconnect();

  //Homie.disableLogging();
  Homie.setSetupFunction(setupHandler);
  Homie.onEvent(onHomieEvent);
  Homie.setup();

  Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
}

/**
 * Main loop of ESP.
 */
void loop() {

  Homie.loop();
/*
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {

    //Homie.getLogger() << "main::loop" << endl;
    Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << F(" max. free block size: ") << ESP.getMaxFreeBlockSize() << endl;

    _lastMeasurement = millis();
  }
*/
}
