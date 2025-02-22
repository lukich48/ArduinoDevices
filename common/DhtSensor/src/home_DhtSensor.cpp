#include "home_DhtSensor.h"
#include "home_Sender.h"

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include <RBD_Timer.h>
#include <string>
#include <ArduinoJson.h>
using std::string;

DhtSensor::DhtSensor(uint8_t dhtPin, uint8_t dhtType, string commandTopic)
	:dht(dhtPin, dhtType)
{
	this->commandTopic = commandTopic;
}


void DhtSensor::setup(Sender& sender)
{
	this->sender = &sender;

	dht.begin();

	sensor_t sensor;
	dht.humidity().getSensor(&sensor);
	uint32_t delayMS = sensor.min_delay / 1000;
  	delayTimer.setTimeout(delayMS);
	reconnectTimer.setTimeout(reconnectTimeout);
}

void DhtSensor::handle()
{
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

			std::fill_n(tbuf, bufCount, 0);
			std::fill_n(hbuf, bufCount, 0);

			i=0;
			reconnectTimer.restart();
			}
		}
	}
}

void DhtSensor::sort(float a[])
{
	float temp = 0;
	for (int i = 0; i < bufCount; i++)
	{
		for (int j = i; j < bufCount; j++)
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

void DhtSensor::publishData(float p_temperature, float p_humidity) {
	// create a JSON object
	// doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
	StaticJsonDocument<200> doc;
	doc["temperature"] = (String)p_temperature;
	doc["humidity"] = (String)p_humidity;

	char data[200];
	serializeJson(doc, data);
	sender->publish(commandTopic, data, true);
}

void DhtSensor::getSensorData(){

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
      tbuf[i] = event.temperature + temperatureOffset;
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
      hbuf[i] = event.relative_humidity + humidityOffset;
    }	
}


