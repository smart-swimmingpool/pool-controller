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
#include "ButtonControlNode.hpp"
#include "LocalDisplayNode.hpp"
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

// Mode-cycle push-button (connect between pin and GND)
// GPIO23 is used here so GPIO21/22 remain free for the default I2C bus (SDA/SCL).
const uint8_t PIN_BUTTON_MODE = 23;

// Settings navigation buttons (connect between pin and GND)
const uint8_t PIN_BUTTON_UP     = 34;  // input-only GPIO, suitable for buttons
const uint8_t PIN_BUTTON_DOWN   = 35;  // input-only GPIO, suitable for buttons
const uint8_t PIN_BUTTON_SELECT = 32;

// Display I2C (default ESP32 I2C bus)
const uint8_t PIN_DISPLAY_SDA = 21;
const uint8_t PIN_DISPLAY_SCL = 22;

#elif defined(ESP8266)

// see: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
const uint8_t PIN_DS_SOLAR = D5;  // Pin of Temp-Sensor Solar
const uint8_t PIN_DS_POOL  = D6;  // Pin of Temp-Sensor Pool

const uint8_t PIN_RELAY_POOL  = D1;
const uint8_t PIN_RELAY_SOLAR = D2;

// Mode-cycle push-button (connect between pin and GND)
const uint8_t PIN_BUTTON_MODE = D7;

// D1/D2 are occupied by relays so the display must use the alternative
// software-I2C pins D3 (GPIO0) and D4 (GPIO2).
// NOTE: D3 and D4 are boot-sensitive; ensure both lines are pulled HIGH at
// power-on (the 4.7 kΩ I2C pull-up resistors on the display module do this).
const uint8_t PIN_DISPLAY_SDA = D3;  // GPIO0 – boot-sensitive, needs pull-up
const uint8_t PIN_DISPLAY_SCL = D4;  // GPIO2 – boot-sensitive, needs pull-up

// ESP8266 has no remaining GPIOs for UP/DOWN/SELECT buttons without a
// hardware redesign; those features are therefore ESP32-only.

#endif
const uint8_t TEMP_READ_INTERVALL = 30;  //Sekunden zwischen Updates der Temperaturen.

HomieSetting<long> loopIntervalSetting("loop-interval", "The processing interval in seconds");

HomieSetting<double> temperatureMaxPoolSetting("temperature-max-pool", "Maximum temperature of solar");
HomieSetting<double> temperatureMinSolarSetting("temperature-min-solar", "Minimum temperature of solar");
HomieSetting<double> temperatureHysteresisSetting("temperature-hysteresis", "Temperature hysteresis");

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

// Local display: shows temperatures, mode, and pump states without internet.
LocalDisplayNode localDisplayNode("local-display", "Local Display",
                                  &operationModeNode,
                                  &poolPumpNode, &solarPumpNode,
                                  &poolTemperatureNode, &solarTemperatureNode,
                                  PIN_DISPLAY_SDA, PIN_DISPLAY_SCL);

// Button control: MODE button cycles modes; on ESP32 UP/DOWN/SELECT open a
// settings menu to adjust temperatures and timer settings offline.
#ifdef ESP32
ButtonControlNode buttonControlNode("button-control", "Button Control",
                                    PIN_BUTTON_MODE,
                                    &operationModeNode,
                                    &localDisplayNode,
                                    PIN_BUTTON_UP,
                                    PIN_BUTTON_DOWN,
                                    PIN_BUTTON_SELECT);
#else
ButtonControlNode buttonControlNode("button-control", "Button Control",
                                    PIN_BUTTON_MODE,
                                    &operationModeNode,
                                    &localDisplayNode);
#endif

unsigned long _measurementInterval = 10;
unsigned long _lastMeasurement;

/**
 * Homie Setup handler.
 * Only called when WiFi and MQTT are connected.
 * Applies MQTT-fetched settings (intervals, temperatures, timer) to nodes.
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

  // Wire temperature sensors and rules before Homie.setup() so that
  // rule evaluation works offline (without a WiFi / MQTT connection).
  operationModeNode.setPoolTemperatureNode(&poolTemperatureNode);
  operationModeNode.setSolarTemperatureNode(&solarTemperatureNode);

  TimerSetting ts      = operationModeNode.getTimerSetting();  //TODO: Configurable
  ts.timerStartHour    = 10;
  ts.timerStartMinutes = 30;
  ts.timerEndHour      = 17;
  ts.timerEndMinutes   = 30;
  operationModeNode.setTimerSetting(ts);

  RuleAuto* autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);

  RuleManu* manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);

  RuleBoost* boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(boostRule);

  RuleTimer* timerRule = new RuleTimer(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(timerRule);

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
