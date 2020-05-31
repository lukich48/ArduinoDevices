/*	WeMos D1 R2 & mini
	Датчик температуры и влажности в гостинной
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include "home_DhtSensor.h"
#include <Secret.h>

#include <ArduinoJson.h>
#include <DHT_U.h>
#include <Wire.h>
#include <SPI.h>

#include <string>
using namespace std;

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
	"livingroom/dht"
);

ConnectionHelper helper(&settings);
DhtSensor dhtSensor(5,DHT22,settings.topicBase + "/" + settings.deviceName);

void setup() {
	Serial.begin(115200);
	helper.setup();

	//dhtSensor.debug = true;
	dhtSensor.setup(helper.sender);
}

void loop() {
	helper.handle();

	dhtSensor.handle();
}
