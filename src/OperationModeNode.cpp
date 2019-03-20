
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

  setRunLoopDisconnected(true);
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
      return _ruleVec[i];
    }
  }

  //return nullptr;
}

/**
 *
 */
bool OperationModeNode::setMode(String mode) {
  bool retval;

  if (mode.equals(STATUS_AUTO) || mode.equals(STATUS_MANU) || mode.equals(STATUS_BOOST)) {
    _mode  = mode;
    retval = true;

  } else {
    Homie.getLogger() << "UNDEFINED Status. Current unchanged mode: " << _mode << endl;
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

  advertise(cMode).setName(cModeName).setRetained(true).setDatatype("enum").setFormat("manu,auto,boost").settable();

  advertise(cPoolMaxTemp)
      .setName(cPoolMaxTempName)
      .setRetained(true)
      .setDatatype("float")
      .setFormat("0:40")
      .setUnit("°C")
      .settable();

  advertise(cSolarMinTemp)
      .setName(cSolarMinTempName)
      .setRetained(true)
      .setDatatype("float")
      .setFormat("0:100")
      .setUnit("°C")
      .settable();
}

/**
 *
 */
void OperationModeNode::onReadyToOperate() {}
/**
 *
 */
void OperationModeNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();

    Homie.getLogger() << "〽 Sending mode: " << getId() << endl;
    Homie.getLogger() << cIndent << "mode: " << _mode << endl;

    if (Homie.isConnected()) {
      setProperty(cMode).send(_mode);
    }

    //call loop to evaluate the current rule
    getRule()->loop();
  }
}

/**
 * Handle update by Homie message.
 */
bool OperationModeNode::handleInput(const HomieRange& range, const String& property, const String& value) {
  printCaption();

  Homie.getLogger() << cIndent << "〽 handleInput -> property '" << property << "' value=" << value << endl;
  bool retval;

  if (property.equalsIgnoreCase(cMode)) {
    //set operational mode
    retval = this->setMode(string2char(value));

  } else if (property.equalsIgnoreCase(cSolarMinTemp)) {
    //solar min temp

  } else if (property.equalsIgnoreCase(cPoolMaxTemp)) {
    // pool max temp

  } else {
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

char* OperationModeNode::string2char(String comand) {
  if (comand.length() != 0) {
    char* p = const_cast<char*>(comand.c_str());
    return p;
  }
}
