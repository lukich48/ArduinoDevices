// MqttButton.h

#ifndef _MQTTBUTTON_h
#define _MQTTBUTTON_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "home_Sender.h"
#include <RBD_Timer.h>
#include <vector>
#include <string>
using namespace std;

class MqttButton {
public:
	MqttButton(byte buttonPin, byte relayPin, string buttonName);
	void btnPress();
	void btnHold(int duration);
	void interruptButtton();
	void mqttCallback(char* topic, byte* payload, unsigned int length);
	void setup();
	void handle();
	bool getState();
	void addTopic(string topic);
	void setSender(Sender& sender);
	byte buttonPin = -1; //-1 - нет физической кнопки
	byte relayPin;
	string buttonName;
	bool levelButton = LOW; // —игнал в нормальном состо¤нии на кнопке или датчике касани¤
	bool levelTrigger = HIGH; //сигнал срабатывани¤ реле
	string topicSwitch; //команда преключени¤ реле
	string topicSwitchState; //команда проверки статуса реле
	string topicSwitchSetup; //настройка кнопки
	bool isHoldButton; //hold - поведение по умолчанию

	int lockTimout = 30;
	int lockTimout2 = 90;
	int holdTimeout = 10000;

private:
	volatile boolean _lock = false;
	vector<string> _publishTopics;
	Sender* _sender;

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

#endif

