
#pragma once

#include "Timer.hpp"

class Rule {

public:
  Rule()
      : _poolTemp(0.0), _solarTemp(0.0), _poolMaxTemp(0.0), _solarMinTemp(0.0), _hysteresis(0.0), _poolVolume(0.0),
        _pumpCapacity(0.0), _useTemperatureBasedDuration(false){};

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
   * Calculate required filtration duration in hours based on water
   * temperature.
   *
   * Implementation: Option A - Enhanced Temperature Formula
   * This approach aligns with DIN standards and pool industry best
   * practices, recommending 2-3 turnovers per day minimum, increasing
   * with temperature.
   *
   * Formula: turnoverFactor = baseTurnoverFactor × (temp / 20.0)
   * - At 20°C: 2.5 turnovers/day (industry minimum)
   * - At 28°C: 3.5 turnovers/day (warm water standard)
   * - At max pool temp: 4.0 turnovers/day (maximum protection)
   *
   * References:
   * - DIN 19643: German standard for public pool water treatment
   * - German rule of thumb: filtration_hours = (temp / 2) + 2
   * - Industry standard: 2-3 turnovers minimum, 3-4 for warm/busy
   *   pools
   *
   * Alternative approaches considered (see documentation):
   * - Option B: Direct German formula (temp/2 + 2 hours)
   * - Option C: Fully configurable turnover min/max parameters
   */
  float calculateFiltrationDuration() {
    if (_pumpCapacity <= 0.0 || _poolVolume <= 0.0) {
      return 0.0;  // Invalid configuration
    }

    float temp = getPoolTemperature();
    float turnoverFactor;

    // Base turnover factor: 2.5 turnovers/day aligns with DIN and
    // industry standards (minimum 2-3 turnovers recommended)
    const float baseTurnoverFactor = 2.5;

    // If max pool temperature is reached, use maximum filtration time
    // to help prevent water from getting warmer (4 turnovers/day)
    if (temp >= getPoolMaxTemperature()) {
      turnoverFactor = 4.0;  // Maximum protection at max pool temp
    }
    // Temperature below 20°C: use base turnover rate
    else if (temp <= 20.0) {
      turnoverFactor = baseTurnoverFactor;  // 2.5 turnovers at 20°C
    }
    // Temperature-dependent scaling: increases linearly with temp
    // At 28°C: 2.5 × (28/20) = 3.5 turnovers
    // At 24°C: 2.5 × (24/20) = 3.0 turnovers
    else {
      float tempAdjustment = temp / 20.0;
      turnoverFactor       = baseTurnoverFactor * tempAdjustment;

      // Cap at 4.0 turnovers to avoid excessive runtime
      if (turnoverFactor > 4.0) {
        turnoverFactor = 4.0;
      }
    }

    // Filtration time (hours) = (Pool Volume / Pump Capacity) ×
    // Turnover factor
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
