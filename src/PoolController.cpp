#include "PoolController.hpp"

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

#include "Config.hpp"

namespace PoolController
{
    static HomieSetting<long> loopIntervalSetting("loop-interval", "The processing interval in seconds");
    static HomieSetting<double> temperatureMaxPoolSetting("temperature-max-pool", "Maximum temperature of solar");
    static HomieSetting<double> temperatureMinSolarSetting("temperature-min-solar", "Minimum temperature of solar");
    static HomieSetting<double> temperatureHysteresisSetting("temperature-hysteresis", "Temperature hysteresis");
    static HomieSetting<const char*> operationModeSetting("operation-mode", "Operational Mode");
    static  LoggerNode LN;
    static DallasTemperatureNode solarTemperatureNode("solar-temp", "Solar Temperature", PIN_DS_SOLAR, TEMP_READ_INTERVALL);
    static DallasTemperatureNode poolTemperatureNode("pool-temp", "Pool Temperature", PIN_DS_POOL, TEMP_READ_INTERVALL);
    #ifdef ESP32
        static ESP32TemperatureNode ctrlTemperatureNode("controller-temp", "Controller Temperature", TEMP_READ_INTERVALL);
    #endif
    static RelayModuleNode poolPumpNode("pool-pump", "Pool Pump", PIN_RELAY_POOL);
    static RelayModuleNode solarPumpNode("solar-pump", "Solar Pump", PIN_RELAY_SOLAR);

    static OperationModeNode operationModeNode("operation-mode", "Operation Mode");

    static unsigned long _measurementInterval = 10;
    static unsigned long _lastMeasurement;

    /**
     * Homie Setup handler.
     * Only called when wifi and mqtt are connected.
     */
    static void setupHandler() {

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

    auto PoolControllerContext::startup() -> void
    {
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
        Homie.setSetupFunction(&setupHandler);

        LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
        Homie.setup();

        LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Free heap: %d", ESP.getFreeHeap());
        Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
    }

    auto PoolControllerContext::loop() -> void
    {
        Homie.loop();
    }
}
