/**
 * Homie Node for Dallas Temperature sensors.
 *
 */
#include "DallasTemperatureNode.hpp"

DallasTemperatureNode::DallasTemperatureNode(const char* id, const char* name, const int pin, const int measurementInterval)
    : HomieNode(id, name, "temperature") {

  _pin                 = pin;
  _measurementInterval = (measurementInterval > MIN_INTERVAL) ? measurementInterval : MIN_INTERVAL;
  _lastMeasurement     = 0;
}

void DallasTemperatureNode::printCaption() {
  Homie.getLogger() << cCaption << endl;
}

/**
 *
 */
void DallasTemperatureNode::loop() {
  if (millis() - _lastMeasurement >= _measurementInterval * 1000UL || _lastMeasurement == 0) {
    _lastMeasurement = millis();

    if (numberOfDevices > 0) {
      Homie.getLogger() << "〽 Sending Temperature: " << getId() << endl;
      // call sensors.requestTemperatures() to issue a global temperature
      // request to all devices on the bus
      sensor.requestTemperatures();  // Send the command to get temperature readings
      for (int i = 0; i < numberOfDevices; i++) {
        int cnt = 0;

        DeviceAddress tempDeviceAddress;
        if (sensor.getAddress(tempDeviceAddress, i)) {
          do {
            temperature = sensor.getTempC(tempDeviceAddress);

            cnt++;
            delay(1);

            if (cnt > 3) {
              temperature = NAN;
              Homie.getLogger() << " Error reading sensor: " << getId() << endl;
              setProperty(cStatus).send("Error reading sensor");

              return;
            }
          } while (temperature >= 50.0 || temperature <= -50.0);
        }

        Homie.getLogger() << cIndent << "Status=ok" << endl;
        setProperty(cStatus).send("ok");
        Homie.getLogger() << cIndent << "Temperature=" << temperature << endl;
        setProperty(cTemperature).send(String(temperature));
      }
    } else {
      //Homie.getLogger() << "No Sensor found!" << endl;
      //setProperty(cStatus).send("no sensor found.");
    }
  }
}

/**
 *
 */
void DallasTemperatureNode::onReadyToOperate() {

  advertise(cStatus).setName(cStatusName);
  advertise(cTemperature).setName(cTemperatureName).setRetained(true).setDatatype("float").setFormat("-50:100").setUnit(cTemperatureUnit);

  // Grab a count of devices on the wire
  numberOfDevices = sensor.getDeviceCount();
  Homie.getLogger() << cIndent << "Devices found at PIN " << _pin << ": " << numberOfDevices << endl;
  // report parasite power requirements
  Homie.getLogger() << cIndent << "Parasite power is: " << sensor.isParasitePowerMode() << endl;

  if (numberOfDevices > 0) {
    for (int i = 0; i < numberOfDevices; i++) {
      // Search the wire for address
      DeviceAddress tempDeviceAddress;  // We'll use this variable to store a found device address

      if (sensor.getAddress(tempDeviceAddress, i)) {
        Homie.getLogger() << cIndent << "Device " << i << " using address ";
        for (uint8_t i = 0; i < 8; i++) {
          if (tempDeviceAddress[i] < 16)
            Serial.print("0");
          Serial.print(tempDeviceAddress[i], HEX);
        }
      }
    }
  } else {
    Homie.getLogger() << "✖ No sensors found at pin " << _pin << endl;
    setProperty(cStatus).send("no sensors found");
  }
}

/**
 *
 */
void DallasTemperatureNode::setup() {
  printCaption();

  OneWire           oneWire(_pin);
  DallasTemperature sensor(&oneWire);

  // Start up the library
  sensor.begin();
}