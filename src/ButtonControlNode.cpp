/**
 * Direct control node for offline button input.
 *
 * MODE button  : cycles operation modes in normal state; exits settings menu.
 * SELECT button: opens / confirms the settings menu.
 * UP / DOWN    : navigate setting items (nav state) or change value (edit state).
 *
 * Settings menu auto-closes after SETTINGS_TIMEOUT_MS of inactivity.
 */

#include "ButtonControlNode.hpp"
#include "Timer.hpp"

const char* ButtonControlNode::MODES[] = {"auto", "boost", "timer", "manu"};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ButtonControlNode::ButtonControlNode(const char*        id,
                                     const char*        name,
                                     uint8_t            modePin,
                                     OperationModeNode* operationModeNode,
                                     LocalDisplayNode*  displayNode,
                                     uint8_t            upPin,
                                     uint8_t            downPin,
                                     uint8_t            selectPin)
    : HomieNode(id, name, "button"),
      _modePin(modePin),
      _upPin(upPin),
      _downPin(downPin),
      _selectPin(selectPin),
      _hasExtButtons(upPin != 0xff && downPin != 0xff && selectPin != 0xff),
      _operationModeNode(operationModeNode),
      _displayNode(displayNode),
      _currentModeIndex(0),
      _state(STATE_MODE_CYCLING),
      _settingsItem(LocalSettingItem::POOL_MAX_TEMP),
      _editValue(0.0f),
      _lastButtonMillis(0) {
  setRunLoopDisconnected(true);
}

// ---------------------------------------------------------------------------
// Homie callbacks
// ---------------------------------------------------------------------------
void ButtonControlNode::setup() {
  _modeDebouncer.attach(_modePin, INPUT_PULLUP);
  _modeDebouncer.interval(50);

  if (_hasExtButtons) {
    _upDebouncer.attach(_upPin, INPUT_PULLUP);
    _upDebouncer.interval(50);

    _downDebouncer.attach(_downPin, INPUT_PULLUP);
    _downDebouncer.interval(50);

    _selectDebouncer.attach(_selectPin, INPUT_PULLUP);
    _selectDebouncer.interval(50);
  }

  _currentModeIndex = findModeIndex(_operationModeNode->getMode());
  _lastButtonMillis = millis();

  Homie.getLogger() << cCaption << F(" modePin=") << _modePin;
  if (_hasExtButtons) {
    Homie.getLogger() << F(", up=") << _upPin
                      << F(", dn=") << _downPin
                      << F(", sel=") << _selectPin;
  }
  Homie.getLogger() << endl;
}

void ButtonControlNode::loop() {
  _modeDebouncer.update();
  if (_hasExtButtons) {
    _upDebouncer.update();
    _downDebouncer.update();
    _selectDebouncer.update();
  }

  // Auto-close settings menu after idle timeout.
  // Unsigned subtraction handles millis() wraparound correctly: (millis() - _lastButtonMillis)
  // gives the correct elapsed time even after the ~49-day wraparound because unsigned
  // arithmetic wraps modulo 2^32.
  if (_state != STATE_MODE_CYCLING && _hasExtButtons) {
    if (millis() - _lastButtonMillis >= SETTINGS_TIMEOUT_MS) {
      exitSettingsMode();
      return;
    }
  }

  switch (_state) {
    case STATE_MODE_CYCLING: handleModeCycling(); break;
    case STATE_SETTINGS_NAV: handleSettingsNav(); break;
    case STATE_SETTINGS_EDIT: handleSettingsEdit(); break;
  }
}

// ---------------------------------------------------------------------------
// State machine handlers
// ---------------------------------------------------------------------------
void ButtonControlNode::handleModeCycling() {
  if (_modeDebouncer.fell()) {
    _lastButtonMillis = millis();
    cycleMode();
  }
  if (_hasExtButtons && _selectDebouncer.fell()) {
    _lastButtonMillis = millis();
    enterSettingsMode();
  }
}

