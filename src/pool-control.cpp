/**
 * ESP32 zur Steuerung des Pools:
 * - 2 Temperaturfühler
 * - Interner Temperatur-Sensor
 * - 433MHz Sender für Pumpnsteuerung
 *
 * Wird über openHAB gesteurt.
 */

#include "pool-control.hpp"
#include "Rule.hpp"

const int PIN_DS_SOLAR = 16;  // Temp-Sensor Solar
const int PIN_DS_POOL  = 17;  // Temp-Sensor Pool
const int PIN_RSSWITCH = 18;  // für 433MHz Sender

const int TEMP_READ_INTERVALL = 60;  //Sekunden zwischen Updates der Temperaturen.

//GPIO12 & GPIO14 -> Temperatur
OneWire dsSolar(PIN_DS_SOLAR);
OneWire dsPool(PIN_DS_POOL);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensorSolar(&dsSolar);
DallasTemperature sensorPool(&dsPool);

HomieNode solarTemperatureNode("solarTemp", "Solar Temperature", cTemperature);
HomieNode poolTemperatureNode("poolTemp", "Pool Temperature", cTemperature);
HomieNode ctrlTemperatureNode("controllerTemp", "Controller Temperature", cTemperature);

HomieNode poolPumpNode("poolPump", "Pool Pump", cSwitch);
HomieNode solarPumpNode("solarPump", "Solar Pump", cSwitch);

HomieSetting<long> temperaturePublishIntervalSetting("temperaturePublishInterval", "The temperature publish interval in seconds");

HomieSetting<long> temperatureMaxPoolSetting("temperatureMaxPoolSetting", "Maximum temperature of solar");
HomieSetting<long> temperatureMinSolarSetting("temperatureMinSolarSetting", "Minimum temperature of solar");
HomieSetting<long> temperatureHysteresisSetting("temperatureHysteresisSetting", "Temperature hysteresis");

//RS Switches via 433MHz
RCSwitch mySwitch = RCSwitch();

CurrentValues currentValues = CurrentValues();

/**
 * Get temperature of temperature sensor.
 *
 * @param sensor
 */
double getTemperature(DallasTemperature sensor) {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensor.requestTemperatures();  // Send the command to get temperature readings

  double t;
  int    cnt = 0;
  do {
    t = sensor.getTempCByIndex(0);  // Why "byIndex"?  You can have more than one DS18B20 on the same bus.
    // 0 refers to the first IC on the wire
    delay(1);
    Homie.getLogger() << ".";
    cnt++;
    if (cnt > 3) {
      return -127.0;
    }
  } while (t >= 50.0 || t <= -50.0);

  return t;
}

/**
 * Ticker to publish solar temperature.
 */
void onTickerTemperatureSolar() {
  Homie.getLogger() << "〽 onTickerTemperatureSolar ->" << endl;

  const double temp = getTemperature(sensorSolar);
  if (temp > -127.0) {
    Homie.getLogger() << "  • Temperature=" << temp << cUnitTemperature << endl;
    currentValues.setTemperatureSolar(temp);
    solarTemperatureNode.setProperty(cTemperature).send(String(temp, 1));
    solarTemperatureNode.setProperty(cStatus).send("ok");
  } else {
    Homie.getLogger() << "  ✖ Error reading sensor" << endl;
    solarTemperatureNode.setProperty(cTemperature).send("");
    solarTemperatureNode.setProperty(cStatus).send("Error reading sensor");
  }

  Homie.getLogger() << "  onTickerTemperatureSolar <-" << endl;
}

/**
 * Ticker to publish pool temperature.
 */
