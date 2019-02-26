/**
 * ESP32 zur Steuerung des Pools:
 * - 2 Temperaturfühler
 * - Interner Temperatur-Sensor
 * - 433MHz Sender für Pumpnsteuerung
 *
 * Wird über openHAB gesteurt.
 */

#include <Homie.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <RCSwitch.h>

#include "ConstantValues.hpp"
#include "CurrentValue.hpp"

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "RCSwitchNode.hpp"
#include "Rule.hpp"

const int PIN_DS_SOLAR = 16;  // Pin of Temp-Sensor Solar
const int PIN_DS_POOL  = 17;  // Pin of Temp-Sensor Pool
const int PIN_RSSWITCH = 18;  // Data-Pin of 433MHz Sender

const int PIN_RELAY_POOL  = 19;
const int PIN_RELAY_SOLAR = 20;


const int TEMP_READ_INTERVALL = 60;  //Sekunden zwischen Updates der Temperaturen.

HomieSetting<long> temperaturePublishIntervalSetting("temperaturePublishInterval", "The temperature publish interval in seconds");

HomieSetting<long> temperatureMaxPoolSetting("temperatureMaxPoolSetting", "Maximum temperature of solar");
HomieSetting<long> temperatureMinSolarSetting("temperatureMinSolarSetting", "Minimum temperature of solar");
HomieSetting<long> temperatureHysteresisSetting("temperatureHysteresisSetting", "Temperature hysteresis");

HomieSetting<long> operationStatusSetting("operationStatusSetting", "Operational Status");

DallasTemperatureNode solarTemperatureNode("solarTemp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVALL);
DallasTemperatureNode poolTemperatureNode("poolTemp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVALL);
ESP32TemperatureNode  ctrlTemperatureNode("controllerTemp", "Controller Temperature", TEMP_READ_INTERVALL);

RelayModuleNode poolPumpNode("poolPump", "Pool Pump", PIN_RELAY_POOL);
RelayModuleNode solarPumpNode("solarPump", "Solar Pump", PIN_RELAY_SOLAR);

RCSwitchNode poolPumpeRCNode("poolPumpRC", "Pool Pump RC", PIN_RSSWITCH, "11111", "10000");
RCSwitchNode solarPumpeRCNode("solarPumpRC", "Solar Pump RC", PIN_RSSWITCH, "11111", "01000");

//RS Switches via 433MHz
RCSwitch mySwitch = RCSwitch();

CurrentValues currentValues = CurrentValues();

/**
 * Handler for switching pool Pump on/off.
 */
bool onPoolPumpSwitchHandler(const HomieRange& range, const String& value) {
  Homie.getLogger() << "〽 poolPumpSwitchOnHandler -> range " << range.index << " value=" << value << endl;
  bool retval;

  if (value != "true" && value != "false") {

    retval = false;
  } else {

    if (value == "true") {
      // Switch on
      mySwitch.switchOn("11111", "10000");  //todo: make configurable
    } else {
      mySwitch.switchOff("11111", "10000");  //todo: make configurable
    }

    poolPumpNode.setProperty(cSwitch).send(value);
    bool on = (value == "true");
    Homie.getLogger() << "Switch is " << (on ? "on" : "off") << endl;

    retval = true;
  }

  Homie.getLogger() << "〽 poolPumpSwitchOnHandler <-" << retval << endl;
  return retval;
}

/**
 * Handler for switching solar Pump on/off.
 */
bool onSolarPumpSwitchHandler(const HomieRange& range, const String& value) {
  Homie.getLogger() << "〽 solarPumpSwitchOnHandler -> range " << range.index << " value=" << value << endl;

  bool retval;

  if (value != "true" && value != "false") {

    Homie.getLogger() << "reveived invalid value: " << value << endl;
    solarPumpNode.setProperty("status").send("reveived invalid value: " + value);

    retval = false;

  } else {

    if (value == "true") {
      // Switch on
      mySwitch.switchOn("11111", "01000");  //todo: make configurable
    } else {
      mySwitch.switchOff("11111", "01000");  //todo: make configurable
    }

    const bool on = (value == "true");

    Homie.getLogger() << "Switch is " << (on ? "on" : "off") << endl;
    solarPumpNode.setProperty(cSwitch).send(value);
    solarPumpNode.setProperty("status").send("ok");

    retval = true;
  }

  Homie.getLogger() << "〽 solarPumpSwitchOnHandler <-" << retval << endl;
  return retval;
}

/**
 * Homie Setup handler.
 * Only called when wifi and mqtt are connected.
 */
void setupHandler() {

  poolPumpNode.advertise(cSwitch).setName("Switch").setDatatype(cDataTypBoolean).settable(onPoolPumpSwitchHandler);
  poolPumpNode.advertise("status").setName("Status");

  solarPumpNode.advertise(cSwitch).setName("Switch").setDatatype(cDataTypBoolean).settable(onSolarPumpSwitchHandler);
  solarPumpNode.advertise("status").setName("Status");

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


  operationStatusSetting.setDefaultValue(0).setValidator(
    [](int candidate) { return (candidate >= 0) && (candidate <= 3); });

  ctrlTemperatureNode.setMeasurementInterval(temperaturePublishIntervalSetting.get());
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
  //Homie.disableLogging();
  Homie.setSetupFunction(setupHandler);

  Homie.setup();

  Homie.getLogger() << "✔ Setup ready" << endl;
}

/**
 * Main loop of ESP.
 */
void loop() {

  Homie.loop();
}