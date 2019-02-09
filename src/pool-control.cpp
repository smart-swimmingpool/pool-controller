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


//GPIO12 & GPIO14 -> Temperatur
OneWire dsSolar(PIN_DS_SOLAR);
OneWire dsPool(PIN_DS_POOL);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensorSolar(&dsSolar);
DallasTemperature sensorPool(&dsPool);

HomieNode solarTemperatureNode("solarTemp", "Solar Temperature", "temperature");
HomieNode poolTemperatureNode("poolTemp", "Pool Temperature", "temperature");
HomieNode ctrlTemperatureNode("controllerTemp", "Controller Temperature", "temperature");

HomieNode poolPumpNode("poolPump", "Pool Pump", "switch");
HomieNode solarPumpNode("solarPump", "Solar Pump", "switch");

HomieSetting<long> temperaturePublishIntervalSetting("temperaturePublishInterval", "The temperature publish interval in seconds");

HomieSetting<long> temperatureMaxPoolSetting("temperatureMaxPoolSetting", "Maximum temperature of solar");
HomieSetting<long> temperatureMinSolarSetting("temperatureMinSolarSetting", "Minimum temperature of solar");
HomieSetting<long> temperatureHysteresisSetting("temperatureHysteresisSetting", "Temperature hysteresis");

//RS Switches via 433MHz
RCSwitch mySwitch = RCSwitch();


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
    Homie.getLogger() << "  • Temperature=" << temp << "°C" << endl;
    solarTemperatureNode.setProperty("degrees").send(String(temp, 1));
    solarTemperatureNode.setProperty("status").send("");
  } else {
    Homie.getLogger() << "  ✖ Error reading sensor" << endl;
    solarTemperatureNode.setProperty("degrees").send("");
    solarTemperatureNode.setProperty("status").send("Error reading sensor");
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
    Homie.getLogger() << "  • Temperature=" << temp << "°C" << endl;
    poolTemperatureNode.setProperty("degrees").send(String(temp, 1));
    poolTemperatureNode.setProperty("status").send("");
  } else {
    Homie.getLogger() << "  ✖ Error reading sensor" << endl;
    poolTemperatureNode.setProperty("degrees").send("");
    poolTemperatureNode.setProperty("status").send("Error reading sensor");
  }

  Homie.getLogger() << "  onTickerTemperaturePool <-" << endl;
}

/**
 * Ticker to publish internal temperature of ESP32.
 */
void onTickerTemperatureCtrl() {
  Homie.getLogger() << "〽 onTickerTemperatureCtrl ->" << endl;

  //internal temp of ESP
  uint8_t temp_farenheit = temprature_sens_read();
  const double  temp   = (temp_farenheit - 32) / 1.8;
  Homie.getLogger() << "  • Temperature=" << temp << "°C" << endl;
  ctrlTemperatureNode.setProperty("degrees").send(String(temp, 1));

  Homie.getLogger() << "  onTickerTemperatureCtrl <-" << endl;
}

/**
 * Handler for switching pool Pump on/off.
 */
bool onPoolPumpSwitchHandler(HomieRange range, String value) {
  Homie.getLogger() << "〽 poolPumpSwitchOnHandler -> range=" << range << ", value=" << value << endl;
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

    poolPumpNode.setProperty("on").send(value);
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
bool onSolarPumpSwitchHandler(HomieRange range, String value) {
  Homie.getLogger() << "〽 solarPumpSwitchOnHandler -> range=" << range << ", value=" << value << endl;

  bool retval;

  if (value != "true" && value != "false") {
    retval = false;
  } else {

    if (value == "true") {
      // Switch on
      mySwitch.switchOn("11111", "01000");  //todo: make configurable
    } else {
      mySwitch.switchOff("11111", "01000");  //todo: make configurable
    }

    solarPumpNode.setProperty("on").send(value);
    bool on = (value == "true");
    Homie.getLogger() << "Switch is " << (on ? "on" : "off")  << endl;
    retval = true;
  }

  Homie.getLogger() << "〽 solarPumpSwitchOnHandler <-" << retval << endl;
  return retval;
}

/**
 * Homie Setup handler.
 */
void setupHandler() {

  solarTemperatureNode.advertise("degrees").setDatatype("float").setFormat("-50:50").setUnit("°C");
  solarTemperatureNode.advertise("status");
  poolTemperatureNode.advertise("degrees").setDatatype("float").setFormat("-50:50").setUnit("°C");
  poolTemperatureNode.advertise("status");
  ctrlTemperatureNode.advertise("degrees").setDatatype("float").setFormat("-50:100").setUnit("°C");
  ctrlTemperatureNode.advertise("status");

  poolPumpNode.advertise("switch").setDatatype("boolean").settable(onPoolPumpSwitchHandler);
  solarPumpNode.advertise("switch").setDatatype("boolean").settable(onSolarPumpSwitchHandler);

  //default intervall of sending Temperature values
  temperaturePublishIntervalSetting.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](long candidate) {
    return (candidate >= 0) && (candidate <= 300);
  });

  temperatureMaxPoolSetting.setDefaultValue(28.5).setValidator( [](long candidate) {
    return (candidate >= 0) && (candidate <= 30);
  });

  temperatureMinSolarSetting.setDefaultValue(50.0).setValidator( [](long candidate) {
    return (candidate >= 0) && (candidate <= 100);
  });

  temperatureHysteresisSetting.setDefaultValue(1.0).setValidator( [](long candidate) {
    return (candidate >= 0) && (candidate <= 10);
  });
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
  Homie.setSetupFunction(setupHandler);           //.setLoopFunction(loopHandler);

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