void ButtonControlNode::handleSettingsNav() {
  if (_modeDebouncer.fell()) {
    _lastButtonMillis = millis();
    exitSettingsMode();
    return;
  }
  if (_upDebouncer.fell()) {
    _lastButtonMillis = millis();
    // Move to previous item (wraps)
    uint8_t cnt   = static_cast<uint8_t>(LocalSettingItem::COUNT);
    uint8_t idx   = static_cast<uint8_t>(_settingsItem);
    _settingsItem = static_cast<LocalSettingItem>((idx + cnt - 1) % cnt);
    notifyDisplay();
  }
  if (_downDebouncer.fell()) {
    _lastButtonMillis = millis();
    // Move to next item (wraps)
    uint8_t cnt   = static_cast<uint8_t>(LocalSettingItem::COUNT);
    uint8_t idx   = static_cast<uint8_t>(_settingsItem);
    _settingsItem = static_cast<LocalSettingItem>((idx + 1) % cnt);
    notifyDisplay();
  }
  if (_selectDebouncer.fell()) {
    _lastButtonMillis = millis();
    enterEditMode();
  }
}

void ButtonControlNode::handleSettingsEdit() {
  if (_modeDebouncer.fell()) {
    _lastButtonMillis = millis();
    // Cancel edit without saving; return to nav
    _state = STATE_SETTINGS_NAV;
    notifyDisplay();
    return;
  }
  if (_upDebouncer.fell()) {
    _lastButtonMillis = millis();
    float max  = getSettingMax(_settingsItem);
    float step = getSettingStep(_settingsItem);
    _editValue = (_editValue + step > max) ? max : _editValue + step;
    notifyDisplay();
  }
  if (_downDebouncer.fell()) {
    _lastButtonMillis = millis();
    float min  = getSettingMin(_settingsItem);
    float step = getSettingStep(_settingsItem);
    _editValue = (_editValue - step < min) ? min : _editValue - step;
    notifyDisplay();
  }
  if (_selectDebouncer.fell()) {
    _lastButtonMillis = millis();
    confirmEdit();
  }
}

// ---------------------------------------------------------------------------
// State transitions
// ---------------------------------------------------------------------------
void ButtonControlNode::enterSettingsMode() {
  _state        = STATE_SETTINGS_NAV;
  _settingsItem = LocalSettingItem::POOL_MAX_TEMP;
  Homie.getLogger() << cIndent << F("Settings menu opened") << endl;
  notifyDisplay();
}

void ButtonControlNode::exitSettingsMode() {
  _state            = STATE_MODE_CYCLING;
  _lastButtonMillis = millis();  // reset so a stale timeout cannot fire again
  Homie.getLogger() << cIndent << F("Settings menu closed") << endl;
  if (_displayNode != nullptr) {
    _displayNode->showStatus();
  }
}

void ButtonControlNode::enterEditMode() {
  _editValue = readSettingValue(_settingsItem);
  _state     = STATE_SETTINGS_EDIT;
  Homie.getLogger() << cIndent << F("Editing setting ") << static_cast<uint8_t>(_settingsItem)
                    << F(" value=") << _editValue << endl;
  notifyDisplay();
}

void ButtonControlNode::confirmEdit() {
  applySettingValue(_settingsItem, _editValue);
  Homie.getLogger() << cIndent << F("Saved setting ") << static_cast<uint8_t>(_settingsItem)
                    << F(" = ") << _editValue << endl;
  _state = STATE_SETTINGS_NAV;
  notifyDisplay();
}

void ButtonControlNode::notifyDisplay() {
  if (_displayNode == nullptr) return;
  if (_state == STATE_SETTINGS_NAV) {
    _displayNode->showSettings(_settingsItem, readSettingValue(_settingsItem), false);
  } else if (_state == STATE_SETTINGS_EDIT) {
    _displayNode->showSettings(_settingsItem, _editValue, true);
  }
}

// ---------------------------------------------------------------------------
// Mode cycling
// ---------------------------------------------------------------------------
int ButtonControlNode::findModeIndex(const String& mode) const {
  for (int i = 0; i < MODE_COUNT; i++) {
    if (mode.equals(MODES[i])) return i;
  }
  return 0;
}

void ButtonControlNode::cycleMode() {
  _currentModeIndex   = (_currentModeIndex + 1) % MODE_COUNT;
  const char* newMode = MODES[_currentModeIndex];
  Homie.getLogger() << cIndent << F("Button: mode -> ") << newMode << endl;
  _operationModeNode->setMode(newMode);
}

