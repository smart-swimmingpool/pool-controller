/**
 * ESP32 zur Steuerung des Pools:
 * - 2 Temperaturfühler
 * - Interner Temperatur-Sensor
 * - 433MHz Sender für Pumpnsteuerung
 *
 * Wird über openHAB gesteurt.
 */
#include <Homie.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <RCSwitch.h>
#include "RemoteDebug.h"  //https://github.com/JoaoLopesF/RemoteDebug

#include "pool-control.h"

extern "C" {

uint8_t temprature_sens_read();
}

//MQTT settings
const char* MQTT_SERVER = "192.168.178.25";
const char* DEVICE_NAME = "ESP32-POOLSENSOR";
//MQTT-Topic receiving RC switch on/off
const char* MQTT_MESSAGE_RCSENDER = "/pool/switch/";

RemoteDebug Debug;

//GPIO12 & GPIO14 -> Temperatur
OneWire dsSolar(PIN_DS_SOLAR);
OneWire dsPool(PIN_DS_POOL);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensorSolar(&dsSolar);
DallasTemperature sensorPool(&dsPool);

HomieNode solarTemperatureNode("solarTemp", "Solar Temperature", "temperature");
HomieNode poolTemperatureNode("poolTemp", "Pool Temperature", "temperature");
HomieNode ctrlTemperatureNode("controllerTemp", "Controller Temperature", "temperature");

HomieNode poolPumpNode("poolPump", "Pool Pump", "switch");
HomieNode solarPumpNode("solarPump", "Solar Pump", "switch");

HomieSetting<long> temperatureIntervalSetting("temperatureInterval", "The temperature interval in seconds");

//RS Switches via 433MHz
RCSwitch mySwitch = RCSwitch();

// the timer objects
volatile int interruptCounter;
hw_timer_t*  timerIntervall = NULL;
portMUX_TYPE timerMux       = portMUX_INITIALIZER_UNLOCKED;

uint32_t cp0_regs[18];

/**
 * https://stackoverflow.com/questions/9072320/split-string-into-string-array
 */
