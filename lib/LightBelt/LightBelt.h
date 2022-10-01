#ifndef LIGHT_BELT_H
#define LIGHT_BELT_H

#include "Arduino.h"

#define DEAD 0
#define PENDING_ON 1
#define PENDING_OFF 2
#define PENDING_ADJUST_LUMINANCE 3
#define PENDING_SET_LUMINANCE 4
#define IDLE 5

typedef void onoffFeedback(bool);
typedef void luminanceFeedback(uint8_t);

class LightBelt
{
private:
    int16_t luminance=0, new_luminance;
    uint8_t pin_warm, pin_cold;
    bool onoff;
    uint8_t state = DEAD;
    onoffFeedback *myOnoffFeedback;
    luminanceFeedback *myLuminanceFeedback;
public:
    LightBelt(/* args */);
    ~LightBelt();
    void setPin(uint8_t, uint8_t);
    void attachCallback(onoffFeedback, luminanceFeedback);
    void begin();
    void init();
    uint8_t getLuminance();
    bool getOnOff();
    void turnOn();
    void turnOff();
    void switchOnOff();
    void setLuminance(uint8_t);
    void ajustLuminance(int8_t);
    void loop();
};

#endif