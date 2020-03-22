// home_Sender.h

#ifndef _HOME_SENDER_h
#define _HOME_SENDER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
#include <PubSubClient.h>

#include <string>
using namespace std;

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

#endif

