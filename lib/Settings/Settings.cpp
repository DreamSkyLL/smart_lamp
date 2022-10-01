#include "Settings.h"

Settings::Settings() {}

Settings::~Settings() {}

bool Settings::begin()
{
    if (SPIFFS.begin())
    {
        if (SPIFFS.exists("/config.json"))
        {
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                size_t size = configFile.size();
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument json(1024);
                auto deserializeError = deserializeJson(json, buf.get());
                serializeJson(json, Serial);
                if (!deserializeError)
                {
                    strcpy(this->mqtt_server, json["mqtt_server"]);
                    strcpy(this->mqtt_port, json["mqtt_port"]);
                }
                configFile.close();
                this->loadSuccess = true;
            }
        }
    }
    
    return this->loadSuccess;
}

bool Settings::init()
{
    return this->begin();
}

void Settings::load(char* mqtt_server, char* mqtt_port) {
    strcpy(mqtt_server, this->mqtt_server);
    strcpy(mqtt_port, this->mqtt_port);
}

bool Settings::update(const char* mqtt_server, const char* mqtt_port)
{
    DynamicJsonDocument json(1024);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("SETTINGS: fail to update");
        return false;
    }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
    Serial.printf("SETTINGS: %s\n", mqtt_server);
    return true;
}