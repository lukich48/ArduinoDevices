#pragma once
#include "arduino.h"

#include "home_ConnectionSettings.h"
#include "home_Button.h"
#include "home_Sender.h"

#include <RBD_Timer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include <string>
using std::string;

class ConnectionHelper {
public:
	ConnectionHelper(ConnectionSettings* settings);
	void setup();
	void handle();
	void addButton(Button* button);

	Sender sender;
	ConnectionSettings* settings;
	WiFiClient wclient;
	PubSubClient mqttClient;
	string topicSubscribe;
	int reconnectTimeout = 60000; //пауза между реконнектами Wi-Fi mc

private:
	bool wifiConnect();
	bool mqttConnect();

	RBD::Timer _reconnectTimer;

	static Button* _buttons[];
	static uint8_t _buttonsCount;
	static void attachInterrupt1();
	static void attachInterrupt2();
	static void attachInterrupt3();
	static void attachInterrupt4();
	static void attachInterrupt5();


};


