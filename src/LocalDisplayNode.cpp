/**
 * Local display node implementation.
 *
 * Default display: SSD1306 OLED 128x64, hardware I2C.
 * E-paper alternative: IL3820 (Waveshare 2.9") 296x128, hardware SPI.
 *   Build with -D USE_EPAPER and set DISPLAY_EPAPER_CS / _DC / _RST in
 *   platformio.ini build_flags to your wiring.
 */

#include "LocalDisplayNode.hpp"

#include <U8g2lib.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// Compile-time display type selection
// ---------------------------------------------------------------------------
#ifdef USE_EPAPER
// Waveshare 2.9" (IL3820, 296x128) via hardware SPI.
// Default SPI pins are used; override CS/DC/RST with build flags if needed.
#  ifndef DISPLAY_EPAPER_CS
#    ifdef ESP32
#      define DISPLAY_EPAPER_CS  5
#      define DISPLAY_EPAPER_DC  17
#      define DISPLAY_EPAPER_RST 25
#    else
#      define DISPLAY_EPAPER_CS  D8
#      define DISPLAY_EPAPER_DC  D0
#      define DISPLAY_EPAPER_RST D1
#    endif
#  endif
static U8G2_IL3820_296X128_F_4W_HW_SPI g_display(
    U8G2_R0, DISPLAY_EPAPER_CS, DISPLAY_EPAPER_DC, DISPLAY_EPAPER_RST);
#else
// Default: SSD1306 OLED 128x64, hardware I2C.  I2C pins are configured via
// Wire.begin() in LocalDisplayNode::setup() using the sdaPin/sclPin arguments.
static U8G2_SSD1306_128X64_NONAME_F_HW_I2C g_display(U8G2_R0, /* reset */ U8X8_PIN_NONE);
#endif

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
LocalDisplayNode::LocalDisplayNode(const char*            id,
                                   const char*            name,
                                   OperationModeNode*     opModeNode,
                                   RelayModuleNode*       poolPump,
                                   RelayModuleNode*       solarPump,
                                   DallasTemperatureNode* poolTemp,
                                   DallasTemperatureNode* solarTemp,
                                   uint8_t                sdaPin,
                                   uint8_t                sclPin)
    : HomieNode(id, name, "display"),
      _opModeNode(opModeNode),
      _poolPump(poolPump),
      _solarPump(solarPump),
      _poolTemp(poolTemp),
      _solarTemp(solarTemp),
      _display(&g_display),
      _sdaPin(sdaPin),
      _sclPin(sclPin),
      _needsRedraw(true),
      _inSettingsView(false),
      _settingsItem(LocalSettingItem::POOL_MAX_TEMP),
      _settingsValue(0.0f),
      _editingValue(false),
      _lastUpdate(0) {
  setRunLoopDisconnected(true);
}

// ---------------------------------------------------------------------------
// Homie framework callbacks
// ---------------------------------------------------------------------------
void LocalDisplayNode::setup() {
  if (_sdaPin != 0xff && _sclPin != 0xff) {
    Wire.begin(_sdaPin, _sclPin);
  } else {
    Wire.begin();
  }
  _display->begin();
  _display->setFont(u8g2_font_6x10_tf);
  _display->clearBuffer();
  _display->sendBuffer();

  Homie.getLogger() << F("• LocalDisplay: initialized") << endl;

  // Verify the I2C bus came up by checking the display width.
  // A width of 0 usually indicates wiring / pull-up problems on boot-sensitive pins.
  if (_display->getDisplayWidth() == 0) {
    Homie.getLogger() << F("✖ LocalDisplay: display width=0, check wiring and pull-ups") << endl;
  }
}

void LocalDisplayNode::loop() {
  unsigned long now = millis();
  if (_needsRedraw || (now - _lastUpdate >= REFRESH_INTERVAL_MS)) {
    _needsRedraw = false;
    _lastUpdate  = now;
    if (_inSettingsView) {
      drawSettings();
    } else {
      drawStatus();
    }
  }
}

// ---------------------------------------------------------------------------
// Public API called by ButtonControlNode
// ---------------------------------------------------------------------------
void LocalDisplayNode::showStatus() {
  _inSettingsView = false;
  _needsRedraw    = true;
}

void LocalDisplayNode::showSettings(LocalSettingItem item, float editValue, bool editing) {
  _inSettingsView = true;
  _settingsItem   = item;
  _settingsValue  = editValue;
  _editingValue   = editing;
  _needsRedraw    = true;
}

