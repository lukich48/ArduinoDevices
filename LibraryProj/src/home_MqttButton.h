#pragma once
#include "arduino.h"

#include "home_Button.h"
#include "home_Sender.h"
#include <RBD_Timer.h>
#include <vector>
#include <string>
using namespace std;

class MqttButton: public Button {
public:
	MqttButton(byte buttonPin, byte relayPin, string buttonName);
	void interruptButtton() override;
	void mqttCallback(char* topic, byte* payload, unsigned int length) override;
	void setup() override;
	void handle() override;
	void setSender(Sender& sender) override;
	
	void btnPress();
	void btnHold(int duration);
	bool getState();
	void addTopic(string topic);
	byte relayPin;
	bool levelTrigger = HIGH; //сигнал срабатывани¤ реле
	bool isHoldButton; //hold - поведение по умолчанию

	int lockTimout = 30;
	int lockTimout2 = 90;
	int holdTimeout = 10000;

private:
	volatile boolean _lock = false;
	vector<string> _publishTopics;

	volatile boolean _flagChange = false; // ‘лаг нужен дл¤ того, чтобы опубликовать сообщение на брокер
	void relaySwitch(bool state);

	RBD::Timer _lockTimer; // защита от дребезга до
	RBD::Timer _lockTimer2; // защита от дребезга после

	RBD::Timer _holdTimer; // удержание кнопки
	boolean _flagHold;

	void onTopicSwitch(byte* payload, unsigned int length);
	void onTopicSwitchState(byte* payload, unsigned int length);
	void onTopicSwitchSetup(byte* payload, unsigned int length);

	void holdStart();
	void holdStop();
};