// ---------------------------------------------------------------------------
// Settings helpers
// ---------------------------------------------------------------------------
void ButtonControlNode::applySettingValue(LocalSettingItem item, float value) {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:
      _operationModeNode->setPoolMaxTemperature(value);
      break;
    case LocalSettingItem::SOLAR_MIN_TEMP:
      _operationModeNode->setSolarMinTemperature(value);
      break;
    case LocalSettingItem::HYSTERESIS:
      _operationModeNode->setTemperatureHysteresis(value);
      break;
    case LocalSettingItem::TIMER_START_H: {
      TimerSetting ts   = _operationModeNode->getTimerSetting();
      unsigned int uval = (value < 0.0f) ? 0u : static_cast<unsigned int>(value);
      ts.timerStartHour = (uval > 23u) ? 23u : uval;
      _operationModeNode->setTimerSetting(ts);
      break;
    }
    case LocalSettingItem::TIMER_START_MIN: {
      TimerSetting ts      = _operationModeNode->getTimerSetting();
      unsigned int uval    = (value < 0.0f) ? 0u : static_cast<unsigned int>(value);
      ts.timerStartMinutes = (uval > 59u) ? 59u : uval;
      _operationModeNode->setTimerSetting(ts);
      break;
    }
    case LocalSettingItem::TIMER_END_H: {
      TimerSetting ts = _operationModeNode->getTimerSetting();
      unsigned int uval = (value < 0.0f) ? 0u : static_cast<unsigned int>(value);
      ts.timerEndHour = (uval > 23u) ? 23u : uval;
      _operationModeNode->setTimerSetting(ts);
      break;
    }
    case LocalSettingItem::TIMER_END_MIN: {
      TimerSetting ts    = _operationModeNode->getTimerSetting();
      unsigned int uval  = (value < 0.0f) ? 0u : static_cast<unsigned int>(value);
      ts.timerEndMinutes = (uval > 59u) ? 59u : uval;
      _operationModeNode->setTimerSetting(ts);
      break;
    }
    default:
      break;
  }
}

float ButtonControlNode::readSettingValue(LocalSettingItem item) const {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:
      return _operationModeNode->getPoolMaxTemperature();
    case LocalSettingItem::SOLAR_MIN_TEMP:
      return _operationModeNode->getSolarMinTemperature();
    case LocalSettingItem::HYSTERESIS:
      return _operationModeNode->getTemperatureHysteresis();
    case LocalSettingItem::TIMER_START_H:
      return static_cast<float>(_operationModeNode->getTimerSetting().timerStartHour);
    case LocalSettingItem::TIMER_START_MIN:
      return static_cast<float>(_operationModeNode->getTimerSetting().timerStartMinutes);
    case LocalSettingItem::TIMER_END_H:
      return static_cast<float>(_operationModeNode->getTimerSetting().timerEndHour);
    case LocalSettingItem::TIMER_END_MIN:
      return static_cast<float>(_operationModeNode->getTimerSetting().timerEndMinutes);
    default:
      return 0.0f;
  }
}

float ButtonControlNode::getSettingStep(LocalSettingItem item) {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:   return 0.5f;
    case LocalSettingItem::SOLAR_MIN_TEMP:  return 0.5f;
    case LocalSettingItem::HYSTERESIS:      return 0.1f;
    case LocalSettingItem::TIMER_START_H:   return 1.0f;
    case LocalSettingItem::TIMER_START_MIN: return 5.0f;
    case LocalSettingItem::TIMER_END_H:     return 1.0f;
    case LocalSettingItem::TIMER_END_MIN:   return 5.0f;
    default:                                return 1.0f;
  }
}

float ButtonControlNode::getSettingMin(LocalSettingItem item) {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:   return 0.0f;
    case LocalSettingItem::SOLAR_MIN_TEMP:  return 0.0f;
    case LocalSettingItem::HYSTERESIS:      return 0.0f;
    case LocalSettingItem::TIMER_START_H:   return 0.0f;
    case LocalSettingItem::TIMER_START_MIN: return 0.0f;
    case LocalSettingItem::TIMER_END_H:     return 0.0f;
    case LocalSettingItem::TIMER_END_MIN:   return 0.0f;
    default:                                return 0.0f;
  }
}

float ButtonControlNode::getSettingMax(LocalSettingItem item) {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:   return 40.0f;
    case LocalSettingItem::SOLAR_MIN_TEMP:  return 100.0f;
    case LocalSettingItem::HYSTERESIS:      return 10.0f;
    case LocalSettingItem::TIMER_START_H:   return 23.0f;
    case LocalSettingItem::TIMER_START_MIN: return 55.0f;
    case LocalSettingItem::TIMER_END_H:     return 23.0f;
    case LocalSettingItem::TIMER_END_MIN:   return 55.0f;
    default:                                return 100.0f;
  }
}
