
#include "OperationModeNode.hpp"
#include "RuleManu.hpp"
#include "RuleAuto.hpp"
#include "RuleBoost.hpp"

/**
 *
 */
OperationModeNode::OperationModeNode(const char* id, const char* name, const int measurementInterval)
    : HomieNode(id, name, "switch") {

  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;

  //setRunLoopDisconnected(true);
}

/**
 *
 */
void OperationModeNode::addRule(Rule* rule) {
  _ruleVec.PushBack(rule);
}

/**
 *
 */
Rule* OperationModeNode::getRule() {

  for (int i = 0; i < _ruleVec.Size(); i++) {
    if (_mode.equals(_ruleVec[i]->getMode())) {
      //update the properties
      _ruleVec[i]->setPoolMaxTemperatur(getPoolMaxTemperature());
      _ruleVec[i]->setSolarMinTemperature(getSolarMinTemperature());
      _ruleVec[i]->setTemperaturHysteresis(getTemperaturHysteresis());
      _ruleVec[i]->setTimerSetting(getTimerSetting()) ;

      return _ruleVec[i];
    }
  }

  return nullptr;
}

/**
 *
 */
bool OperationModeNode::setMode(String mode) {
  bool retval;

  if (mode.equals(STATUS_AUTO) || mode.equals(STATUS_MANU) || mode.equals(STATUS_BOOST)) {
    _mode = mode;
    setProperty(cMode).send(_mode);
    setProperty(cHomieNodeState).send(cHomieNodeState_OK);
    retval = true;

  } else {
    Homie.getLogger() << F("✖ UNDEFINED Mode. Current unchanged mode: ") << _mode << endl;
    setProperty(cHomieNodeState).send(cHomieNodeState_Error);
    retval = false;
  }

  return retval;
}

/**
 *
 */
String OperationModeNode::getMode() {
  return _mode;
}

/**
 *
 */
void OperationModeNode::setup() {

  advertise(cHomieNodeState).setName(cHomieNodeStateName);
  advertise(cMode).setName(cModeName).setDatatype("enum").setFormat("manu,auto,boost").settable();
  advertise(cPoolMaxTemp).setName(cPoolMaxTempName).setDatatype("float").setFormat("0:40").setUnit("°C").settable();
  advertise(cSolarMinTemp).setName(cSolarMinTempName).setDatatype("float").setFormat("0:100").setUnit("°C").settable();
  advertise(cHysteresis).setName(cHysteresisName).setDatatype("float").setFormat("0:10").setUnit("K").settable();

  advertise(cTimerStartHour).setName("Timer Start").setDatatype("float").setFormat("0:23").setUnit("hh").settable();
  advertise(cTimerStartMin).setName("Timer Start").setDatatype("float").setFormat("0:59").setUnit("MM").settable();

  advertise(cTimerEndHour).setName("Timer End").setDatatype("float").setFormat("0:23").setUnit("hh").settable();
  advertise(cTimerEndMin).setName("Timer End").setDatatype("float").setFormat("0:59").setUnit("MM").settable();
}

/**
 *
 */
void OperationModeNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << F("〽 OperatioalMode update rule ") << endl;
    //call loop to evaluate the current rule
    getRule()->loop();

    if (Homie.isConnected()) {
/*

      Homie.getLogger() << cIndent << F("mode: ") << _mode << endl;
      Homie.getLogger() << cIndent << F("SolarMinTemp: ") << _solarMinTemp << endl;
      Homie.getLogger() << cIndent << F("PoolMaxTemp:  ") << _poolMaxTemp << endl;
      Homie.getLogger() << cIndent << F("Hysteresis:   ") << _hysteresis << endl;
*/
      setProperty(cMode).send(_mode);
      setProperty(cSolarMinTemp).send(String(_solarMinTemp));
      setProperty(cPoolMaxTemp).send(String(_poolMaxTemp));
      setProperty(cHysteresis).send(String(_hysteresis));

      setProperty(cTimerStartHour).send(String(_timerSetting.timerStartHour));
      setProperty(cTimerStartMin).send(String(_timerSetting.timerStartMinutes));

      setProperty(cTimerEndHour).send(String(_timerSetting.timerEndHour));
      setProperty(cTimerEndMin).send(String(_timerSetting.timerEndMinutes));
    } else {
      Homie.getLogger() << F("✖ OperationalMode: not connected.") << endl;
    }

    _lastMeasurement = millis();
  }
}

/**
 * Handle update by Homie message.
 */
bool OperationModeNode::handleInput(const HomieRange& range, const String& property, const String& value) {
  printCaption();

  Homie.getLogger() << cIndent << F("〽 handleInput -> property '") << property << F("' value=") << value << endl;
  bool retval;

  if (property.equalsIgnoreCase(cMode)) {
    Homie.getLogger() << cIndent << F("✔ set operational mode: ") << value << endl;
    retval = this->setMode(value);

  } else if (property.equalsIgnoreCase(cSolarMinTemp)) {
    Homie.getLogger() << cIndent << F("✔ solar min temp: ") << value << endl;
    _solarMinTemp = value.toFloat();
    retval        = true;

  } else if (property.equalsIgnoreCase(cPoolMaxTemp)) {
    Homie.getLogger() << cIndent << F("✔ pool max temp: ") << value << endl;
    _poolMaxTemp = value.toFloat();
    retval       = true;

  } else if (property.equalsIgnoreCase(cTimerStartHour)) {
    Homie.getLogger() << cIndent << F("✔ Timer start hh: ") << value << endl;
    _timerSetting.timerStartHour = value.toInt();
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerStartMin)) {
    Homie.getLogger() << cIndent << F("✔  Timer start min.: ") << value << endl;
    _timerSetting.timerStartMinutes = value.toInt();
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEndHour)) {
    Homie.getLogger() << cIndent << F("✔ Timer end h: ") << value << endl;
    _timerSetting.timerEndHour = value.toInt();
    retval = true;

  } else if (property.equalsIgnoreCase(cTimerEndMin)) {
    Homie.getLogger() << cIndent << F("✔ Timer end min.: ") << value << endl;
    _timerSetting.timerEndMinutes = value.toInt();
    retval = true;

  } else {
    Homie.getLogger() << cIndent << F("✖ unmanaged property: ") << property << endl;
    retval = false;

  }

  return retval;
}

/**
 *
 */
void OperationModeNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}
