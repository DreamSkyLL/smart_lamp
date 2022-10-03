#ifndef SETTING_H
#define SETTING_H
#include <FS.h>
#include <ArduinoJson.h>

class Settings
{
private:
    char mqtt_server[40];
    char mqtt_port[6];
    bool loadSuccess = false;
public:
    Settings(/* args */);
    ~Settings();
    bool init();
    bool begin();
    void load(char* mqtt_server, char* mqtt_port);
    void format();
    bool update(const char*, const char*);
};

// extern Settings MySettings;


#endif