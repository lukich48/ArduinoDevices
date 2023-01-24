#pragma once
#include "arduino.h"

#include <PubSubClient.h>

#include <string>
using std::string;

class Sender
{
public:
	Sender(PubSubClient& mqttClient);
	void publish(string topic, string payload, boolean retained);
	void publish(string topic, const char* payload, boolean retained);
	void publish(string topic, bool payload, boolean retained);
	void print(string message);
	PubSubClient* mqttClient;

private:
};
