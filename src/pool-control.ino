/**
 * ESP32 zur Steuerung des Pools:
 * - 2 Temperaturfühler
 * - Interner Temperatur-Sensor
 * - 433MHz Sender für Pumpensteuerung
 *
 * Wird über openHAB gesteurt.
 */

#include <Arduino.h>
#include <ESPBASE.h>

//*** Normal code definition here ...
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RCSwitch.h>
#include "RemoteDebug.h"        //https://github.com/JoaoLopesF/RemoteDebug
#include "esp_system.h"

extern "C" {

uint8_t temprature_sens_read();

}

#define PIN_DS_SOLAR 16 // Temp-Sensor Solar
#define PIN_DS_POOL 17  // Temp-Sensor Pool
#define PIN_RSSWITCH 18 // für 433MHz Sender

#define TEMP_READ_INTERVALL 60   //Sekunden zwischen Updates der Temperaturen.

//MQTT settings
const char* MQTT_SERVER = "192.168.178.25";
const char* DEVICE_NAME = "ESP32-POOLSENSOR";
const char* MQTT_TOPIC = "/sensor";
//MQTT-Topic receiving RC switch on/off
const char* MQTT_MESSAGE_RCSENDER ="/pool/switch/";

RemoteDebug Debug;
ESPBASE Esp;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

//GPIO12 & GPIO14 -> Temperatur
OneWire dsSolar(PIN_DS_SOLAR);
OneWire dsPool(PIN_DS_POOL);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensorSolar(&dsSolar);
DallasTemperature sensorPool(&dsPool);

//RS Switches via 433MHz
RCSwitch mySwitch = RCSwitch();

// the timer objects
volatile int interruptCounter;
hw_timer_t *timerIntervall = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t cp0_regs[18];

/**
 *  Connect MQTT Server
 */
void connectMQTT() {
  DEBUG_V("Attempting MQTT connection to %s ...\r", MQTT_SERVER);
  // Attempt to connect
  if (mqttClient.connect(DEVICE_NAME)) {
    DEBUG_V("MQTT connected.\r");
    mqttClient.subscribe("/pool/switch/#");

  } else {
    DEBUG_E("failed, rc=%i\n", mqttClient.state());
    // Print to know why the connection failed
    // See http://pubsubclient.knolleary.net/api.html#state for the failure code and its reason
    switch (mqttClient.state()) {
      case -4:
        DEBUG_E("MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time\r");
        break;
      case -3:
        DEBUG_E("MQTT_CONNECTION_LOST - the network connection was broken\r");
        break;
      case -2:
        DEBUG_E("MQTT_CONNECT_FAILED - the network connection failed\r");
        break;
      case -1:
        DEBUG_E("MQTT_DISCONNECTED - the client is disconnected cleanly\r");
        break;
    }
  }
}