String getValue(String data, char separator, int index) {
  int found      = 0;
  int strIndex[] = {0, -1};
  int maxIndex   = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/**
 *
 */
double getTemperature(DallasTemperature sensor) {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensor.requestTemperatures();  // Send the command to get temperature readings

  double t;
  int    cnt = 0;
  do {
    t = sensor.getTempCByIndex(0);  // Why "byIndex"?  You can have more than one DS18B20 on the same bus.
    // 0 refers to the first IC on the wire
    delay(1);
    Serial.print(".");
    cnt++;
    if (cnt > 3) {
      return -127.0;
    }
  } while (t == 85.0 || t == -127.0);

  return t;
}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

/**
 * called on Timer.
 */
void onTemperatureTimer(void* parameter) {
  Serial.println("onTermperatureTimer ->");

  // get FPU state
  uint32_t cp_state = xthal_get_cpenable();

  if (cp_state) {
    // Save FPU registers
    xthal_save_cp0(cp0_regs);
  } else {
    // enable FPU
    xthal_set_cpenable(1);
  }

  double temp1 = getTemperature(sensorSolar);
  if (temp1 > -127.0) {
    solarTemperatureNode.setProperty("degrees").send(String(temp1, 1));
  } else {
    solarTemperatureNode.setProperty("status").send("Error reading sensor");
  }

  double temp2 = getTemperature(sensorPool);
  if (temp2 > -127.0) {
    poolTemperatureNode.setProperty("degrees").send(String(temp2, 1));
  } else {
    poolTemperatureNode.setProperty("status").send("Error reading sensor");
  }

  //internal temp of ESP
  uint8_t temp_farenheit = temprature_sens_read();
  double  temp3          = (temp_farenheit - 32) / 1.8;
  ctrlTemperatureNode.setProperty("degrees").send(String(temp3, 1));

  if (cp_state) {
    // Restore FPU registers
    xthal_restore_cp0(cp0_regs);
  } else {
    // turn it back off
    xthal_set_cpenable(0);
  }

  Serial.println("onTermperatureTimer <-");
}

/**
 * Handler for switching pool Pump on/off.
 */
bool poolPumpSwitchOnHandler(HomieRange range, String value) {
  bool retval;

  if (value != "true" && value != "false") {
    retval = false;
  } else {

    if (value == "true") {
      // Switch on
      mySwitch.switchOn("11111", "10000");  //todo: make configurable
    } else {
      mySwitch.switchOff("11111", "10000");  //todo: make configurable
    }

    poolPumpNode.setProperty("on").send(value);
    bool on = (value == "true");
    Homie.getLogger() << "Switch is " << (on ? "on" : "off") << endl;
    retval = true;
  }
  return retval;
}

/**
 * Handler for switching solar Pump on/off.
 */
bool solarPumpSwitchOnHandler(HomieRange range, String value) {
  bool retval;

  if (value != "true" && value != "false") {
    retval = false;
  } else {

    if (value == "true") {
      // Switch on
      mySwitch.switchOn("11111", "01000");  //todo: make configurable
    } else {
      mySwitch.switchOff("11111", "01000");  //todo: make configurable
    }

    solarPumpNode.setProperty("on").send(value);
    bool on = (value == "true");
    Homie.getLogger() << "Switch is " << (on ? "on" : "off") << endl;
    retval = true;
  }
  return retval;
}

/**
 * Homie Setup handler.
 */
void setupHandler() {

  solarTemperatureNode.advertise("degrees").setName("Degrees").setDatatype("float").setUnit("ºC");
  poolTemperatureNode.advertise("degrees").setName("Degrees").setDatatype("float").setUnit("ºC");
  ctrlTemperatureNode.advertise("degrees").setName("Degrees").setDatatype("float").setUnit("ºC");

  poolPumpNode.advertise("on").setName("On").setDatatype("boolean").settable(poolPumpSwitchOnHandler);
  solarPumpNode.advertise("on").setName("On").setDatatype("boolean").settable(solarPumpSwitchOnHandler);

  //default intervall of sending Temperature values
  temperatureIntervalSetting.setDefaultValue(TEMP_READ_INTERVALL).setValidator([](long candidate) { return candidate > 0; });
}

/**
 * Startup of controller.
 */
void setup() {
  Serial.begin(115200);

  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println(F("-------------------------------------"));
  Serial.println(F(" Pool Controller "));
  Serial.println(F("-------------------------------------"));

  //Homie.disableLogging();
  Homie_setFirmware("pool-controller", "1.0.0");  // The underscore is not a typo! See Magic bytes
  Homie.setSetupFunction(setupHandler);           //.setLoopFunction(loopHandler);

  Homie.setup();

  if (Homie.isConnected()) {

    mySwitch.enableTransmit(PIN_RSSWITCH);
    //mySwitch.setRepeatTransmit(10);
    //mySwitch.setPulseLength(350);

    /* 2nd tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
    timerIntervall = timerBegin(0, 80, true);  //timer 1, div 80
    /* Attach onTimer function to our timer */
    timerAttachInterrupt(timerIntervall, &onTimer, true);
    /* Set alarm to call onTimer function every second 1 tick is 1us => 1 second is 1000000us */
    /* Repeat the alarm (third parameter) */
    timerAlarmWrite(timerIntervall, 1e+6 * temperatureIntervalSetting.get(), true);
    timerAlarmEnable(timerIntervall);  //enable interrupt

    //publish("pool", "device", "started"); //pubish started event
  }

  DEBUG_I("Setup ready");
}

/**
 * Main loop of ESP.
 */
void loop() {

  Homie.loop();

  // Remote debug over telnet
  Debug.handle();

  if (interruptCounter > 0) {

    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    onTemperatureTimer(NULL);
  }
}
