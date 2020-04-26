/*	WeMos D1 R2 & mini
	Датчик температуры и влажности в гостинной
	!!!Работает с устаревшими библиотеками DHT!!! 
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include <Secret.h>

#include <DHT.h> 
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

DHT dht(5, DHT22);
const int sleepingTimeSecond = 120; //сколько спать
RBD::Timer reconnectTimer(sleepingTimeSecond * 1000);
const int bufCount = 5;
const bool debug = false;

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

	/*
	{
	"temperature": "23.20" ,
	"humidity": "43.70"
	}
	*/
	char data[200];
	serializeJson(doc, data);
	helper.sender.publish(settings.topicBase + "/" + settings.deviceName, data, true);
}

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	helper.setup();
	dht.begin();
}

// the loop function runs over and over again until power down or reset
void loop() {
	helper.handle();

	if (reconnectTimer.isExpired())
	{
		int i = bufCount;
		float* tbuf = new float[bufCount];
		float* hbuf = new float[bufCount];

		while (i > 0)
		{
			delay(1000);
			// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
			float h = dht.readHumidity();
			// Read temperature as Celsius (the default)
			float t = dht.readTemperature();

			if (isnan(h) || isnan(t))
			{
				Serial.println("ERROR: Failed to read from DHT sensor!");
				continue;
			}
			else
			{
				i--;
				tbuf[i] = t;
				hbuf[i] = h;
				if (debug)
				{
					Serial.println(h);
					Serial.println(t);
				}

			}
		}

		sort(tbuf);
		sort(hbuf);

		publishData(tbuf[bufCount / 2], hbuf[bufCount / 2]);


		delete[] tbuf;
		delete[] hbuf;

		reconnectTimer.restart();
	}
}