/**
 * https://stackoverflow.com/questions/9072320/split-string-into-string-array
 */
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++) {
    if(data.charAt(i)==separator || i==maxIndex) {
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

/**
 *
 */
double getTemperature(DallasTemperature sensor) {
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensor.requestTemperatures(); // Send the command to get temperature readings

  double t;
  int cnt = 0;
  do {
    t = sensor.getTempCByIndex(0); // Why "byIndex"?  You can have more than one DS18B20 on the same bus.
    // 0 refers to the first IC on the wire
    delay(1);
    Serial.print(".");
    cnt++;
    if(cnt > 3) {
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
void onTemperatureTimer( void * parameter ) {
  Serial.println("onTermperatureTimer ->");

  // get FPU state
  uint32_t cp_state = xthal_get_cpenable();

  if(cp_state) {
    // Save FPU registers
    xthal_save_cp0(cp0_regs);
  } else {
    // enable FPU
    xthal_set_cpenable(1);
  }

  double temp1 = getTemperature(sensorSolar);
  if(temp1 > -127.0) {
    publish("solar", "temperature", String(temp1, 1));
  } else {
    publish("solar", "status", "Error reading sensor");
  }

  double temp2 = getTemperature(sensorPool);
  if(temp2 > -127.0) {
    publish("pool", "temperature", String(temp2, 1));
  } else {
    publish("pool", "status", "Error reading sensor");
  }

  //internal temp of ESP
  uint8_t temp_farenheit= temprature_sens_read();
  double temp3 = ( temp_farenheit - 32 ) / 1.8;
  publish("esp", "temperature", String(temp3, 1));

  if(cp_state) {
    // Restore FPU registers
    xthal_restore_cp0(cp0_regs);
  } else {
    // turn it back off
    xthal_set_cpenable(0);
  }
  
  Serial.println("onTermperatureTimer <-");
}

/**
 *  switch on MQTT message
 */
void onMqttCallback(char* topic, byte* payload, unsigned int length) {
  DEBUG_V("onMqttCallback -> topic='%s'", topic);
  
  String command = String((char*)topic);

  if(command.indexOf(MQTT_MESSAGE_RCSENDER) < 0) {
    DEBUG_E("onMqttCallback -> ABORTING: Command ignored: '%s'", topic);
    DEBUG_E("topic has to start with '%s'", MQTT_MESSAGE_RCSENDER);

  } else {

    // "/switch/groupNumber/switchNumber"
    char groupNumber[6];
    getValue(command, '/', 3).toCharArray(groupNumber, 6);

    char switchNumber[6];
    getValue(command, '/', 4).toCharArray(switchNumber, 6);

    payload[length] = '\0';
    String message = String((char*)payload);

    DEBUG_I("Switch: Group: %s Switch: %s Message:%s\r", groupNumber, switchNumber, message.c_str());

    if(message.equals( "ON")) {
      // Switch on
      mySwitch.switchOn(groupNumber, switchNumber);
    } else {
      mySwitch.switchOff(groupNumber, switchNumber);
    }

  }
  DEBUG_V("onMqttCallback <-\r");
}

/**
 *  MQTT publish
 */
void publish(String sensor, String type, String value) {
  char topic[48];
  memset(&topic, 0, sizeof(topic));
  sprintf(topic, "%s/%s/%s", MQTT_TOPIC, sensor.c_str(), type.c_str());

  char msg[256];
  memset(&msg, 0, sizeof(msg));
  sprintf(msg, "{\"device\": \"%s\", \"sensor\": \"%s\", \"type\": \"%s\", \"value\": \"%s\"}\n",
           DEVICE_NAME, sensor.c_str(), type.c_str(), value.c_str());

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  DEBUG_V("publish on Topic [%s] message: [%s]\r", topic, msg);
  mqttClient.publish(topic, msg);
}

/**

*/
void setup() {
  Serial.begin(115200);
  Esp.initialize();

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

 	Debug.begin(config.DeviceName.c_str(), Debug.VERBOSE); // Initiaze the telnet server
  Debug.setResetCmdEnabled(true); // Enable the reset command
  //Debug.showDebugLevel(false); // To not show debug levels
  //Debug.showTime(true); // To show time
  //Debug.showProfiler(true); // To show profiler - time between messages of Debug
  // Good to "begin ...." and "end ...." messages

  Debug.showProfiler(true); // Profiler
  Debug.showColors(true); // Colors

  Debug.setSerialEnabled(true); // if you wants serial echo - only recommended if ESP8266 is plugged in USB

  Serial.println(F("-------------------------------------"));
  Serial.println(F(" Pool Steuerung"));
  Serial.println(F("-------------------------------------"));

  if (Esp.WIFI_connected) {
    if (MDNS.begin(config.DeviceName.c_str())) { // Start the mDNS responder for esp8266.local
      DEBUG_I("mDNS responder started\r");
      // Add service to MDNS-SD
      //MDNS.addService("arduino", "tcp", 8266);
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("telnet", "tcp", 23); // Telnet server RemoteDebug
    } else {
      DEBUG_E("Error setting up mDNS responder!\r");
    }

    mySwitch.enableTransmit(PIN_RSSWITCH);
    //mySwitch.setRepeatTransmit(10);
    //mySwitch.setPulseLength(350);

    mqttClient.setServer(MQTT_SERVER, 1883);
    mqttClient.setCallback(onMqttCallback);
    if (!mqttClient.connected()) {
      connectMQTT();
    }

    /* 2nd tick take 1/(80MHZ/80) = 1us so we set divider 80 and count up */
    timerIntervall = timerBegin(0, 80, true); //timer 1, div 80
    /* Attach onTimer function to our timer */
    timerAttachInterrupt(timerIntervall, &onTimer, true);
    /* Set alarm to call onTimer function every second 1 tick is 1us => 1 second is 1000000us */
    /* Repeat the alarm (third parameter) */
    timerAlarmWrite(timerIntervall, 1e+6 * TEMP_READ_INTERVALL, true);
    timerAlarmEnable(timerIntervall); //enable interrupt

    ArduinoOTA.onStart([]() { // what to do before OTA download insert code here
      Serial.println("OTA Start");
      timerAlarmDisable(timerIntervall);
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        //  feed de DOG :)
        customWatchdog = millis();
    });
    ArduinoOTA.onEnd([]() { // what to do after OTA download insert code here
      Serial.println("OTA end");
      timerAlarmEnable(timerIntervall); //enable interrupt
    });

    publish("pool", "device", "started"); //pubish started event
  }

  DEBUG_I("Setup ready");
}

/**

*/
void loop() {
  // OTA request handling
  ArduinoOTA.handle();

  //  WebServer requests handling
  server.handleClient();

  //  feed de DOG :)
  customWatchdog = millis();

  //**** Normal Skecth code here ...

  // Remote debug over telnet
  Debug.handle();

  if (Esp.CFG_saved) {
    int wifi_retry = 0;

    while(WiFi.status() != WL_CONNECTED && wifi_retry < 5 ) {
      wifi_retry++;
      Serial.println("WiFi not connected. Try reconnect");
      WiFi.disconnect();

      WiFi.mode(WIFI_OFF);
      WiFi.mode(WIFI_STA);
      WiFi.begin(config.ssid.c_str(), config.password.c_str());
      delay(100);
    }

    if(wifi_retry >= 5) {
      ets_printf("reboot\n");
      esp_restart_noos();
    }


    if (!mqttClient.connected()) {
      connectMQTT();
    }
    mqttClient.loop();
  }

  if (interruptCounter > 0) {

    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);

    onTemperatureTimer(NULL);
  }
}
