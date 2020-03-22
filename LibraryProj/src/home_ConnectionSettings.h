// ConnectionSettings.h

#ifndef _CONNECTIONSETTINGS_h
#define _CONNECTIONSETTINGS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <string>
using namespace std;
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

#endif

