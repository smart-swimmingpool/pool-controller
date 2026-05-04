/**
 * Smart Swimming Pool - Pool Contoller
 *
 * Main entry of sketch.
 */

#include <Arduino.h>
#include <Homie.h>
#include <SPI.h>
#include "platform/PinDefinitions.hpp"
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

HomieSetting<long> loopIntervalSetting("loop-interval", "The processing interval in seconds");

HomieSetting<double> temperatureMaxPoolSetting("temperature-max-pool", "Maximum temperature of solar");
HomieSetting<double> temperatureMinSolarSetting("temperature-min-solar", "Minimum temperature of solar");
HomieSetting<double> temperatureHysteresisSetting("temperature-hysteresis", "Temperature hysteresis");

HomieSetting<const char*> operationModeSetting("operation-mode", "Operational Mode");

LoggerNode LN;

DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PlatformPins::DS_SOLAR, TEMP_READ_INTERVAL);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PlatformPins::DS_POOL, TEMP_READ_INTERVAL);
#ifdef ESP32
ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVAL);
#endif
RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PlatformPins::RELAY_POOL);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PlatformPins::RELAY_SOLAR);

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
  TimerSetting ts      = operationModeNode.getTimerSetting();  //TODO: Configurable
  ts.timerStartHour    = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour      = 17;
  ts.timerEndMinutes   = 30;
  operationModeNode.setTimerSetting(ts);

  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  // add the rules - using unique_ptr for automatic memory management
  operationModeNode.addRule(std::make_unique<RuleAuto>(&solarPumpNode, &poolPumpNode));
  operationModeNode.addRule(std::make_unique<RuleManu>());
  operationModeNode.addRule(std::make_unique<RuleBoost>(&solarPumpNode, &poolPumpNode));
  operationModeNode.addRule(std::make_unique<RuleTimer>(&solarPumpNode, &poolPumpNode));

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

  operationModeSetting.setDefaultValue("auto").setValidator([](const char* candidate) {
    return (strcmp(candidate, "auto")) || (strcmp(candidate, "manu")) || (strcmp(candidate, "boost"));
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
