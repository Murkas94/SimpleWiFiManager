#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <SimpleWiFiManager.h>         //https://github.com/tzapu/WiFiManager

bool Connected = false;
SimpleWiFiManager* wifiManager;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
}

void loop() {
    if(Connected){
        //Check if the wifi-status is still connected
        if(WiFi.status() != WL_CONNECTED){
            //wifi is not connected anymore, restart
            wifiManager = new SimpleWiFiManager();

            //set custom ip for portal
            //wifiManager->setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

            //fetches ssid and pass from eeprom and tries to connect
            //if it does not connect it starts an access point with the specified name
            //here  "AutoConnectAP"
            Connected = wifiManager->autoConnect("AutoConnectAP"));
            //or use this for auto generated name ESP + ChipID
            //Connected = wifiManager->autoConnect();
        }
    }else{
        //wifi is not connected, but is connecting
        if(wifiManager->HandleConnecting()){
            //Connection was successful, clean up
            Connected = true;
            delete wifiManager;
            wifiManager = nullptr;
            Serial.println("connected...yeey :)");
        }
    }
    // put your main code here, to run repeatedly:
    delay(100);
}
