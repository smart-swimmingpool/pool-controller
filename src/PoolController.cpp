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
#include "StateManager.hpp"
#include "SystemMonitor.hpp"

#include "Config.hpp"

namespace PoolController {
static LoggerNode            LN;
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

static PoolControllerContext* Self;
auto                          Detail::setupProxy() -> void {
  Self->setupHandler();
}

PoolControllerContext::PoolControllerContext() {
  assert(!Self);
  Self = this;
}

PoolControllerContext::~PoolControllerContext() {
  assert(Self);
  Self = nullptr;
}

/**
     * Homie Setup handler.
     * Only called when wifi and mqtt are connected.
     */
auto PoolControllerContext::setupHandler() -> void {

  // Initialize state management
  StateManager::begin();

  // Initialize system monitor and watchdog
  SystemMonitor::begin();

  // set mesurement intervals
  const std::uint32_t _loopInterval = this->loopIntervalSetting_.get();

  solarTemperatureNode.setMeasurementInterval(_loopInterval);
  poolTemperatureNode.setMeasurementInterval(_loopInterval);

  poolPumpNode.setMeasurementInterval(_loopInterval);
  solarPumpNode.setMeasurementInterval(_loopInterval);

#ifdef ESP32
  ctrlTemperatureNode.setMeasurementInterval(_loopInterval);
#endif

  // Load persisted state first, then override with config if different
  operationModeNode.loadState();

  // Apply configuration settings (these will override persisted state if different)
  operationModeNode.setMode(this->operationModeSetting_.get());
  operationModeNode.setPoolMaxTemperature(this->temperatureMaxPoolSetting_.get());
  operationModeNode.setSolarMinTemperature(this->temperatureMinSolarSetting_.get());
  operationModeNode.setTemperatureHysteresis(this->temperatureHysteresisSetting_.get());

  // Timer settings are now loaded from state, but can be overridden here if needed
  // TimerSetting ts = operationModeNode.getTimerSetting();
  // ts.timerStartHour    = 10;
  // ts.timerStartMinutes = 30;
  // ts.timerEndHour      = 17;
  // ts.timerEndMinutes   = 30;
  // operationModeNode.setTimerSetting(ts);

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

  LN.log(__PRETTY_FUNCTION__, LoggerNode::INFO, "State persistence and system monitoring initialized");
}

auto PoolControllerContext::setup() -> void {
  Homie.setLoggingPrinter(&Serial);

  Homie_setFirmware("pool-controller", "3.1.0");
  Homie_setBrand("smart-swimmingpool");

  //default intervall of sending Temperature values
  this->loopIntervalSetting_.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](const long candidate) -> bool {
    return candidate >= 0 && candidate <= 300;
  });

  this->temperatureMaxPoolSetting_.setDefaultValue(28.5).setValidator(
      [](const long candidate) -> bool { return candidate >= 0 && candidate <= 30; });

  this->temperatureMinSolarSetting_.setDefaultValue(55.0).setValidator(
      [](const long candidate) noexcept -> bool { return candidate >= 0 && candidate <= 100; });

  this->temperatureHysteresisSetting_.setDefaultValue(1.0).setValidator(
      [](const long candidate) -> bool { return candidate >= 0 && candidate <= 10; });

  this->operationModeSetting_.setDefaultValue("auto").setValidator([](const char* const candidate) -> bool {
    return std::strcmp(candidate, "auto") == 0 || std::strcmp(candidate, "manu") == 0 || std::strcmp(candidate, "boost") == 0;
  });

  this->mqttProtocolSetting_.setDefaultValue("homie").setValidator([](const char* const candidate) -> bool {
    return std::strcmp(candidate, "homie") == 0 || std::strcmp(candidate, "homeassistant") == 0;
  });

  Homie.setSetupFunction(&Detail::setupProxy);

  LN.log(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Before Homie setup())");
  Homie.setup();

  LN.logf(__PRETTY_FUNCTION__, LoggerNode::DEBUG, "Free heap: %d", ESP.getFreeHeap());
  Homie.getLogger() << F("Free heap: ") << ESP.getFreeHeap() << endl;
}

auto PoolControllerContext::loop() -> void {
  // Feed watchdog and check memory
  SystemMonitor::feedWatchdog();
  SystemMonitor::checkMemory();

  Homie.loop();
}
}  // namespace PoolController
