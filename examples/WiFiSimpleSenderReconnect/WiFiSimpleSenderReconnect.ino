/*
  ArduinoMqttClient - WiFi Reconnect

  This example connects to a MQTT broker and publishes a message to
  a topic once a second. If the client is disconnected from the broker,
  it automatically tries to reconnect. 
  

  The circuit:
  - Arduino MKR 1000, MKR 1010 or Uno WiFi Rev.2 board


  This example code is in the public domain.
*/

#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // for MKR1000 change to: #include <WiFi101.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate 
//    flashed in the WiFi module.

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "test.mosquitto.org";
int        port     = 1883;
const char topic[]  = "arduino/simple";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

int attemptReconnect(){
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

  }
  return mqttClient.connectError(); // return status
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  // attempt to connect to Wifi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    
    delay(5000);
  }


  Serial.println("You're connected to the network");
  Serial.println();

  
  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("14");

  // You can provide a username and password for authentication
  // mqttClient.setUsernamePassword(SECRET_MQTT_USER, SECRET_MQTT_PW);
  

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  
  
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }
  

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

}

void loop() {
  // avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    mqttClient.poll(); // poll to avoid beeing disconnected
    

    Serial.print("Sending message to topic: ");
    Serial.println(topic);
    Serial.print("hello ");
    Serial.println(count);

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print("hello ");
    mqttClient.print(count);
    mqttClient.endMessage();
   
    Serial.println();

    count++;

    if(!mqttClient.connected()){ // if the client has been disconnected, 
      Serial.println("Client disconnected, attempting reconnection");
      Serial.println();
    
      if(!attemptReconnect()){ // try reconnecting
        Serial.print("Client reconnected!");
        Serial.println();
    
      }
    }
  }
}
