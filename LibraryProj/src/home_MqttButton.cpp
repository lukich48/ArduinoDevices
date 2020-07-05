/*
	turn on:
		home/testproj/btn1 1
		home/testproj/btn1 {"action":"on"}
	turn off:
		home/testproj/btn1 0
		home/testproj/btn1 {"action":"off"}
	hold 10 sec:
		home/testproj/btn1 {"action":"hold", "duration":10000}

	subscriptions:
		home/testproj/btn1/state 0
		home/testproj/btn1/state/manual 0
*/

//#include "Arduino.h"
#include "home_MqttButton.h"
#include "home_Sender.h"

#include <RBD_Timer.h>
#include <vector>
#include <string>
#include <ArduinoJson.h>
using namespace std;

MqttButton::MqttButton(byte buttonPin, byte relayPin, string buttonName)
{
	this->buttonPin = buttonPin;
	this->relayPin = relayPin;
	this->buttonName = buttonName;

	pinMode(this->relayPin, OUTPUT);

}

void MqttButton::setup()
{
	_lockTimer.setTimeout(lockTimout);
	_lockTimer2.setTimeout(lockTimout2);

	//выключаем реле
	relaySwitch(LOW);

}

void MqttButton::handle()
{
	// Для прерывания. Если запущен флаг, то публикуем состояние на брокер
	if (_flagChange) {

		int curState = getState();

		//состояние кнопки
		_sender->publish(topicSwitchState, curState, true);
		_sender->publish(topicSwitchState + "/manual", curState, false);

		//отсылаем все топики
		for (int i = 0; i < _publishTopics.size(); i++)
			_sender->publish(_publishTopics[i], curState, false);

		_flagChange = false;
	}

	//для холда
	if (_flagHold) {
		if (_holdTimer.isExpired()) {

			//выключаем
			relaySwitch(false);

			//состояние кнопки
			_sender->publish(topicSwitchState, false, true);

			holdStop();
		}
	}
}

void MqttButton::mqttCallback(char* topic, byte* payload, unsigned int length)
{
	if (strcmp(topic, topicSwitch.c_str()) == 0)
	{
		onTopicSwitch(payload, length);
	}
	else if (strcmp(topic, topicSwitchState.c_str()) == 0)
	{
		onTopicSwitchState(payload, length);
	}
	else if (strcmp(topic, topicSwitchSetup.c_str()) == 0)
	{
		onTopicSwitchSetup(payload, length);
	}
}

void MqttButton::onTopicSwitch(byte* payload, unsigned int length)
{
	//simple request
	if (length == 1) {

		bool val = false;
		if (payload[0] == '1')
			val = true;
		else
			val = false;

		holdStop();

		relaySwitch(val);
	}
	else {

		//deserialize json
		StaticJsonDocument<200> doc;
		DeserializationError error = deserializeJson(doc, payload);

		if (error) {
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.c_str());
			return;
		}

		const char* action = doc["action"];

		// включаем или выключаем реле в зависимоти от полученных значений данных
		if (strcmp(action, "on") == 0)
		{
			holdStop();

			relaySwitch(true);
		}
		else if (strcmp(action, "off") == 0)
		{
			holdStop();

			relaySwitch(false);
		}
		else if (strcmp(action, "hold") == 0)
		{
			long duration = doc["duration"]>0? doc["duration"]: holdTimeout;

			relaySwitch(true);

			_holdTimer.setTimeout(duration);
			_holdTimer.restart();
			_flagHold = true;
		}
	}

	_sender->publish(topicSwitchState, getState(), true);
}

void MqttButton::onTopicSwitchState(byte* payload, unsigned int length)
{
	bool val = false;
	if (payload[0] == '1')
		val = true;
	else
		val = false;

	//обновляем статус других устройств, фактическим состоянием выключателя
	bool curState = getState();
	if (val != curState)
		_sender->publish(topicSwitchState, curState, true);
}

void MqttButton::onTopicSwitchSetup(byte* payload, unsigned int length)
{
	//deserialize json
	StaticJsonDocument<200> doc;
	DeserializationError error = deserializeJson(doc, payload);

	if (error) {
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.c_str());
		return;
	}

	long holdDuration = doc["holdDuration"];
	if (holdDuration > 0)
		holdTimeout = holdDuration;

	//defaultAction
	//"none" - отключить прерывание

	//disable - true

}

// Функция, вызываемая прерыванием, для кнопки без фиксации (button without fixing)
ICACHE_RAM_ATTR void MqttButton::interruptButtton() {
	//Защита от дребезга 
	if (_lock || !_lockTimer2.isExpired())
		return;
	_lock = true;
	_lockTimer.restart();

	while (!_lockTimer.isExpired())
	{
	}

	if (digitalRead(buttonPin) != levelButton)
	{
		//повторное нажатие кнопки выключает hold таймер
		if (isHoldButton && !_flagHold)
			btnHold(holdTimeout);
		else
			btnPress();

	}

	_lockTimer2.restart(); // защищаемся от э/м скачков в реле
	_lock = false;
}

//добавить publish topic
void MqttButton::addTopic(string topic)
{
	_publishTopics.push_back(topic);
}

void MqttButton::setSender(Sender& sender)
{
	_sender = &sender;
}

void MqttButton::btnPress() 
{
	int state = !getState();

	Serial.println((buttonName + ' ' + "BtnPress(" + String(state).c_str() + ")").c_str());

	holdStop();

	relaySwitch(state);

	_flagChange = true;
}

void MqttButton::btnHold(int duration) {

	//реализация удержания кнопки
	Serial.println((buttonName + ' ' + "BtnHold(" + String(duration).c_str() + ")").c_str());

	relaySwitch(true);

	_holdTimer.setTimeout(duration);

	holdStart();

}

//читаем текущее состояние реле
bool MqttButton::getState()
{
	int curState = digitalRead(relayPin);
	if (levelTrigger)
		return curState;
	else
		return !curState;
}

void MqttButton::relaySwitch(bool state)
{
	if (levelTrigger)
		digitalWrite(relayPin, state);
	else
		digitalWrite(relayPin, !state);
}

void MqttButton::holdStart()
{
	_holdTimer.restart();
	_flagHold = true;
	_flagChange = true;

}

void MqttButton::holdStop()
{
	_holdTimer.stop();
	_flagHold = false;
}




