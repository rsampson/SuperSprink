/*
  Basic ESP8266 MQTT example from:
  https://thenewstack.io/off-the-shelf-hacker-automated-yard-watering-project/
  https://thenewstack.io/off-the-shelf-hacker-adding-mqtt-and-cron-to-the-lawn-sprinkler-project/
  Relay board data at:
  http://wiki.sunfounder.cc/index.php?title=8_Channel_5V_Relay_Module&utm_source=thenewstack&utm_medium=website

  Test with:   mosquitto_pub -h 192.168.1.111 -t inTopic -m 1

  Sprinkler station connections:
  relay pin  NodeMCU GPIO  NodeMCU physical pin
  N1          GPIO16        D0
  N2          GPIO5         D1
  N3          GPIO4         D2
  N4          GPIO0         D3
  N5          GPIO2         D4
  N6          GPIO14        D5
  N7          GPIO12        D6
  N8          GPIO13        D7
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include "password.h"   // keep your private keys and passwords in this format:
/*
 * EXAMPLE:
  const char* ssid = "xxxx";
  const char* password = "xxxx";
  const char* APIKey = "xxxx";
  #define MYCHANNEL xxxx
  const char* mqtt_server = "xxxx";
*/

#define MAXTIME 20 // maximum minutes sprinklers can be left on.

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void allOff() { // turn all sprinkler stations off
  digitalWrite(16, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(14, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
}

void setup_wifi() {

  delay(10);
  // Connect to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  allOff(); // only allow one station on at a time
  allOff();
  allOff();

  bool ran = false;
  
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(16, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '2') {
    digitalWrite(5, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '3') {
    digitalWrite(4, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '4') {
    digitalWrite(0, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '5') {
    digitalWrite(2, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '6') {
    digitalWrite(14, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '7') {
    digitalWrite(12, LOW);
    ran = true;
  }
  else if ((char)payload[0] == '8') {
    digitalWrite(13, LOW);
    ran = true;
  }
  else {
    allOff();
    allOff();
    allOff();
  }
  
  if(ran){
    // echo message received back to mqtt server
    snprintf (msg, 50, "activate #%ld", payload[0]);
    client.publish("outTopic", msg);
  }
  
  lastMsg = millis(); // for max timeout
}

void reconnect() {
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    client.publish("outTopic", "hello world");
    // ... and resubscribe
    client.subscribe("inTopic");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 250 ms");
    delay(250);
  }
}

void setup() {
  pinMode(16, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  allOff();
  allOff();
  allOff()
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // *** OTA stuff ***

  ArduinoOTA.setHostname("sprinkler8266_test");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });

  ArduinoOTA.begin();

}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // after MAXTIME minutes, shut down all stations
  long now = millis();
  if (now - lastMsg > (MAXTIME * 60 * 1000)) {
    lastMsg = now;
    ++value;
    // send out a 'heart beat' message
    snprintf (msg, 50, "hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic", msg);
    allOff();
  }

  ArduinoOTA.handle();
}
