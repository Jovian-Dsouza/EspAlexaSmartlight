#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include "fauxmoESP.h"

fauxmoESP fauxmo;

struct OutputDevice{
  const char * device_name;
  int pin;
  bool toggle_output;
};

// -----------------------------------------------------------------------------
#define SERIAL_BAUDRATE     115200

//The safest ESP8266 pins to use with relays are: GPIO 5(D1), GPIO 4(D2), GPIO 14(D5), GPIO 12(D6) and GPIO 13(D7)
OutputDevice device_list[] = {
      {"Tubelight", D5, false},
//      {"TestLED", LED_BUILTIN, true},
};
int NUM_DEVICES = sizeof(device_list)/sizeof(device_list[0]);

void wifiSetup(){
  // Initialise wifi connection
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect("ESPSmartDevice");
  Serial.print("Wifi Connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void otaSetup(){
  //Handle OTA
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}


void setup() {

    // Initialize serial port 
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();
    Serial.print("Number of devices -> ");
    Serial.println(NUM_DEVICES);
    
    // Initialize 512 bytes of eeprom
    EEPROM.begin(512);
    
    // Initialize devices and their states
    for(int i=0; i<NUM_DEVICES; i++){
      Serial.print("Initializing : ");
      Serial.print(device_list[i].device_name);

      pinMode(device_list[i].pin, OUTPUT);

      // Initialize the device states
      if(EEPROM.read(i) == 0){
          digitalWrite(device_list[i].pin, device_list[i].toggle_output? HIGH : LOW);
          Serial.println(" OFF");
      }
      else{
          digitalWrite(device_list[i].pin, device_list[i].toggle_output? LOW : HIGH);
          Serial.println(" ON");
      }
    }

    // Initialize Wifi
    wifiSetup();

    // Initialize OTA
    otaSetup();

    // Initialize Fauxmo
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices
    fauxmo.enable(true);

    for(int i=0; i<NUM_DEVICES; i++){
      fauxmo.addDevice(device_list[i].device_name);
    }

    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
        for(int i=0; i<NUM_DEVICES; i++){
          if (strcmp(device_name, device_list[i].device_name)==0){
            if(state){//ON
              EEPROM.write(i, 1);
              digitalWrite(device_list[i].pin, device_list[i].toggle_output? LOW : HIGH);
            }
            else{//OFF
              EEPROM.write(i, 0);
              digitalWrite(device_list[i].pin, device_list[i].toggle_output? HIGH : LOW);
            }
          }
        }
        EEPROM.commit();
    });
}

void loop() {
    fauxmo.handle();
    ArduinoOTA.handle();
    delay(1);
}
