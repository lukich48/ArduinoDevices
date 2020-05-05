/*	WeMos D1 R2 & mini
	Датчик температуры и влажности в гостинной
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include <Secret.h>

#include <DHT_U.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>

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
	"livingroom/dht"
);

ConnectionHelper helper(&settings);

DHT_Unified dht(5, DHT22);
const int sleepingTimeSecond = 120; //сколько спать
RBD::Timer reconnectTimer(sleepingTimeSecond * 1000);
const uint8_t bufCount = 7;
float tbuf[bufCount];
float hbuf[bufCount];
const bool debug = false;
uint32_t delayMS;
uint8_t i;
RBD::Timer delayTimer;

void sort(float a[])
{
	float temp = 0;
	for (int i = 0; i < bufCount; i++)
	{
		for (int j = 0; j < bufCount; j++)
		{
			if (a[i] > a[j])
			{
				temp = a[i];
				a[i] = a[j];
				a[j] = temp;
			}
		}
	}
}

// function called to publish the temperature and the humidity
void publishData(float p_temperature, float p_humidity) {
	// create a JSON object
	// doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
	//deserialize json
	StaticJsonDocument<200> doc;
	doc["temperature"] = (String)p_temperature;
	doc["humidity"] = (String)p_humidity;

	char data[200];
	serializeJson(doc, data);
	helper.sender.publish(settings.topicBase + "/" + settings.deviceName, data, true);
}

void getSensorData(){

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
      return;
    }
    else {
      if(debug)
      {
        Serial.print(F("Temperature: "));
        Serial.print(event.temperature);
        Serial.println(F("°C"));
      }
      tbuf[i] = event.temperature;
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
      return;
    }
    else {
      if (debug)
      {
        Serial.print(F("Humidity: "));
        Serial.print(event.relative_humidity);
        Serial.println(F("%"));
      }
      hbuf[i] = event.relative_humidity;
    }	
}

void setup() {
	Serial.begin(115200);
	helper.setup();
	dht.begin();

	sensor_t sensor;
	dht.humidity().getSensor(&sensor);
	delayMS = sensor.min_delay / 1000;
  delayTimer.setTimeout(delayMS);
}

void loop() {
	helper.handle();

	if (reconnectTimer.isExpired())
	{
		if(delayTimer.isExpired())
		{
			getSensorData();

			i++;
			delayTimer.restart();

			if(i == bufCount){

			sort(tbuf);
			sort(hbuf);

			publishData(tbuf[bufCount / 2], hbuf[bufCount / 2]);

			fill_n(tbuf, bufCount, 0);
			fill_n(hbuf, bufCount, 0);

			i=0;
			reconnectTimer.restart();
			}
		}
	}
}
