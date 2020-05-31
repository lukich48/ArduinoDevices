/*	WeMos D1 R2 & mini
	Теплый пол в ванной
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include "home_MqttButton.h"
#include "home_DhtSensor.h"
#include <Secret.h>

#include <string>
using namespace std;

//***Блок переменных
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
	"hall/switch"
);

ConnectionHelper helper(&settings);

MqttButton button1(-1, 14, "warmfloor");
MqttButton button2(2, 12, "lamp");

DhtSensor dhtSensor(5,DHT22,settings.topicBase + "/hall/dht");

void setup() {
	Serial.begin(115200);
	helper.setup();

	button1.levelTrigger = LOW;
	helper.addButton(&button1);

	button2.levelTrigger = LOW;
	button2.levelButton = HIGH;
	helper.addButton(&button2);

	dhtSensor.setup(helper.sender);
}

void loop() {
	helper.handle();

	dhtSensor.handle();
}
