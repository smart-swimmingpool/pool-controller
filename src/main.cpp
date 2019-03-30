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
#include <time.h>
#include <simpleDSTadjust.h>

#include "ConstantValues.hpp"

#include "DallasTemperatureNode.hpp"
#include "ESP32TemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "RCSwitchNode.hpp"
#include "OperationModeNode.hpp"
#include "Rule.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"
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
const uint8_t PIN_DHT11    = D7;

const uint8_t PIN_RELAY_POOL  = D1;
const uint8_t PIN_RELAY_SOLAR = D2;
#endif
const uint8_t TEMP_READ_INTERVALL = 60;  //Sekunden zwischen Updates der Temperaturen.

struct dstRule StartRule = {"MESZ", Last, Sun, Mar, 1, 3600};  // Daylight time = UTC/GMT -4 hours
struct dstRule EndRule   = {"MEZ", Last, Sun, Oct, 1, 0};      // Standard time = UTC/GMT -5 hour
// Setup simpleDSTadjust Library rules
simpleDSTadjust dstAdjusted(StartRule, EndRule);

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

/**
 *
 */
tm* getDateTime(time_t offset) {

  char*      dstAbbrev;
  time_t     t        = dstAdjusted.time(&dstAbbrev) + offset;
  struct tm* timeinfo = localtime(&t);

  return timeinfo;
}

void printTime(time_t offset) {
  struct tm* timeinfo = getDateTime(offset);

  int  hour = (timeinfo->tm_hour + 11) % 12 + 1;  // take care of noon and midnight
  char buf[30];
  //sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d%s %s \n",timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_year+1900, hour, timeinfo->tm_min, timeinfo->tm_sec, timeinfo->tm_hour>=12?"pm":"am" /*, dstAbbrev*/);
  sprintf(buf, "%02d/%02d/%04d %02d:%02d:%02d%s \n", timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year + 1900, hour,
          timeinfo->tm_min, timeinfo->tm_sec, timeinfo->tm_hour >= 12 ? "pm" : "am");
  Homie.getLogger() << buf << endl;
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

  String mode = String(operationModeSetting.get());
  operationModeNode.setMode(mode);
  operationModeNode.setPoolMaxTemp(temperatureMaxPoolSetting.get());
  operationModeNode.setSolarMinTemp(temperatureMinSolarSetting.get());

  configTime(1 * 3600, 0 * 3600, "europe.pool.ntp.org", "time.nist.gov");
  //Waiting for time:
  while (!time(nullptr)) {
    delay(1000);
  }

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

  Serial.println(F("-------------------------------------"));
  Serial.println(F(" Pool Controller                     "));
  Serial.println(F("-------------------------------------"));

  Homie_setFirmware("pool-controller", "1.0.0");  // The underscore is not a typo! See Magic bytes
  Homie_setBrand("smart-swimmingpool");

  //default intervall of sending Temperature values
  loopIntervalSetting.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](long candidate) {
    return (candidate >= 0) && (candidate <= 300);
  });

  temperatureMaxPoolSetting.setDefaultValue(28.5).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 30); });

  temperatureMinSolarSetting.setDefaultValue(50.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 100); });

  temperatureHysteresisSetting.setDefaultValue(1.0).setValidator(
      [](long candidate) { return (candidate >= 0) && (candidate <= 10); });

  operationModeSetting.setDefaultValue("auto").setValidator([](const char* candidate) {
    return (strcmp(candidate, "auto")) || (strcmp(candidate, "manu")) || (strcmp(candidate, "boost"));
  });
  // set default configured OperationMode
  String mode = operationModeSetting.get();
  operationModeNode.setMode((char*)mode.c_str());

  // add the rules
  RuleAuto* autoRule = new RuleAuto(&solarPumpNode, &poolPumpNode);
  operationModeNode.addRule(autoRule);

  RuleManu* manuRule = new RuleManu();
  operationModeNode.addRule(manuRule);

  RuleBoost* boostRule = new RuleBoost(&solarPumpNode, &poolPumpNode);
  boostRule->setPoolMaxTemperatur(temperatureMaxPoolSetting.get());
  boostRule->setSolarMinTemperature(temperatureMinSolarSetting.get());  // TODO make changeable

  operationModeNode.addRule(boostRule);

  //Homie.disableLogging();
  Homie.setSetupFunction(setupHandler);
  Homie.setup();
}

/**
 * Main loop of ESP.
 */
void loop() {

  Homie.loop();
  /*
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();
    if (Homie.isConnected()) {

/*
      time_t now = time(nullptr);
      Serial.println(ctime(&now));
      printTime(0);
      */
    }

  }
  */
}
