
#include "home_Sender.h"
#include <PubSubClient.h>

#include <string>
using std::string;

Sender::Sender(PubSubClient& mqttClient)
{
	this->mqttClient = &mqttClient;
}

void Sender::publish(string topic, string payload, boolean retained)
{
	print("publish: [" + topic + "] " + payload + "");
	mqttClient->publish(topic.c_str(), payload.c_str(), retained);
}

void Sender::publish(string topic, const char* payload, boolean retained)
{
	print("publish: [" + topic + "] " + payload + "");
	mqttClient->publish(topic.c_str(), payload, retained);
}

void Sender::publish(string topic, bool payload, boolean retained)
{
	print("publish: [" + topic + "] " + String(payload).c_str() + "");
	mqttClient->publish(topic.c_str(), String(payload).c_str(), retained);
}


void Sender::print(string message)
{
	Serial.println(message.c_str());
}


