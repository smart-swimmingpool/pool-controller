/**
 * Homie Node for Dallas Temperature sensors.
 *
 */

#include "DallasTemperatureNode.hpp"

DallasTemperatureNode::DallasTemperatureNode(const char *id, const int pin, const int measurementInterval)
    : HomieNode(id, "DallasTemperature", cTemperature), _pin(pin), _lastMeasurement(0) {
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
}

void DallasTemperatureNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

void DallasTemperatureNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    Homie.getLogger() << "〽 Sending Temperature: " << getId() << endl;


    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    sensor.requestTemperatures();  // Send the command to get temperature readings

    int    cnt = 0;
    do {
      temperature = sensor.getTempCByIndex(0);  // Why "byIndex"?  You can have more than one DS18B20 on the same bus.
      // 0 refers to the first IC on the wire
      delay(1);
      Homie.getLogger() << "✖";
      _lastMeasurement = millis();
      cnt++;
      if (cnt > 3) {
        temperature = NAN;
        Homie.getLogger() << " Error reading sensor: " << getId() << endl;
        setProperty(cStatus).send("error");
        return;
      }
    } while (temperature >= 50.0 || temperature <= -50.0);


    Homie.getLogger() << "  • Temperature=" << temperature << cTemperatureUnit << endl;
    setProperty(cStatus).send("ok");
    setProperty(cTemperature).send(String(temperature));
  }
}
void DallasTemperatureNode::onReadyToOperate() {
  //Homie.getLogger() << "〽 DallasTemperatureNode::onReadyToOperate" << endl;

  //setProperty(cTemperatureUnit).send("°C");
}

void DallasTemperatureNode::setup() {
  //Homie.getLogger() << "〽 DallasTemperatureNode::setup" << endl;

  advertise(cStatus)
    .setName(cStatusName);

  advertise(cTemperature)
    .setName(cTemperature)
    .setDatatype("float")
    .setFormat("-50:50")
    .setUnit(cTemperatureUnit);

  OneWire oneWire(_pin);

  DallasTemperature dt(&oneWire);
  sensor =  dt;
  sensor.begin();
}
