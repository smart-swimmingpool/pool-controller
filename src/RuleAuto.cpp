
#include "RuleAuto.hpp"

RuleAuto::RuleAuto(RelayModuleNode* solarRelay, RelayModuleNode* poolRelay) {
  _solarRelay = solarRelay;
  _poolRelay  = poolRelay;
}

void RuleAuto::loop() {
  Homie.getLogger() << cIndent << F("§ RuleAuto: loop") << endl;

  _poolRelay->setSwitch(checkPoolPumpTimer());

  if (_poolRelay->getSwitch()) {
    //pool pump is running

    if (_solarRelay->getSwitch()) {
      //solar is on

      float hyst = getTemperatureHysteresis();
      if (getSolarTemperature() < (getSolarMinTemperature() - hyst)) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Solar below min. required solar temp. (") << getSolarMinTemperature()
                          << F("). Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else if (getPoolTemperature() >= (getSolarTemperature() + hyst)) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Pool temp. (") << getPoolTemperature() << F(") reaches solar temp (")
                          << getSolarTemperature() << F("). Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else if (getPoolTemperature() >= (getPoolMaxTemperature() + hyst)) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: Pool temp. (") << getPoolTemperature() << F(") above max. temperature (")
                          << getPoolMaxTemperature() << F("). Switch solar off") << endl;
        _solarRelay->setSwitch(false);

      } else {
        // leave it on.
        Homie.getLogger() << cIndent << F("§ RuleAuto: Solar on -> no change") << endl;
      }

    } else {
      //solar is off: !_solarRelay->getSwitch()
      if ((getPoolTemperature() <= getPoolMaxTemperature()) && (getPoolTemperature() <= getSolarTemperature()) &&
          (getSolarMinTemperature() <= getSolarTemperature())) {
        Homie.getLogger() << cIndent << F("§ RuleAuto: below max. Temperature (") << getPoolMaxTemperature()
                          << F("). Switch solar on") << endl;
        _solarRelay->setSwitch(true);

      } else {
        // no change of status
        Homie.getLogger() << cIndent << F("§ RuleAuto: Solar off -> no change") << endl;
      }
    }
  } else {

    if (_solarRelay->getSwitch()) {
      Homie.getLogger() << cIndent << F("§ RuleAuto: pool pump is disabled. Switch solar off") << endl;
      _solarRelay->setSwitch(false);
    }
  }
  Homie.getLogger() << cIndent << F("§ RuleAuto: Pool temp. :     ") << getPoolTemperature() << endl;
  Homie.getLogger() << cIndent << F("§ RuleAuto: max. Pool temp.: ") << getPoolMaxTemperature() << endl;
  Homie.getLogger() << cIndent << F("§ RuleAuto: Solar temp. :     ") << getSolarTemperature() << endl;
  Homie.getLogger() << cIndent << F("§ RuleAuto: min. Solar temp.: ") << getSolarMinTemperature() << endl;
}

bool RuleAuto::checkPoolPumpTimer() {
  Homie.getLogger() << F("↕  checkPoolPumpTimer") << endl;

  tm   time = getCurrentDateTime();
  bool retval;

  tm startTime = getStartTime(getTimerSetting());
  tm endTime;

  // Check if temperature-based duration is enabled
  if (getUseTemperatureBasedDuration()) {
    // Use dynamic filtration duration based on temperature
    Homie.getLogger() << cIndent << F("Using temperature-based filtration duration") << endl;
    
    float filtrationHours = calculateFiltrationDuration();
    
    // Check for invalid configuration
    if (filtrationHours <= 0.0) {
      Homie.getLogger() << cIndent << F("✖ Invalid filtration configuration (pool volume or pump capacity not set)") << endl;
      Homie.getLogger() << cIndent << F("  Pool volume: ") << getPoolVolume() << F(" m³") << endl;
      Homie.getLogger() << cIndent << F("  Pump capacity: ") << getPumpCapacity() << F(" m³/h") << endl;
      // Return false to keep pump off when configuration is invalid
      return false;
    }
    
    // Calculate end time based on start time + filtration duration
    endTime = startTime;
    // Add hours and minutes separately for better precision
    int hoursToAdd = (int)filtrationHours;
    int minutesToAdd = (int)((filtrationHours - hoursToAdd) * 60.0 + 0.5);  // Round to nearest minute
    endTime.tm_hour += hoursToAdd;
    endTime.tm_min += minutesToAdd;
    mktime(&endTime);  // Normalize the time structure

    Homie.getLogger() << cIndent << F("Pool temp: ") << getPoolTemperature() << F("°C") << endl;
    Homie.getLogger() << cIndent << F("Max pool temp: ") << getPoolMaxTemperature() << F("°C") << endl;
    Homie.getLogger() << cIndent << F("filtration duration (h)= ") << filtrationHours << endl;
  } else {
    // Use fixed timer end time from settings
    Homie.getLogger() << cIndent << F("Using fixed timer duration") << endl;
    endTime = getEndTime(getTimerSetting());
  }

  Homie.getLogger() << cIndent << F("currenttime=") << asctime(&time);
  Homie.getLogger() << cIndent << F("startTime=  ") << asctime(&startTime);
  Homie.getLogger() << cIndent << F("endTime=    ") << asctime(&endTime);

  if (difftime(mktime(&time), mktime(&startTime)) >= 0 && difftime(mktime(&time), mktime(&endTime)) <= 0) {
    retval = true;

  } else {
    retval = false;
  }

  Homie.getLogger() << cIndent << F("checkPoolPumpTimer = ") << retval << endl;
  return retval;
}
