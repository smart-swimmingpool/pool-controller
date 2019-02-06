
//MQTT settings
const char* MQTT_SERVER = "192.168.178.25";
const char* DEVICE_NAME = "ESP32-POOLSENSOR";
//MQTT-Topic receiving RC switch on/off
const char* MQTT_MESSAGE_RCSENDER = "/pool/switch/";


#define PIN_DS_SOLAR 16 // Temp-Sensor Solar
#define PIN_DS_POOL 17  // Temp-Sensor Pool
#define PIN_RSSWITCH 18 // für 433MHz Sender

#define TEMP_READ_INTERVALL 60   //Sekunden zwischen Updates der Temperaturen.

