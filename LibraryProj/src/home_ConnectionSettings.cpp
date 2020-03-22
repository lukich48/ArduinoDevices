/*
Dto с настройками
*/

#include "home_ConnectionSettings.h"
#include "Arduino.h"
#include <string>
using namespace std;

ConnectionSettings::ConnectionSettings(	const char* ssid, 
	const char* wifiPass, 
	const char* mqttServer, 
	int mqttPort, 
	const char* mqttUser, 
	const char* mqttPass, 
	string deviceName)
{
	this->ssid = ssid;
	this->wifiPass = wifiPass;
	this->deviceName = deviceName;

	this->mqttServer = mqttServer;
	this->mqttPort = mqttPort;
	this->mqttUser = mqttUser;
	this->mqttPass = mqttPass;

}
