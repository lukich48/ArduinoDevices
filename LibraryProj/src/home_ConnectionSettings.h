#pragma once
#include "arduino.h"

#include <string>
using std::string;

class ConnectionSettings
{
public:
	ConnectionSettings(const char* ssid, const char* wifiPass, const char* mqttServer, int mqttPort, const char* mqttUser, const char* mqttPass, string deviceName);

	const char* ssid;
	const char* wifiPass;
	const char* mqttServer;
	int mqttPort;
	const char* mqttUser;
	const char* mqttPass;
	string deviceName;
	string topicBase = "home";
};


