
#pragma once
#include "arduino.h"

#include "home_Sender.h"
using std::string;

class Button {
public:
    virtual void interruptButtton()=0;
    virtual void mqttCallback(char* topic, uint8_t* payload, unsigned int length)=0;
    virtual void setup()=0;   
    virtual void handle()=0;
    virtual void setSender(Sender& sender)=0;

	string buttonName;
    string topicSwitch; //команда преключени¤ реле
	string topicSwitchState; //команда проверки статуса реле
	string topicSwitchSetup; //настройка кнопки
    uint8_t buttonPin = -1; //-1 - нет физической кнопки
    bool levelButton = LOW; // —игнал в нормальном состо¤нии на кнопке или датчике касани¤

protected:
    Sender* _sender;

};