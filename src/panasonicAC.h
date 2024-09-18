#ifndef PANASONIC_AC_H
#define PANASONIC_AC_H
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRsend.h>
#include <IRtext.h>
#include <assert.h>

const uint16_t kIrled = 4; //ESP8266 pin D2
IRPanasonicAc ac(kIrled);
//State of the AC Panasonic
class statePanasonic {
public:
    statePanasonic(){
    }
    void setTemp(uint8_t tempAC){
        temp = tempAC;
    }

    void setFan(std::string fanAC){
        if(fanAC == "Low"){
            fan = kPanasonicAcFanLow;
        }
        else if(fanAC == "Medium"){
            fan = kPanasonicAcFanMed;
        }
        else if(fanAC == "High"){
            fan = kPanasonicAcFanHigh;
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
            mode = kPanasonicAcAuto;
        }
        else if(modeAC == "Cool"){
            mode = kPanasonicAcCool;
        }
        else if(modeAC == "Dry"){
            mode = kPanasonicAcDry;
        }
        else if(modeAC == "Heat"){
            mode = kPanasonicAcHeat;
        }
        else{
            Serial.println("Invalid value for Fan");
        }
    }

    std::string fanToString(){
        switch(fan){
            case kPanasonicAcFanLow:
                return "Low";
            case kPanasonicAcFanMed:
                return "Medium";
            case kPanasonicAcFanHigh:
                return "High";
            default:
                return "Medium";
        }
    } 

    std::string modeToString(){
        switch(mode){
            case kPanasonicAcAuto:
                return "Auto";
            case kPanasonicAcCool:
                return "Cool";
            case kPanasonicAcHeat:
                return "Heat";
            case kPanasonicAcDry:
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
            case kPanasonicAcAuto:
                ac.setMode(kPanasonicAcAuto);
                break;
            case kPanasonicAcHeat:
                ac.setMode(kPanasonicAcHeat);
                break;
            case kPanasonicAcCool:
                ac.setMode(kPanasonicAcCool);
                break;
            case kPanasonicAcDry:
                ac.setMode(kPanasonicAcDry);
                break;
        }
        switch (fanAC){
            case kPanasonicAcFanLow:
                ac.setFan(kPanasonicAcFanLow);
                break;
            case kPanasonicAcFanMin:
                ac.setFan(kPanasonicAcFanMin);
                break;
            case kPanasonicAcFanHigh:
                ac.setFan(kPanasonicAcFanHigh);
                break;
            case kPanasonicAcFanAuto:
                ac.setFan(kPanasonicAcFanAuto);
                break;
        }
        ac.setTemp(tempAC);
        ac.send();
        Serial.println("Sending A/C done");
        delay(2000);
    }
};
statePanasonic acState;
#endif