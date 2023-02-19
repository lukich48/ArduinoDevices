
#include "home_Sender.h"
#include <PubSubClient.h>

#include <string>
using std::string;

#define _HOME_DEBUG 0

#ifndef HOME_DEBUG
#define HOME_DEBUG _HOME_DEBUG
#endif

Sender::Sender(PubSubClient& mqttClient)
{
	this->mqttClient = &mqttClient;
}

void Sender::publish(string topic, string payload, boolean retained)
{
	#if HOME_DEBUG
	print("publish: [" + topic + "] " + payload + "");
	#endif
	mqttClient->publish(topic.c_str(), payload.c_str(), retained);
}

void Sender::publish(const char* topic, string payload, boolean retained)
{
	#if HOME_DEBUG
	print(string("publish: [") + topic + "] " + payload + "");
	#endif
	mqttClient->publish(topic, payload.c_str(), retained);
}

void Sender::publish(string topic, const char* payload, boolean retained)
{
	#if HOME_DEBUG
	print("publish: [" + topic + "] " + payload + "");
	#endif
	mqttClient->publish(topic.c_str(), payload, retained);
}

void Sender::publish(string topic, int payload, boolean retained)
{
	#if HOME_DEBUG
	print("publish: [" + topic + "] " + String(payload).c_str() + "");
	#endif
	mqttClient->publish(topic.c_str(), String(payload).c_str(), retained);
}


void Sender::print(string message)
{
	Serial.println(message.c_str());
}


