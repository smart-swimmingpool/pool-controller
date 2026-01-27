
#pragma once

#include "Timer.hpp"

class Rule {

public:
  Rule() : _poolTemp(0.0), _solarTemp(0.0), _poolMaxTemp(0.0), _solarMinTemp(0.0), _hysteresis(0.0), _poolVolume(0.0), _pumpCapacity(0.0), _useTemperatureBasedDuration(false){};

  void  setPoolTemperature(float temp) { _poolTemp = temp; };
  float getPoolTemperature() { return _poolTemp; };
  void  setSolarTemperature(float temp) { _solarTemp = temp; };
  float getSolarTemperature() { return _solarTemp; };

  void  setPoolMaxTemperature(float temp) { _poolMaxTemp = temp; };
  float getPoolMaxTemperature() { return _poolMaxTemp; };

  void  setSolarMinTemperature(float temp) { _solarMinTemp = temp; };
  float getSolarMinTemperature() { return _solarMinTemp; };

  void  setTemperatureHysteresis(float temp) { _hysteresis = temp; };
  float getTemperatureHysteresis() { return _hysteresis; };

  void  setPoolVolume(float volume) { _poolVolume = volume; };
  float getPoolVolume() { return _poolVolume; };

  void  setPumpCapacity(float capacity) { _pumpCapacity = capacity; };
  float getPumpCapacity() { return _pumpCapacity; };

  void setUseTemperatureBasedDuration(bool enable) { _useTemperatureBasedDuration = enable; };
  bool getUseTemperatureBasedDuration() { return _useTemperatureBasedDuration; };

  void         setTimerSetting(TimerSetting setting) { _timerSetting = setting; };
  TimerSetting getTimerSetting() { return _timerSetting; };

  /**
   * Calculate required filtration duration in hours based on water temperature.
   * At 20°C: 1 complete turnover per day
   * At 28°C: 2 complete turnovers per day
   * Linear interpolation between 20-28°C: turnover = 1.0 + ((temp - 20.0) / 8.0)
   *   where 8.0 is the temperature range (28°C - 20°C)
   */
  float calculateFiltrationDuration() {
    if (_pumpCapacity <= 0.0 || _poolVolume <= 0.0) {
      return 0.0;  // Invalid configuration
    }

    float temp = getPoolTemperature();
    float turnoverFactor;

    // Calculate turnover factor based on temperature
    if (temp <= 20.0) {
      turnoverFactor = 1.0;  // 1 complete turnover at 20°C or below
    } else if (temp >= 28.0) {
      turnoverFactor = 2.0;  // 2 complete turnovers at 28°C or above
    } else {
      // Linear interpolation between 20°C and 28°C
      // Formula: 1.0 + (temperature increase / temperature range)
      turnoverFactor = 1.0 + ((temp - 20.0) / 8.0);
    }

    // Filtration time (hours) = (Pool Volume / Pump Capacity) × Turnover factor
    return (_poolVolume / _pumpCapacity) * turnoverFactor;
  }

  /**
   * get the Mode for which the Rule is created.
   */
  virtual const char* getMode();
  virtual void        loop();

protected:
  float _poolTemp;
  float _solarTemp;

  float _poolMaxTemp;
  float _solarMinTemp;

  float _hysteresis;

  float _poolVolume;    // Pool volume in m³
  float _pumpCapacity;  // Pump capacity in m³/h

  bool _useTemperatureBasedDuration;  // Enable/disable temperature-based duration calculation

  TimerSetting _timerSetting;
};
