#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include "LightBelt.h"
#include "Settings.h"
#include "TimedTask.h"

// MQTT Broker
const char *topic_state_onoff = "home/bedroom/light/0000/status";
const char *topic_set_onoff = "home/bedroom/light/0000/switch";
const char *topic_state_brightness = "home/bedroom/light/0000/brightness";
const char *topic_set_brightness = "home/bedroom/light/0000/brightness/set";
const char *topic_set_timedtask = "home/bedroom/light/0000/timedtask/set";

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
bool shouldSaveConfig = false;

Settings MySettings;
WiFiManager wifiManager;
WiFiClient wifiClient;
PubSubClient client(wifiClient);
LightBelt Light;
WiFiUDP ntpUDP;
NTPClient timeClient = NTPClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);
TimedTask timedTask(timeClient);

//
IRAM_ATTR void ClockChanged()
{
    if (digitalRead(SUB_PIN) == HIGH)
    {
        if (digitalRead(ADD_PIN) == LOW)
            Light.ajustLuminance(4);
        else
            Light.ajustLuminance(-4);
    }
    else
    {
        if (digitalRead(ADD_PIN) == HIGH)
            Light.ajustLuminance(4);
        else
            Light.ajustLuminance(-4);
    }
    timer = 0;
}

// SW Interrupt
IRAM_ATTR void SwitchChanged()
{
    if (digitalRead(SWITCH_PIN) == LOW)
    {
        clicktime = millis();
        switch_flag = true;
        while (digitalRead(SWITCH_PIN) == LOW)
            ;
    }
    timer = 0;
}

//
void taskOn()
{
    Light.turnOn();
}
void taskOff()
{
    Light.turnOff();
}

// WiFiManager save config callback
void saveConfigCallback()
{
    shouldSaveConfig = true;
}

// MQTT callback
void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.printf("Message arrived in topic: %s\n", topic);
    if (strcmp(topic, topic_set_onoff) == 0)
    {
        if (length == 2)
            Light.turnOn();
        else if (length == 3)
            Light.turnOff();
    }
    else if (strcmp(topic, topic_set_brightness) == 0)
    {
        char *p = new char[length];
        memcpy(p, payload, length);
        Serial.print(atoi(p));
        Light.setLuminance(atoi(p));
    }
    else if (strcmp(topic, topic_set_timedtask) == 0)
    {
        DynamicJsonDocument doc(1024);
        char *p = new char[length];
        memcpy(p, payload, length);
        deserializeJson(doc, p);
        Serial.println("deserializeJson().");
        JsonObject obj = doc.as<JsonObject>();
        if (strcmp("SW", obj["opt"]) == 0)
        {
            Serial.println("In SW handler.");
            if (PERIODIC == int(obj["type"]))
            {
                String value = obj["val"];
                Serial.println("In periodic");
                if (value.length() == 2)
                    timedTask.addTask(Task(taskOn, PERIODIC, time_t(obj["time"]), time_t(obj["interval"])));
                else if (value.length() == 3)
                    timedTask.addTask(Task(taskOff, PERIODIC, time_t(obj["time"]), time_t(obj["interval"])));
            }
            else if (DISPOSABLE == int(obj["type"]))
            {
                String value = obj["val"];
                Serial.println("In disposable");
                Serial.println(time_t(obj["time"]));
                if (value.length() == 2)
                    timedTask.addTask(Task(taskOn, DISPOSABLE, time_t(obj["time"])));
                else if (value.length() == 3)
                    timedTask.addTask(Task(taskOff, DISPOSABLE, time_t(obj["time"])));
            }
        }
    }
    Serial.printf("Message: %s\n", payload);
}

// MQTT reconnect
void reconnect()
{
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Trying to reconnect MQTT server, result: ");
    // Attempt to connect
    if (client.connect(client_id.c_str()))
    {
        Serial.println("connected");
        client.subscribe(topic_set_onoff);
        client.subscribe(topic_set_brightness);
        client.subscribe(topic_set_timedtask);
    }
    else
    {
        Serial.print("failed with state ");
        Serial.println(client.state());
    }
}

// Switch Callback
void onoffCallback(bool onoff)
{
    char ch[4];
    Serial.print("Onoff callback.\n");
    if (!client.connected())
        reconnect();
    client.publish(topic_state_onoff, Light.getOnOff() ? "ON" : "OFF");
    client.publish(topic_state_brightness, itoa(Light.getLuminance(), ch, 10));
}

// Set luminance Callback
void luminanceCallback(uint8_t luminance)
{
    char ch[4];
    Serial.print("Luminance callback.\n");
    if (!client.connected())
        reconnect();
    if (Light.getOnOff())
        client.publish(topic_state_brightness, itoa(Light.getLuminance(), ch, 10));
}

void setup()
{
    // Init serial
    Serial.begin(115200);
    Serial.println("Serial init.");

    // Init WiFi
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.setTimeout(60);
    wifiManager.autoConnect("AutoConnectAP");
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    Serial.println("WiFi init.");
    if (shouldSaveConfig)
        MySettings.update(mqtt_server, mqtt_port);

    // Init Settings
    MySettings.begin();
    MySettings.load(mqtt_server, mqtt_port);
    Serial.printf("Setting server: %s, Setting port: %s.\n", mqtt_server, mqtt_port);

    // Init MQTT
    client.setServer(mqtt_server, atoi(mqtt_port));
    client.setCallback(callback);
    Serial.printf("MQTT init: %s, %d\n", mqtt_server, atoi(mqtt_port));

    // Init light
    Light.attachCallback(onoffCallback, luminanceCallback);
    Light.setPin(PWM_WARM_PIN, PWM_COLD_PIN);
    pinMode(SWITCH_PIN, INPUT);
    pinMode(SUB_PIN, INPUT);
    pinMode(ADD_PIN, INPUT);
    Serial.println("Init finished.");

    // Init NTP
    timeClient.begin();

    // Attach interrupt
    attachInterrupt(SUB_PIN, ClockChanged, CHANGE);
    attachInterrupt(SWITCH_PIN, SwitchChanged, FALLING);
}

void loop()
{
    client.loop();
    Light.loop();
    timedTask.loop();
    timeClient.update();

    // Delay pub MQTT
    if (timer > 90 && timer < 100)
    {
        Light.setLuminance(Light.getLuminance());
        timer = 100;
    }
    else if (timer > 100)
    {
        timer = 100;
    }

    // Long press
    if (millis() - clicktime > 5000 && switch_flag)
    {
        switch_flag = false;
        wifiManager.resetSettings();
        ESP.reset();
    }
    else if (switch_flag)
    {
        Light.switchOnOff();
        switch_flag = false;
    }

    timer++;
    delay(1);
}
