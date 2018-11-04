// Including the ESP8266 WiFi library
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

#include <ConfigManager.h>

// Data wire is plugged into pin D6 on the NodeMCU v3
#define ONE_WIRE_BUS D5
#define MAX_RETRIES 10
#define SECONDS 1000

#define BUTTON_PIN D2
#define LED_PIN D1

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature DS18B20(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);
ConfigManager configManager;

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
      
      Serial.println("Publishing state to");
      Serial.print(configManager.mqtt_topic);
      Serial.print(": "); 
      Serial.println(lastState);

      client.publish(configManager.mqtt_topic, lastState);
    }
  } while ((tempC == 85.0 || tempC == (-127.0)) && retries < MAX_RETRIES);
}

void handleRootPath() {            
  //Handler for the rooth path
  Serial.println("New client");
  //server.send(200, "application/json", lastState);
}

void ledOn(){
  digitalWrite(LED_PIN, HIGH);
}

void ledOff(){
  digitalWrite(LED_PIN, LOW);
}

// only runs once on boot
void setup() {
  // Initializing serial port for debugging purposes
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  ledOff();
  delay(5);

  DS18B20.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  
    
  WiFi.mode(WIFI_STA);

  configManager.readConfig();
  
  // Try Connecting to WiFi network
  ledOn();
  configManager.autoConnect();
  ledOff();
  Serial.println("");
  Serial.println("WiFi connected");
 
  delay(2 * SECONDS);
  
  // Printing the ESP IP address
  WiFi.localIP().toString().toCharArray(ip, 20);
  Serial.println(ip);
}

void reconnect() {

  // Set MQTT server
  client.setServer(configManager.mqtt_server, 1883);

  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str(), configManager.mqtt_user, configManager.mqtt_password)) {
    Serial.println("connected");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5 * SECONDS);
  }

}

// runs over and over again
void loop() {
  
  // if button pressed, start configuration mode
  if ( digitalRead(BUTTON_PIN) == HIGH ) {
    ledOn();
    Serial.println("Now reconfigure");
    delay(3 * SECONDS);
    configManager.startConfigMode();
    ledOff();
  }

  if (!client.connected()) {
    reconnect();
  } else {
    long now = millis();
    if (now - lastMsg > 15 * SECONDS) {
      lastMsg = now;
      updateTemperature();
    }
    
    client.loop();
  }
}   