/**
 * ESP32 zur Steuerung des Pools:
 * - 2 Temperaturfühler
 * - Interner Temperatur-Sensor
 * - 433MHz Sender für Pumpnsteuerung
 *
 * Wird über openHAB gesteurt.
 */

#include <Homie.h>

#include "ConstantValues.hpp"
#include "CurrentValue.hpp"

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "RCSwitchNode.hpp"
#include "Rule.hpp"

#ifdef ESP32
const int PIN_DS_SOLAR = 15;  // Pin of Temp-Sensor Solar
const int PIN_DS_POOL  = 16;  // Pin of Temp-Sensor Pool
const int PIN_DHT11    = 17;

const int PIN_RSSWITCH = 18;  // Data-Pin of 433MHz Sender

const int PIN_RELAY_POOL  = 18;
const int PIN_RELAY_SOLAR = 19;
#else
const int PIN_DS_SOLAR = D5;  // Pin of Temp-Sensor Solar
const int PIN_DS_POOL  = D6;  // Pin of Temp-Sensor Pool
const int PIN_DHT11    = D7;

const int PIN_RSSWITCH = 18;  // Data-Pin of 433MHz Sender

const int PIN_RELAY_POOL  = D1;
const int PIN_RELAY_SOLAR = D2;
#endif
const int TEMP_READ_INTERVALL = 60;  //Sekunden zwischen Updates der Temperaturen.

HomieSetting<long> temperaturePublishIntervalSetting("temperature-publish-interval",
                                                     "The temperature publish interval in seconds");

HomieSetting<long> temperatureMaxPoolSetting("temperature-max-pool", "Maximum temperature of solar");
HomieSetting<long> temperatureMinSolarSetting("temperature-min-solar", "Minimum temperature of solar");
HomieSetting<long> temperatureHysteresisSetting("temperature-hysteresis", "Temperature hysteresis");

HomieSetting<long> operationStatusSetting("operation-status", "Operational Status");

DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVALL);
DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVALL);
ESP32TemperatureNode  ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVALL);

RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

//RCSwitchNode poolPumpeRCNode("poolPumpRC", "Pool Pump RC", PIN_RSSWITCH, "11111", "10000");
//RCSwitchNode solarPumpeRCNode("solarPumpRC", "Solar Pump RC", PIN_RSSWITCH, "11111", "01000");

CurrentValues currentValues = CurrentValues();

unsigned long _measurementInterval;
unsigned long _lastMeasurement;

/**
 *
 */
void loopHandler() {
  if (millis() - _lastLoop >= _loopInterval * 1000UL || _lastLoop == 0) {

    _lastLoop = millis();
  }
}

/**
 * Homie Setup handler.
 * Only called when wifi and mqtt are connected.
 */
void setupHandler() {

  //default intervall of sending Temperature values
  temperaturePublishIntervalSetting.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](long candidate) {
    return (candidate >= 0) && (candidate <= 300);
  });

  temperatureMaxPoolSetting.setDefaultValue(28.5).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 30); });

  temperatureMinSolarSetting.setDefaultValue(50.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 100); });

  temperatureHysteresisSetting.setDefaultValue(1.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 10); });

  operationStatusSetting.setDefaultValue(0).setValidator([](int candidate) { return (candidate >= 0) && (candidate <= 3); });

  // set mesurement intervals
  _measurementInterval = temperaturePublishIntervalSetting.get();
  ctrlTemperatureNode.setMeasurementInterval(_measurementInterval);
  solarTemperatureNode.setMeasurementInterval(_measurementInterval);
  poolTemperatureNode.setMeasurementInterval(_measurementInterval);

  poolPumpNode.setMeasurementInterval(_measurementInterval);
  solarPumpNode.setMeasurementInterval(_measurementInterval);
}

/**
 * Startup of controller.
 */
void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println(F("-------------------------------------"));
  Serial.println(F(" Pool Controller                     "));
  Serial.println(F("-------------------------------------"));

  //mySwitch.enableTransmit(PIN_RSSWITCH);
  //mySwitch.setRepeatTransmit(10);
  //mySwitch.setPulseLength(350);

  Homie_setFirmware("pool-controller", "1.0.0");  // The underscore is not a typo! See Magic bytes
  Homie_setBrand("SmartSwimmingpool");
  //Homie.disableLogging();
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);

  Homie.setup();

  _lastMeasurement = 0;
  Homie.getLogger() << "✔ main: Setup ready" << endl;
}

/**
 * Main loop of ESP.
 */
void loop() {

  Homie.loop();

  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();

  }
}
