/*	WeMos D1 R2 & mini
	свет в ванной над зеркалом
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include "home_MqttButton.h"
#include "home_DhtSensor.h"
#include <Secret.h>

//***DHT
#include <ArduinoJson.h>
#include <DHT_U.h>
#include <Wire.h>
#include <SPI.h>

#include <string>

//connection variables
const char* ssid = WI_FI_SSID;
const char* wifiPass = WI_FI_PASSWORD;
const char* mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT; 
const char* mqttUser = MQTT_USER;
const char* mqttPass = MQTT_PASSWORD;

ConnectionSettings settings(
	ssid,
	wifiPass,
	mqttServer,
	mqttPort,
	mqttUser,
	mqttPass,
	"bathroom/switch"
);

ConnectionHelper helper(&settings);

MqttButton button1(14, 12, "mirror");
MqttButton button2(-1, 13, "laundry-fan");
MqttButton button3(4, 5, "fan");

DhtSensor dhtSensor(2,DHT22,settings.topicBase + "/bathroom/dht");

void setup() {
	Serial.begin(115200);
	helper.setup();

	button1.levelButton = HIGH;
	button1.levelTrigger = LOW;
	button1.isHoldButton = true;
	button1.holdTimeout = 10*60*1000;
	helper.addButton(&button1);

	button2.levelTrigger = LOW;
	helper.addButton(&button2);

	button3.levelButton = HIGH;
	button3.levelTrigger = LOW;
	button3.isHoldButton = true;
	button3.holdTimeout = 10*60*1000;
	helper.addButton(&button3);

	// dhtSensor.humidityOffset = 2.0;
	dhtSensor.setup(helper.sender);

}

void loop() {
	helper.handle();

  	dhtSensor.handle();
}
