/*  Generic ESP8266 Module
	Подсветка кухонной рабочей поверхности
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include "home_MqttButton.h"
#include <Secret.h>

#include <string>

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
	"kitchen/switch"
);

ConnectionHelper helper(&settings);

MqttButton button1(13, 14, "worktop");


// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	helper.setup();
	button1.levelButton = HIGH;
	button1.addTopic("home/kitchen/switch2/table");
	helper.addButton(&button1);

}

// the loop function runs over and over again until power down or reset
void loop() {
	helper.handle();
}