// ---------------------------------------------------------------------------
// Private rendering
// ---------------------------------------------------------------------------
void LocalDisplayNode::drawStatus() {
  char buf[32];  // 32 bytes: enough for longest formatted row with null terminator

  _display->clearBuffer();
  _display->setFont(u8g2_font_6x10_tf);

  // Row 1: operation mode + WiFi indicator
  // Mode strings are all ≤5 chars (auto/boost/timer/manu) so "%-5s" never truncates.
  snprintf(buf, sizeof(buf), "%-5s  %s",
           _opModeNode->getMode().c_str(),
           Homie.isConnected() ? "[W]" : "---");
  _display->drawStr(0, 10, buf);

  // Row 2: pool temperature + pool pump state
  float poolTempVal = (_poolTemp != nullptr) ? _poolTemp->getTemperature() : NAN;
  bool  poolOn      = (_poolPump != nullptr) && _poolPump->getSwitch();
  if (isnan(poolTempVal)) {
    snprintf(buf, sizeof(buf), "Pool:  ---.-C %s", poolOn ? "ON " : "OFF");
  } else {
    snprintf(buf, sizeof(buf), "Pool: %5.1fC  %s", poolTempVal, poolOn ? "ON " : "OFF");
  }
  _display->drawStr(0, 22, buf);

  // Row 3: solar temperature + solar pump state
  float solarTempVal = (_solarTemp != nullptr) ? _solarTemp->getTemperature() : NAN;
  bool  solarOn      = (_solarPump != nullptr) && _solarPump->getSwitch();
  if (isnan(solarTempVal)) {
    snprintf(buf, sizeof(buf), "Solar: ---.-C %s", solarOn ? "ON " : "OFF");
  } else {
    snprintf(buf, sizeof(buf), "Solar: %4.1fC  %s", solarTempVal, solarOn ? "ON " : "OFF");
  }
  _display->drawStr(0, 34, buf);

  // Row 4: configured timer window
  const TimerSetting ts = _opModeNode->getTimerSetting();
  snprintf(buf, sizeof(buf), "%02u:%02u - %02u:%02u",
           ts.timerStartHour, ts.timerStartMinutes,
           ts.timerEndHour, ts.timerEndMinutes);
  _display->drawStr(0, 46, buf);

  _display->sendBuffer();
}

void LocalDisplayNode::drawSettings() {
  char    buf[32];  // 32 bytes: enough for longest formatted line with null terminator
  uint8_t idx   = static_cast<uint8_t>(_settingsItem);
  uint8_t total = static_cast<uint8_t>(LocalSettingItem::COUNT);
  float   value = _editingValue ? _settingsValue : readSettingValue(_settingsItem);

  _display->clearBuffer();
  _display->setFont(u8g2_font_6x10_tf);

  // Title + item counter
  snprintf(buf, sizeof(buf), "Settings %u/%u", idx + 1, total);
  _display->drawStr(0, 10, buf);

  // Setting label
  _display->drawStr(0, 22, getSettingLabel(_settingsItem));

  // Setting value (angle brackets indicate edit mode)
  snprintf(buf, sizeof(buf), _editingValue ? ">%6.2f<" : " %6.2f ", value);
  _display->drawStr(28, 36, buf);

  // Hint line in smaller font
  _display->setFont(u8g2_font_5x7_tf);
  _display->drawStr(0, 60, _editingValue ? "SEL=save UP/DN=chg" : "SEL=edit UP/DN=nav");

  _display->sendBuffer();
}

const char* LocalDisplayNode::getSettingLabel(LocalSettingItem item) {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:   return "Pool Max Temp";
    case LocalSettingItem::SOLAR_MIN_TEMP:  return "Solar Min Temp";
    case LocalSettingItem::HYSTERESIS:      return "Hysteresis";
    case LocalSettingItem::TIMER_START_H:   return "Start Hour";
    case LocalSettingItem::TIMER_START_MIN: return "Start Minute";
    case LocalSettingItem::TIMER_END_H:     return "End Hour";
    case LocalSettingItem::TIMER_END_MIN:   return "End Minute";
    default:                                return "?";
  }
}

float LocalDisplayNode::readSettingValue(LocalSettingItem item) const {
  switch (item) {
    case LocalSettingItem::POOL_MAX_TEMP:
      return _opModeNode->getPoolMaxTemperature();
    case LocalSettingItem::SOLAR_MIN_TEMP:
      return _opModeNode->getSolarMinTemperature();
    case LocalSettingItem::HYSTERESIS:
      return _opModeNode->getTemperatureHysteresis();
    case LocalSettingItem::TIMER_START_H:
      return static_cast<float>(_opModeNode->getTimerSetting().timerStartHour);
    case LocalSettingItem::TIMER_START_MIN:
      return static_cast<float>(_opModeNode->getTimerSetting().timerStartMinutes);
    case LocalSettingItem::TIMER_END_H:
      return static_cast<float>(_opModeNode->getTimerSetting().timerEndHour);
    case LocalSettingItem::TIMER_END_MIN:
      return static_cast<float>(_opModeNode->getTimerSetting().timerEndMinutes);
    default:
      return 0.0f;
  }
}
