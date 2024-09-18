#ifndef MITSUBISHI_AC_H
#define MITSUBISHI_AC_H
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRsend.h>
#include <IRtext.h>
#include <assert.h>

const uint16_t kIrled = 4; //ESP8266 pin D2
IRPanasonicAc ac(kIrled);
//State of the AC Panasonic
class stateMitsubishi {
public:
    statePanasonic(){
    }
    void setTemp(uint8_t tempAC){
        temp = tempAC;
    }

    void setFan(std::string fanAC){
        if(fanAC == "Low"){
            fan = kMitsubishiAcFanSilent;
        }
        else if(fanAC == "Medium"){
            fan = kMitsubishiAcFanQuiet;
        }
        else if(fanAC == "High"){
            fan = kMitsubishiAcFanMax;
        }
        else{
            Serial.println("Invalid value for Fan");
        }
    }

    void setPower(std::string powerAC){
        if(powerAC == "on"){
            power = 1;
        }
        else if(powerAC == "off"){
            power = 0;
        }
        else{
            Serial.println("Invalid value for Power");
        }
    }

    void setMode(std::string modeAC){
        if(modeAC == "Auto"){
            mode = kMitsubishiAcAuto;
        }
        else if(modeAC == "Cool"){
            mode = kMitsubishiAcCool;
        }
        else if(modeAC == "Dry"){
            mode = kMitsubishiAcDry;
        }
        else if(modeAC == "Heat"){
            mode = kMitsubishiAcHeat;
        }
        else{
            Serial.println("Invalid value for Fan");
        }
    }

    std::string fanToString(){
        switch(fan){
            case kMitsubishiAcFanSilent:
                return "Low";
            case kMitsubishiAcFanQuiet:
                return "Medium";
            case kMitsubishiAcFanMax:
                return "High";
            default:
                return "Medium";
        }
    } 

    std::string modeToString(){
        switch(mode){
            case kMitsubishiAcAuto:
                return "Auto";
            case kMitsubishiAcCool:
                return "Cool";
            case kMitsubishiAcHeat:
                return "Heat";
            case kMitsubishiAcDry:
                return "Dry";
            default: 
                return "Auto";
        }
    }

    std::string powerToString(){
        if(power){
            return "On";
        }
        else{
            return "Off";
        }
    }

    uint8_t temp = 25, fan = 1, mode = 3;
    bool power = 0;

    void controlAC(bool powerAC, uint8_t tempAC, uint8_t modeAC, uint8_t fanAC){
        if(powerAC){
            ac.on();
        }
        else{
            ac.off();
        }
        switch (modeAC){
            case kMitsubishiAcAuto:
                ac.setMode(kMitsubishiAcAuto);
                break;
            case kPanasonicAcHeat:
                ac.setMode(kMitsubishiAcHeat);
                break;
            case kPanasonicAcCool:
                ac.setMode(kMitsubishiAcCool);
                break;
            case kPanasonicAcDry:
                ac.setMode(kMitsubishiAcDry);
                break;
        }
        switch (fanAC){
            case kMitsubishiAcFanSilent:
                ac.setFan(kMitsubishiAcFanSilent);
                break;
            case kMitsubishiAcFanQuiet:
                ac.setFan(kMitsubishiAcFanQuiet);
                break;
            case kMitsubishiAcFanMax:
                ac.setFan(kMitsubishiAcFanMax);
                break;
            case kMitsubishiAcFanAuto:
                ac.setFan(kMitsubishiAcFanAuto);
                break;
        }
        ac.setTemp(tempAC);
        ac.send();
        Serial.println("Sending A/C done");
        delay(2000);
    }
};
stateMitsubishi acState;
#endif