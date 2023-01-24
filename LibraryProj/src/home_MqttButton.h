#pragma once
#include "arduino.h"

#include "home_Button.h"
#include "home_Sender.h"
#include <RBD_Timer.h>
#include <vector>
#include <string>
using std::string;

class MqttButton: public Button {
public:
	MqttButton(uint8_t buttonPin, uint8_t relayPin, string buttonName);
	void interruptButtton() override;
	void mqttCallback(char* topic, uint8_t* payload, unsigned int length) override;
	void setup() override;
	void handle() override;
	void setSender(Sender& sender) override;
	
	void btnPress();
	void btnHold(int duration);
	bool getState();
	void addTopic(string topic);
	uint8_t relayPin;
	bool levelTrigger = HIGH; //сигнал срабатывани¤ реле
	bool isHoldButton; //hold - поведение по умолчанию

	int lockTimout = 70;
	int lockTimout2 = 90;
	int holdTimeout = 10000;

private:
	volatile boolean _lock = false;
	std::vector<string> _publishTopics;

	volatile boolean _flagChange = false; // ‘лаг нужен дл¤ того, чтобы опубликовать сообщение на брокер
	void relaySwitch(bool state);

	RBD::Timer _lockTimer; // защита от дребезга до
	RBD::Timer _lockTimer2; // защита от дребезга после

	RBD::Timer _holdTimer; // удержание кнопки
	boolean _flagHold;

	void onTopicSwitch(uint8_t* payload, unsigned int length);
	void onTopicSwitchState(uint8_t* payload, unsigned int length);
	void onTopicSwitchSetup(uint8_t* payload, unsigned int length);

	void holdStart();
	void holdStop();
};