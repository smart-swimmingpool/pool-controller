
#include "OperationModeNode.hpp"
#include "Rule.hpp"

/**
 *
 */
OperationModeNode::OperationModeNode(const char* id, const char* name, const int measurementInterval)
    : HomieNode(id, name, "switch") {

  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;
}

/**
 *
 */
void OperationModeNode::setMode(char* mode) {

  _mode = mode;

  setProperty(cMode).send(_mode);
}

/**
 *
 */
char* OperationModeNode::getMode() {
  return _mode;
}

/**
 *
 */
void OperationModeNode::setup() {

  advertise(cMode).setName(cModeName).setRetained(true).setDatatype("enum").setFormat("manu,auto,boost");
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

    //setProperty(cStatus).send(_state);
  }
}

/**
 * Handle update by Homie message.
 */
bool OperationModeNode::handleInput(const HomieRange& range, const String& property, const String& value) {
  printCaption();

  Homie.getLogger() << cIndent << "〽 handleInput -> property '" << property << "' value=" << value << endl;

  if (value == STATUS_AUTO) {
    Homie.getLogger() << "AUTO" << endl;

  } else if (value == STATUS_MANU) {

    Homie.getLogger() << "MANU" << endl;

  } else if (value == STATUS_BOOST) {
    Homie.getLogger() << "BOOST" << endl;

  } else {
    Homie.getLogger() << "UNDEFINED Status" << endl;

    return false;
  }

  return true;
}
/**
 *
 */
void OperationModeNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}
