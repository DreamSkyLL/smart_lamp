#include "LightBelt.h";



LightBelt::LightBelt()
{

}

LightBelt::~LightBelt()
{
}

void LightBelt::setPin(uint8_t pin_cold, uint8_t pin_warm) {
    this->pin_warm = pin_cold;
    this->pin_cold = pin_warm;
}

void LightBelt::attachCallback(onoffFeedback pubOnoff, luminanceFeedback pubLuminance) {
    this->myOnoffFeedback = pubOnoff;
    this->myLuminanceFeedback = pubLuminance;
}

void LightBelt::begin() {
    pinMode(pin_warm, OUTPUT);
    pinMode(pin_cold, OUTPUT);
}

void LightBelt::init() {
    this->begin();
}

uint8_t LightBelt::getLuminance() {
    return this->luminance;
}

bool LightBelt::getOnOff() {
    return this->onoff;
}

void LightBelt::turnOn() {
    this->state = PENDING_ON;
}

void LightBelt::turnOff() {
    this->state = PENDING_OFF;
}

void LightBelt::switchOnOff() {
    this->state = this->onoff ? PENDING_OFF : PENDING_ON;
}

void LightBelt::setLuminance(uint8_t new_luminance) {
    this->state = PENDING_SET_LUMINANCE;
    this->new_luminance = new_luminance;
}

void LightBelt::ajustLuminance(int8_t delta_luminance) {
    this->state = PENDING_ADJUST_LUMINANCE;
    this->new_luminance = this->luminance + delta_luminance;
    if (this->new_luminance < 0) this->new_luminance = 0;
    else if (this->new_luminance > 255) this->new_luminance = 255;
    Serial.println(this->new_luminance);
    Serial.println(delta_luminance);
}

void LightBelt::loop() {
    switch(state) {
        int8_t addminus;
        case PENDING_ON:
            if (this->getOnOff()) break;
            if (this->luminance <= 0) this->luminance = 255;
            for (uint8_t i = 0; i < this->luminance; i++)
            {
                analogWrite(pin_warm, i);
                analogWrite(pin_cold, i);
                delay(1);
            }
            this->onoff = true;
            this->myOnoffFeedback(this->onoff);
            this->state = IDLE;
            break;
        case PENDING_OFF:
            if (!this->getOnOff()) break;
            for (uint8_t i = this->luminance; i > 0; i--)
            {
                analogWrite(pin_warm, i);
                analogWrite(pin_cold, i);
                delay(1);
            }
            analogWrite(pin_warm, 0);
            analogWrite(pin_cold, 0);
            this->onoff = false;
            this->myOnoffFeedback(this->onoff);
            this->state = IDLE;
            break;
        case PENDING_ADJUST_LUMINANCE:
            analogWrite(pin_warm, this->new_luminance);
            analogWrite(pin_cold, this->new_luminance);
            this->luminance = this->new_luminance;

            if (this->luminance <= 0) this->onoff = false;
            else if (this->luminance > 0) this->onoff = true;

            this->state = IDLE;
            break;
        case PENDING_SET_LUMINANCE:
            if (!this->getOnOff()){ 
                this->luminance = this->new_luminance;
                break;
            }
            addminus = this->new_luminance > this->luminance ? 1 : -1;
            for (uint8_t i = this->luminance; addminus > 0 ? i<new_luminance : i>new_luminance; i += addminus)
            {
                analogWrite(pin_warm, i);
                analogWrite(pin_cold, i);
                delay(1);
            }
            this->luminance = this->new_luminance;
            this->myLuminanceFeedback(this->luminance);
            this->state = IDLE;
            break;
        default:
            break;
    }
}