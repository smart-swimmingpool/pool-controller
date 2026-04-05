/**
 * Direct control node for offline button input.
 *
 * A single MODE button cycles through operation modes without WiFi/MQTT.
 * Optional UP / DOWN / SELECT buttons (recommended on ESP32) open a settings
 * menu that lets the user adjust temperatures, hysteresis, and timer settings
 * entirely offline.
 *
 * All buttons use the internal pull-up; connect each button between its pin
 * and GND (active-low).
 */

#pragma once

#include <Bounce2.h>
#include <Homie.hpp>
#include "LocalDisplayNode.hpp"
#include "OperationModeNode.hpp"

class ButtonControlNode : public HomieNode {

public:
  /**
   * Internal state of the settings menu state machine.
   */
  enum State : uint8_t {
    STATE_MODE_CYCLING,  ///< Normal: MODE button cycles operation modes.
    STATE_SETTINGS_NAV,  ///< Settings menu open; UP/DOWN navigate items.
    STATE_SETTINGS_EDIT  ///< Editing a specific setting value.
  };

  /**
   * @param id / name         Homie node identifiers
   * @param modePin           Pin for mode-cycle button
   * @param operationModeNode OperationModeNode to control
   * @param displayNode       Optional LocalDisplayNode for visual feedback
   *                          (nullptr = no display)
   * @param upPin             Optional UP button pin (0xff = not connected)
   * @param downPin           Optional DOWN button pin (0xff = not connected)
   * @param selectPin         Optional SELECT button pin (0xff = not connected)
   */
  ButtonControlNode(const char*        id,
                    const char*        name,
                    uint8_t            modePin,
                    OperationModeNode* operationModeNode,
                    LocalDisplayNode*  displayNode = nullptr,
                    uint8_t            upPin       = 0xff,
                    uint8_t            downPin     = 0xff,
                    uint8_t            selectPin   = 0xff);

protected:
  void setup() override;
  void loop() override;

private:
  // Mode cycling order: auto → boost → timer → manu → auto
  static const char* MODES[];
  static const int   MODE_COUNT = 4;

  // After this many ms without a button press, the settings menu auto-closes.
  static const unsigned long SETTINGS_TIMEOUT_MS = 30000UL;

  uint8_t _modePin;
  uint8_t _upPin;
  uint8_t _downPin;
  uint8_t _selectPin;
  bool    _hasExtButtons;  ///< true when all of up/down/select are configured

  Bounce _modeDebouncer;
  Bounce _upDebouncer;
  Bounce _downDebouncer;
  Bounce _selectDebouncer;

  OperationModeNode* _operationModeNode;
  LocalDisplayNode*  _displayNode;

  int              _currentModeIndex;
  State            _state;
  LocalSettingItem _settingsItem;
  float            _editValue;
  unsigned long    _lastButtonMillis;

  const char* cCaption = "• Button Control:";
  const char* cIndent  = "  ◦ ";

  int  findModeIndex(const String& mode) const;
  void cycleMode();

  // State machine handlers
  void handleModeCycling();
  void handleSettingsNav();
  void handleSettingsEdit();

  // Transitions
  void enterSettingsMode();
  void exitSettingsMode();
  void enterEditMode();
  void confirmEdit();

  // Settings helpers
  void  applySettingValue(LocalSettingItem item, float value);
  float readSettingValue(LocalSettingItem item) const;
  static float getSettingStep(LocalSettingItem item);
  static float getSettingMin(LocalSettingItem item);
  static float getSettingMax(LocalSettingItem item);

  void notifyDisplay();
};