void onTickerTemperaturePool() {
  Homie.getLogger() << "〽 onTickerTemperaturePool ->" << endl;

  const double temp = getTemperature(sensorPool);

  if (temp > -127.0) {
    Homie.getLogger() << "  • Temperature=" << temp << cUnitTemperature << endl;
    currentValues.setTemperaturePool(temp);
    poolTemperatureNode.setProperty(cTemperature).send(String(temp, 1));
    poolTemperatureNode.setProperty(cStatus).send("ok");
  } else {
    Homie.getLogger() << "  ✖ Error reading sensor" << endl;
    poolTemperatureNode.setProperty(cTemperature).send("");
    poolTemperatureNode.setProperty(cStatus).send("Error reading sensor");
  }

  Homie.getLogger() << "  onTickerTemperaturePool <-" << endl;
}

/**
 * Ticker to publish internal temperature of ESP32.
 */
void onTickerTemperatureCtrl() {
  Homie.getLogger() << "〽 onTickerTemperatureCtrl ->" << endl;

  //internal temp of ESP
  uint8_t      temp_farenheit = temprature_sens_read();
  const double temp           = (temp_farenheit - 32) / 1.8;
  Homie.getLogger() << "  • Temperature=" << temp << cUnitTemperature << endl;
  ctrlTemperatureNode.setProperty(cTemperature).send(String(temp, 1));

  Homie.getLogger() << "  onTickerTemperatureCtrl <-" << endl;
}

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
    solarPumpNode.setProperty(cStatus).send("reveived invalid value: " + value);

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
    solarPumpNode.setProperty(cStatus).send("ok");

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

  solarTemperatureNode.advertise(cTemperature)
      .setName(cTemperature)
      .setDatatype(cDataTypFloat)
      .setFormat("-50:50")
      .setUnit(cUnitTemperature);
  solarTemperatureNode.advertise(cStatus).setName(cStatusName);

  poolTemperatureNode.advertise(cTemperature)
      .setName(cTemperature)
      .setDatatype(cDataTypFloat)
      .setFormat("-50:100")
      .setUnit(cUnitTemperature);
  poolTemperatureNode.advertise(cStatus).setName(cStatusName);

  ctrlTemperatureNode.advertise(cTemperature)
      .setName(cTemperature)
      .setDatatype(cDataTypFloat)
      .setFormat("-50:100")
      .setUnit(cUnitTemperature);
  ctrlTemperatureNode.advertise(cStatus).setName(cStatusName);

  poolPumpNode.advertise(cSwitch).setName("Switch").setDatatype(cDataTypBoolean).settable(onPoolPumpSwitchHandler);
  poolPumpNode.advertise(cStatus).setName(cStatusName);

  solarPumpNode.advertise(cSwitch).setName("Switch").setDatatype(cDataTypBoolean).settable(onSolarPumpSwitchHandler);
  solarPumpNode.advertise(cStatus).setName(cStatusName);

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
}

/**
 * Only called when wifi and mqtt are connected.
 */
void loopHandler() {

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

    //Homie.disableLogging();
    Homie_setFirmware("pool-controller", "1.0.0");  // The underscore is not a typo! See Magic bytes
    Homie.setSetupFunction(setupHandler);
    Homie.setLoopFunction(loopHandler);

    Homie.setup();

    //mySwitch.enableTransmit(PIN_RSSWITCH);
    //mySwitch.setRepeatTransmit(10);
    //mySwitch.setPulseLength(350);

    tickerTemperatureSolar.attach(temperaturePublishIntervalSetting.get(), onTickerTemperatureSolar);
    tickerTemperaturePool.attach(temperaturePublishIntervalSetting.get(), onTickerTemperaturePool);
    tickerTemperatureCtrl.attach(temperaturePublishIntervalSetting.get(), onTickerTemperatureCtrl);

    Homie.getLogger() << "✔ Setup ready" << endl;
  }

  /**
 * Main loop of ESP.
 */
  void loop() {

    Homie.loop();

    if (Homie.isConfigured()) {
      // The device is configured, in normal mode

      if (Homie.isConnected()) {

      } else {
        // The device is not connected
      }
    } else {
      // The device is not configured, in either configuration or standalone mode
    }
  }
