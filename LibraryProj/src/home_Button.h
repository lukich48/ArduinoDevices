// Button.h

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "home_Sender.h"

class Button {
public:
    virtual void interruptButtton()=0;
    virtual void mqttCallback(char* topic, byte* payload, unsigned int length)=0;
    virtual void setup()=0;   
    virtual void handle()=0;
    virtual void setSender(Sender& sender)=0;

	string buttonName;
    string topicSwitch; //команда преключени¤ реле
	string topicSwitchState; //команда проверки статуса реле
	string topicSwitchSetup; //настройка кнопки
    byte buttonPin = -1; //-1 - нет физической кнопки
    bool levelButton = LOW; // —игнал в нормальном состо¤нии на кнопке или датчике касани¤


};