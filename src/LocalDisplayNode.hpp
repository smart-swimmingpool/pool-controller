/**
 * Local display node for showing pool controller status without internet.
 *
 * Supports OLED (SSD1306 128x64, default) and optionally e-paper displays
 * via the U8g2 library.  Compile with -D USE_EPAPER to enable e-paper mode.
 *
 * The node runs its loop even when WiFi / MQTT are not connected so that
 * status information is always available locally.
 */

#pragma once

#include <Homie.hpp>
#include "DallasTemperatureNode.hpp"
#include "RelayModuleNode.hpp"
#include "OperationModeNode.hpp"

// Forward-declare U8G2 so that the large U8g2lib.h header is only pulled in
// by LocalDisplayNode.cpp, not by every translation unit that includes this header.
class U8G2;

/**
 * Controller settings that can be adjusted via local UP/DOWN/SELECT buttons.
 * The enum is defined here so that ButtonControlNode can share the same type.
 */
enum class LocalSettingItem : uint8_t {
  POOL_MAX_TEMP   = 0,
  SOLAR_MIN_TEMP  = 1,
  HYSTERESIS      = 2,
  TIMER_START_H   = 3,
  TIMER_START_MIN = 4,
  TIMER_END_H     = 5,
  TIMER_END_MIN   = 6,
  COUNT           = 7
};

class LocalDisplayNode : public HomieNode {

public:
  /**
   * @param id / name   Homie node identifiers
   * @param opModeNode  Node whose mode / settings are displayed
   * @param poolPump    Pool-pump relay node (for pump state display)
   * @param solarPump   Solar-pump relay node
   * @param poolTemp    Pool temperature sensor node
   * @param solarTemp   Solar temperature sensor node
   * @param sdaPin      I2C SDA pin (0xff = use platform default)
   * @param sclPin      I2C SCL pin (0xff = use platform default)
   */
  LocalDisplayNode(const char*            id,
                   const char*            name,
                   OperationModeNode*     opModeNode,
                   RelayModuleNode*       poolPump,
                   RelayModuleNode*       solarPump,
                   DallasTemperatureNode* poolTemp,
                   DallasTemperatureNode* solarTemp,
                   uint8_t                sdaPin = 0xff,
                   uint8_t                sclPin = 0xff);

  /** Switch to the status view (temperatures, mode, pump states). */
  void showStatus();

  /** Switch to the settings view for the given item. */
  void showSettings(LocalSettingItem item, float editValue, bool editing);

  /** Force a redraw on the next loop tick (e.g. after a state change). */
  void requestRedraw() { _needsRedraw = true; }

protected:
  void setup() override;
  void loop() override;

private:
// Refresh interval: OLED can update quickly; e-paper needs long pauses.
#ifdef USE_EPAPER
  static const unsigned long REFRESH_INTERVAL_MS = 60000UL;
#else
  static const unsigned long REFRESH_INTERVAL_MS = 2000UL;
#endif

  OperationModeNode*     _opModeNode;
  RelayModuleNode*       _poolPump;
  RelayModuleNode*       _solarPump;
  DallasTemperatureNode* _poolTemp;
  DallasTemperatureNode* _solarTemp;

  U8G2*   _display;
  uint8_t _sdaPin;
  uint8_t _sclPin;

  bool             _needsRedraw;
  bool             _inSettingsView;
  LocalSettingItem _settingsItem;
  float            _settingsValue;
  bool             _editingValue;

  unsigned long _lastUpdate;

  void drawStatus();
  void drawSettings();

  static const char* getSettingLabel(LocalSettingItem item);
  float              readSettingValue(LocalSettingItem item) const;
};
