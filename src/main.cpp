/**
 * Smart Swimming Pool - Pool Contoller
 *
 * Main entry of sketch.
 */

#include <Arduino.h>
#include <Homie.h>
#include <SPI.h>
#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"
#include "Rule.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
#include "RuleTimer.hpp"

#include "LoggerNode.hpp"
#include "TimeClientHelper.hpp"

#ifdef ESP32
const uint8_t PIN_DS_SOLAR = 15;  // Pin of Temp-Sensor Solar
const uint8_t PIN_DS_POOL  = 16;  // Pin of Temp-Sensor Pool

const uint8_t PIN_RELAY_POOL  = 18;
const uint8_t PIN_RELAY_SOLAR = 19;
#elif defined(ESP8266)

// see: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
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

HomieSetting<double> poolVolumeSetting("pool-volume", "Pool volume in cubic meters");
HomieSetting<double> pumpCapacitySetting("pump-capacity", "Pump capacity in cubic meters per hour");
HomieSetting<bool> useTemperatureBasedDurationSetting("use-temp-based-duration", "Use temperature-based filtration duration");

HomieSetting<const char*> operationModeSetting("operation-mode", "Operational Mode");

LoggerNode LN;

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
  operationModeNode.setPoolMaxTemperature(temperatureMaxPoolSetting.get());
  operationModeNode.setSolarMinTemperature(temperatureMinSolarSetting.get());
  operationModeNode.setTemperatureHysteresis(temperatureHysteresisSetting.get());
  operationModeNode.setPoolVolume(poolVolumeSetting.get());
  operationModeNode.setPumpCapacity(pumpCapacitySetting.get());
  operationModeNode.setUseTemperatureBasedDuration(useTemperatureBasedDurationSetting.get());
  TimerSetting ts      = operationModeNode.getTimerSetting();  //TODO: Configurable
  ts.timerStartHour    = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour      = 17;
  ts.timerEndMinutes   = 30;
  operationModeNode.setTimerSetting(ts);

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

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
  Serial.begin(SERIAL_SPEED);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }
  Homie.setLoggingPrinter(&Serial);

  Homie_setFirmware("pool-controller", "2.0.0");
  Homie_setBrand("smart-swimmingpool");

  //WiFi.setSleepMode(WIFI_NONE_SLEEP); //see: https://github.com/esp8266/Arduino/issues/5083

  //default intervall of sending Temperature values
  loopIntervalSetting.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](long candidate) {
    return (candidate >= 0) && (candidate <= 300);
  });

  temperatureMaxPoolSetting.setDefaultValue(28.5).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 30); });

  temperatureMinSolarSetting.setDefaultValue(55.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 100); });

  temperatureHysteresisSetting.setDefaultValue(1.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 10); });

  // Pool volume default: 30 m³ (typical small pool)
  poolVolumeSetting.setDefaultValue(30.0).setValidator(
      [](double candidate) { return (candidate > 0) && (candidate <= 1000); });

  // Pump capacity default: 6 m³/h (typical pool pump)
  pumpCapacitySetting.setDefaultValue(6.0).setValidator(
      [](double candidate) { return (candidate > 0) && (candidate <= 100); });

  // Use temperature-based duration: default false (use fixed timer)
  // This allows users to choose between dynamic temperature-based calculation
  // and traditional fixed start/stop times
  useTemperatureBasedDurationSetting.setDefaultValue(false);

  operationModeSetting.setDefaultValue("auto").setValidator([](const char* candidate) {
    return (strcmp(candidate, "auto") == 0) || (strcmp(candidate, "manu") == 0) || 
           (strcmp(candidate, "boost") == 0) || (strcmp(candidate, "timer") == 0);
  });

  //Homie.disableLogging();
  Homie.setSetupFunction(setupHandler);

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();

  LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Free heap: %d", ESP.getFreeHeap());
  Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
}

/**
 * Main loop of ESP.
 */
void loop() {

  Homie.loop();
}
