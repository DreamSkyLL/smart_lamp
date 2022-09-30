#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "LightBelt.h"

// MQTT Broker
const char *mqtt_broker = "192.168.50.230";
const char *topic_state_onoff = "home/bedroom/light/0000/status";
const char *topic_set_onoff = "home/bedroom/light/0000/switch";
const char *topic_state_brightness = "home/bedroom/light/0000/brightness";
const char *topic_set_brightness = "home/bedroom/light/0000/brightness/set";

// LIGHT
const uint8_t SWITCH_PIN = 12;
const uint8_t SUB_PIN = 14;
const uint8_t ADD_PIN = 16;
const uint8_t PWM_WARM_PIN = 4;
const uint8_t PWM_COLD_PIN = 5;

// WiFi
char mqtt_server[40];
char mqtt_port[6] = "1883";
WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);

bool switch_flag = false;
uint8_t timer = 0;
uint32_t clicktime = 0;
WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient client(wifiClient);
LightBelt Light;

IRAM_ATTR void ClockChanged() {
    if (digitalRead(SUB_PIN) == HIGH) { 
        if (digitalRead(ADD_PIN) == LOW) Light.ajustLuminance(4);
        else Light.ajustLuminance(-4);
    } else { 
        if (digitalRead(ADD_PIN) == HIGH) Light.ajustLuminance(4);
        else Light.ajustLuminance(-4);
    }
    timer = 0;
}

// SW
IRAM_ATTR void SwitchChanged() {
    if (digitalRead(SWITCH_PIN) == LOW) {
        clicktime = millis();
        switch_flag = true;
        while(digitalRead(SWITCH_PIN) == LOW);
        // Light.switchOnOff();
    }
    timer = 0;
}

// MQTT callback
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic:");
    Serial.println(topic);
    if (strcmp(topic, topic_set_onoff) == 0) {
        if (length == 2) Light.turnOn();
        else if (length == 3) Light.turnOff();
    }
    if (strcmp(topic, topic_set_brightness) == 0) {
        char* p = new char[length];
        memcpy(p, payload, length);
        Serial.print(atoi(p));
        Light.setLuminance(atoi(p));
    }
    Serial.print("Message:");
    char* p = new char[length];
    memcpy(p, payload, length);
    Serial.println(p);
}

// MQTT reconnect
void reconnect() {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Trying to reconnect MQTT server, result: ");
        // Attempt to connect
        if (client.connect(client_id.c_str())) {
            Serial.println("connected");
            client.subscribe(topic_set_onoff);
            client.subscribe(topic_set_brightness);
        } else {
            Serial.print("failed with state ");
            Serial.println(client.state());
        }
}

// 
void onoffCallback(bool onoff) {
    char ch[4];
    Serial.print("Onoff callback.\n");
    if (!client.connected()) reconnect();
    client.publish(topic_state_onoff, Light.getOnOff() ? "ON" : "OFF");
    client.publish(topic_state_brightness, itoa(Light.getLuminance(), ch, 10));
}

void luminanceCallback(uint8_t luminance) {
    char ch[4];
    Serial.print("Luminance callback.\n");
    if (!client.connected()) reconnect();
    if (Light.getOnOff()) client.publish(topic_state_brightness, itoa(Light.getLuminance(), ch, 10));
}

void setup() {
    // Init serial
    Serial.begin(115200);
    Serial.println("Serial init.");

    // Init WiFi
    // WiFi.mode(WIFI_STA);
    // WiFi.setAutoReconnect(true);
    // WiFi.persistent(true);
    // WiFi.setPhyMode(WIFI_PHY_MODE_11B);
    // WiFi.begin(ssid, password);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    //  wifiManager.resetSettings();
    wifiManager.setTimeout(30);
    wifiManager.autoConnect("AutoConnectAP");
    Serial.println("WiFi init.");

    // Init MQTT
    client.setServer(mqtt_broker, atoi(mqtt_port));
    Serial.print(mqtt_broker);Serial.println(atoi(mqtt_port));
    client.setCallback(callback);
    Serial.println("MQTT init.");

    // Init light
    Light.attachCallback(onoffCallback, luminanceCallback);
    Light.setPin(PWM_WARM_PIN, PWM_COLD_PIN);
    pinMode(SWITCH_PIN, INPUT);
    pinMode(SUB_PIN, INPUT);
    pinMode(ADD_PIN, INPUT);
    Serial.println("Init finished.");

    // Init Interrupt
    attachInterrupt(SUB_PIN, ClockChanged, CHANGE);
    attachInterrupt(SWITCH_PIN, SwitchChanged, FALLING);
}

void loop() {
    client.loop();
    Light.loop();
    

    if (timer > 90 && timer < 100) {
        Light.setLuminance(Light.getLuminance());
        timer = 100;
    } else if (timer > 100) {
        timer = 100;
    }

    
    if (millis() - clicktime > 5000 && switch_flag) {
        switch_flag = false;
        wifiManager.resetSettings();
        ESP.reset();
    } else if (switch_flag) {
        Light.switchOnOff();
        switch_flag = false;
    }

    timer++;
    delay(1);
}

