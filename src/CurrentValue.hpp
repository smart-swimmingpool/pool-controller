#pragma once


class CurrentValues {

  public:
    CurrentValues();
    virtual ~CurrentValues();

    void setTemperaturePool(const long temp) {
      _temperaturePool = temp;
    }
    long getTemperaturePool() {
      return _temperaturePool;
    }

    void setTemperatureSolar(const long temp) {
      _temperatureSolar = temp;
    }
    long getTemperatureSolar() {
      return _temperatureSolar;
    }

  private:
    long _temperaturePool;
    long _temperatureSolar;

};
