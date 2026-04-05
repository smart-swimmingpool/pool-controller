/**
 * Direct control node for offline button input.
 *
 * A single push-button cycles through operation modes
 * (auto → boost → timer → manu → auto), allowing the controller
 * to be operated without any internet or MQTT connection.
 *
 * The button uses the internal pull-up resistor; connect the button
 * between the configured pin and GND (active-low).
 */

#include "ButtonControlNode.hpp"

const char* ButtonControlNode::MODES[]   = {"auto", "boost", "timer", "manu"};

ButtonControlNode::ButtonControlNode(const char* id, const char* name, uint8_t pin,
                                     OperationModeNode* operationModeNode)
    : HomieNode(id, name, "button"), _pin(pin), _operationModeNode(operationModeNode), _currentModeIndex(0) {
  // Run loop even when WiFi / MQTT is not connected so the button
  // works fully offline.
  setRunLoopDisconnected(true);
}

/**
 * Called by the Homie framework during startup (before WiFi connects).
 */
void ButtonControlNode::setup() {
  _debouncer.attach(_pin, INPUT_PULLUP);
  _debouncer.interval(50);  // 50 ms debounce window

  _currentModeIndex = findModeIndex(_operationModeNode->getMode());

  Homie.getLogger() << cCaption << F(" pin=") << _pin << F(", initial mode=") << _operationModeNode->getMode() << endl;
}

/**
 * Polls the button and cycles the operation mode on a falling edge
 * (button press).
 */
void ButtonControlNode::loop() {
  _debouncer.update();

  if (_debouncer.fell()) {
    cycleMode();
  }
}

/**
 * Returns the index of the given mode string in MODES[], or 0 if not found.
 */
int ButtonControlNode::findModeIndex(const String& mode) const {
  for (int i = 0; i < MODE_COUNT; i++) {
    if (mode.equals(MODES[i])) {
      return i;
    }
  }
  return 0;
}

/**
 * Advances to the next mode in the cycle and applies it.
 */
void ButtonControlNode::cycleMode() {
  _currentModeIndex   = (_currentModeIndex + 1) % MODE_COUNT;
  const char* newMode = MODES[_currentModeIndex];
  Homie.getLogger() << cIndent << F("Button pressed: cycling to mode -> ") << newMode << endl;
  _operationModeNode->setMode(newMode);
}
