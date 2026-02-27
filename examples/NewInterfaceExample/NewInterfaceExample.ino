/*
 * The following example is added for testing purposes and may be deleted before publishing the PR.
 * This had been tested with mosquitto with the following config files inside of a local directory `/mosquitto/config`
 * ```
 * port 1883
 * allow_anonymous true
 * password_file /mosquitto/config/passwd
 * ```
 * passwd file containing auth arduino:arduino:
 * ```
 * arduino:$7$101$CkoJckWT8WWX9+Z1$mzMdh0TD99P41AEAT55GXDIPf8FLKXp7LS3BTpEvhBUnXMYj4myWSGMH7aShCufuvsbtUXChk+KCW3AncUgtuA==
 * ```
 * run with docker:
 * ```
 * docker run -it -p 1883:1883 -v "${PWD}/mosquitto/config:/mosquitto/config" eclipse-mosquitto
 * ```
 */

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <Ethernet.h>

uint8_t payload[] = {
  0xda, 0x00, 0x01, 0x00, 0x00, 0x81, 0x58, 0x20,
  0x1c, 0x05, 0xeb, 0xfa, 0xee, 0x5a, 0x38, 0xa5,
  0x44, 0x05, 0xe9, 0x1b, 0xaa, 0xeb, 0xeb, 0x40,
  0x68, 0x3e, 0xad, 0x1f, 0x93, 0x90, 0xd8, 0x57,
  0x4a, 0xe3, 0x35, 0x9d, 0x84, 0xc9, 0x0f, 0x64,
};

char topic[] = "/a/d/f334982b-032f-40e9-a91a-99da31a90dbe/c/up";
uint32_t start;
int i = 0;

MqttClient client;
// ZephyrMqttClient client;
volatile bool packet = false;

void mqtt_cbk(const char* topic) {
  Serial.print("Received a message on topic: \"");
  Serial.print(topic);
  // Serial.print("\" with payload: 0x");

  // while(stream.available() > 0) {
  //   int res = stream.read();

  //   // if(res < 0x10) {
  //   //   Serial.print('0');
  //   // }
  //   // Serial.print(res, HEX);
  // }

  // while(client.available() > 0) {
  //   int res = client.read();

  //   if(res < 0x10) {
  //     Serial.print('0');
  //   }
  //   Serial.print(res, HEX);
  // }
  // packet = true;

  Serial.println();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("begin ");
  Serial.println(i);
	// int err = tls_credential_add(APP_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
	// 			 ca_certificate, sizeof(ca_certificate));
	// if (err < 0) {
	// 	DEBUG_ERROR("Failed to register public certificate: %d", err);
	// }

	// err = tls_credential_add(APP_KEYS, TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
	// 			 client_public_key, sizeof(client_public_key));
	// if (err < 0) {
	// 	DEBUG_ERROR("Failed to register puiblic key: %d", err);
	// }

	// err = tls_credential_add(APP_KEYS, TLS_CREDENTIAL_PRIVATE_KEY,
	// 			 client_private_key, sizeof(client_private_key));
	// if (err < 0) {
	// 	DEBUG_ERROR("Failed to register private key: %d", err);
	// }

  while (Ethernet.linkStatus() != LinkON) {
    Serial.println("waiting for link on");
    delay(100);
  }
  Ethernet.begin();

  Serial.println(Ethernet.localIP());

  client.setId("f334982b-032f-40e9-a91a-99da31a90dbe");
  client.setUsernamePassword("arduino", "arduino");

  client.setReceiveCallback(mqtt_cbk);

	// int res = client.connect("iot.arduino.cc", 8885);
	int res = client.connect(IPAddress(192, 168, 10, 250), 1883);
  // int res = client.connect("test.mosquitto.org", 1883);

	Serial.print("connection result: ");
	Serial.println(res);

  res = client.subscribe("test0", MqttQos0);
  res = client.subscribe("test1", MqttQos1);
  res = client.subscribe("test2", MqttQos2);

  Serial.print("subscribe result: ");
	Serial.println(res);

  start = millis();
}
void loop() {
  client.poll();

  if(client.available() > 0) {
    Serial.println("packet received");
    Serial.println(client.available());

    while(client.available() > 0) {
      int res = client.read();

      if(res < 0x10) {
        Serial.print('0');
      }
      Serial.print(res, HEX);
    }

    packet = false;
  }

  if(millis() - start > 10000) {
    int res = client.publish(
      topic,
      payload,
      sizeof(payload)
    );

    Serial.print("publish res: ");
    Serial.println(res);

    start = millis();
  }
  delay(100);
}
