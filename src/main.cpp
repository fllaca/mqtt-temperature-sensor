// Including the ESP8266 WiFi library
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include "secrets.h"

// Data wire is plugged into pin D1 on the ESP8266 12-E - GPIO 5
#define ONE_WIRE_BUS 5
#define MAX_RETRIES 10

#define SECONDS 1000

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);

// Web Server on port 80
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server(80);

char ip[20] = "";
long lastMsg = 0;
float lastTempC = 0;
char temperatureCString[7];
char lastState[50] = "";

void updateTemperature() {
  int retries = 0;
  float tempC = 0;
  do {
    
    DS18B20.requestTemperatures(); 
    tempC = DS18B20.getTempCByIndex(0);
    Serial.println(tempC);
    if (tempC == lastTempC) {
      break;
    } else {
      lastTempC = tempC;
      dtostrf(tempC, 2, 2, temperatureCString);
      delay(100);

      sprintf(lastState, "{\"celsius\": %s, \"IP\": \"%s\"}", temperatureCString, ip);
      
      Serial.println(lastState);

      client.publish("temperature/living", lastState);
    }
  } while ((tempC == 85.0 || tempC == (-127.0)) && retries < MAX_RETRIES);
}

void handleRootPath() {            
  //Handler for the rooth path
  Serial.println("New client");
  server.send(200, "application/json", lastState);
}

// only runs once on boot
void setup() {
  // Initializing serial port for debugging purposes
  Serial.begin(9600);
  delay(10);

  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  
  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Starting the web server
  Serial.println("Web server running. Waiting for the ESP IP...");
  delay(2 * SECONDS);
  
  // Printing the ESP IP address
  WiFi.localIP().toString().toCharArray(ip, 20);
  Serial.println(ip);

  server.on("/", handleRootPath);
  server.begin();

  // Set MQTT server
  client.setServer(mqtt_server, 1883);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// runs over and over again
void loop() {
  // Listenning for new clients
  server.handleClient();

  if (!client.connected()) {
    reconnect();
  }

  long now = millis();
  if (now - lastMsg > 15 * SECONDS) {
    lastMsg = now;
    updateTemperature();
  }

  client.loop();
}   