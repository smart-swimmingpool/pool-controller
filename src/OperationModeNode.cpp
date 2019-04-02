
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
      _ruleVec[i]->setPoolMaxTemperatur(getPoolMaxTemperature());
      _ruleVec[i]->setSolarMinTemperature(getSolarMinTemperature());
      _ruleVec[i]->setTemperaturHysteresis(getTemperaturHysteresis());

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
    _mode  = mode;
    setProperty(cMode).send(_mode);
    setProperty(cState).send(F(""));
    retval = true;

  } else {
    Homie.getLogger() << F("UNDEFINED Mode. Current unchanged mode: ") << _mode << endl;
    setProperty(cState).send(F("Undefined mode."));
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

  advertise(cState).setName(cStateName);
  advertise(cMode).setName(cModeName).setDatatype("enum").setFormat("manu,auto,boost").settable();
  advertise(cPoolMaxTemp).setName(cPoolMaxTempName).setDatatype("float").setFormat("0:40").setUnit("°C").settable();
  advertise(cSolarMinTemp).setName(cSolarMinTempName).setDatatype("float").setFormat("0:100").setUnit("°C").settable();
  advertise(cHysteresis).setName(cHysteresisName).setDatatype("float").setFormat("0:10").setUnit("K").settable();
}

/**
 *
 */
void OperationModeNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {

    Homie.getLogger() << F("〽 Sending mode: ") << getId() << endl;
    Homie.getLogger() << cIndent << "mode: " << _mode << endl;
    Homie.getLogger() << cIndent << "SolarMinTemp: " << _solarMinTemp << endl;
    Homie.getLogger() << cIndent << "PoolMaxTemp:  " << _poolMaxTemp << endl;
    Homie.getLogger() << cIndent << "Hysteresis:   " << _hysteresis << endl;

    if (Homie.isConnected()) {
      setProperty(cMode).send(_mode);
      setProperty(cSolarMinTemp).send(String(_solarMinTemp));
      setProperty(cPoolMaxTemp).send(String(_poolMaxTemp));
      setProperty(cHysteresis).send(String(_hysteresis));
    } else {
      Homie.getLogger() << F("✖ not connected.") << endl;
    }

    //call loop to evaluate the current rule
    getRule()->loop();

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
    Homie.getLogger() << cIndent << F("✔ set operational mode") << endl;
    retval = this->setMode(value);

  } else if (property.equalsIgnoreCase(cSolarMinTemp)) {
    Homie.getLogger() << cIndent << F("✔ solar min temp") << endl;
    _solarMinTemp = value.toFloat();
    retval        = true;

  } else if (property.equalsIgnoreCase(cPoolMaxTemp)) {
    Homie.getLogger() << cIndent << F("✔ pool max temp");
    _poolMaxTemp = value.toFloat();
    retval       = true;

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

