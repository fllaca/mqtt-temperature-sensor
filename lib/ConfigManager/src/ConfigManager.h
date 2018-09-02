#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

class ConfigManager {
    public:
        char mqtt_server[40];
        char mqtt_port[6] = "1883";
        char mqtt_user[34];
        char mqtt_password[34];
        char mqtt_topic[34] = "temperature/test";

        void startConfigMode();
        void readConfig();
        void saveConfig();
        void autoConnect();
};