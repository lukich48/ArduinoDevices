#pragma once
#include "arduino.h"

#include "home_Sender.h"
#include <RBD_Timer.h>
#include <string>
#include <DHT_U.h>
#include <Wire.h>
#include <SPI.h>
using std::string;

class DhtSensor{
public:
    DhtSensor(uint8_t dhtPin, uint8_t dhtType, string commandTopic);
    void setup(Sender& sender);
    void handle();

    DHT_Unified dht;
    string commandTopic;
    int reconnectTimeout = 120 * 1000;
    bool debug;
    float humidityOffset = 0; //offset of reference value  
    float temperatureOffset = 0; //offset of reference value  

private:
    void sort(float a[]);
    void publishData(float p_temperature, float p_humidity);
    void getSensorData();

    Sender* sender;
    RBD::Timer reconnectTimer;
    RBD::Timer delayTimer;
    const static uint8_t bufCount = 7;
    uint8_t i = 0;
    float tbuf[bufCount];
    float hbuf[bufCount];
};
