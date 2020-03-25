/*	WeMos D1 R2 & mini
	свет в ванной над зеркалом
*/
#include "home_ConnectionSettings.h"
#include "home_ConnectionHelper.h"
#include "home_MqttButton.h"
#include <Secret.h>

#include <RCSwitch.h>

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
	"livingroom/lamp"
);

ConnectionHelper helper(&settings);

MqttButton button1(-1, 12, "warm");
MqttButton button2(-1, 13, "cold");

unsigned long  rfButton1 = 3931160; //turn on all buttons
unsigned long  rfButton2 = 3931154; //turn on button1
RBD::Timer _lockTimer(250);

RCSwitch mySwitch = RCSwitch();

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	helper.setup();

	button1.levelTrigger = HIGH;
	helper.addButton(&button1);

	button2.levelTrigger = HIGH;
	helper.addButton(&button2);

 	mySwitch.enableReceive(5);  // Receiver on interrupt 0 => that is pin #2

}

// the loop function runs over and over again until power down or reset
void loop() {
	helper.handle();

	if (mySwitch.available()) {  
		if (mySwitch.getReceivedValue() == rfButton1){
			if(_lockTimer.isExpired()){
				//если хотя бы один выключен
				if(button1.getState() == LOW || button2.getState() == LOW ){

					if(button1.getState() == LOW)
						button1.btnPress();
					if(button2.getState() == LOW)
						button2.btnPress();
				}
				//выключаем все
				else{
					button1.btnPress();
					button2.btnPress();
				} 
				
				_lockTimer.restart();
			}
		}
		else if(mySwitch.getReceivedValue() == rfButton2){
			if(_lockTimer.isExpired()){
				if(button2.getState() == HIGH ){
					button2.btnPress();
					if(button1.getState() == LOW)
						button1.btnPress();

				} 
				else{
					button1.btnPress();
				} 
				
				_lockTimer.restart();
			}
		}
		
		mySwitch.resetAvailable();
	}
}
