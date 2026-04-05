/**
 * Direct control node for offline button input.
 *
 * Allows cycling through operation modes using a physical push-button,
 * so the pool controller can be operated without internet/MQTT connectivity.
 */

#pragma once

#include <Bounce2.h>
#include <Homie.hpp>
#include "OperationModeNode.hpp"

class ButtonControlNode : public HomieNode {

public:
  ButtonControlNode(const char* id, const char* name, uint8_t pin, OperationModeNode* operationModeNode);

protected:
  void setup() override;
  void loop() override;

private:
  uint8_t            _pin;
  Bounce             _debouncer;
  OperationModeNode* _operationModeNode;
  int                _currentModeIndex;

  // Mode cycling order: auto → boost → timer → manu → auto
  static const char* MODES[];
  static const int   MODE_COUNT = 4;

  const char* cCaption = "• Button Control:";
  const char* cIndent  = "  ◦ ";

  int  findModeIndex(const String& mode) const;
  void cycleMode();
};
