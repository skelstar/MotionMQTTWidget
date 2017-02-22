//#include <WiFi.h>
#include <myWifiHelper.h>
//#include "wificonfig.h"
#include <myPushButton.h>
//#include <UbidotsMicroESP8266.h>
#include <ESP8266HTTPClient.h>

char versionText[] = "MotionMQTTWidget v0.9";

#define 	WIFI_HOSTNAME   "LiamRoomDoorPIR"

#define 	TOPIC_LIAMROOMDOOR_PIR	"/dev/liam-room-door-pir"

#define     UBIDOT_DEVICE           "liam-room-door-sensor"
#define     UBIDOT_VARIABLE         "motion"
#define     UBIDOT_LIAMROOMDOOR_ID  "58abde8c76254260e42f1632"
#define     UBIDOT_TOKEN            "GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF"

#define 	PIR_PIN    		D0
#define		PULL_UP     	true

/*---------------------------------------------------------------------*/

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

int status = WL_IDLE_STATUS;

/*---------------------------------------------------------------------*/

//Ubidots ubiclient("GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF");

/*---------------------------------------------------------------------*/

void listener_PIR(int eventCode, int eventParams);

myPushButton pir(PIR_PIN, PULL_UP, 100000, LOW, listener_PIR);

void listener_PIR(int eventCode, int eventParams) {

	switch (eventParams) {

		case pir.EV_BUTTON_PRESSED:

            wifiHelper.mqttPublish(TOPIC_LIAMROOMDOOR_PIR, "1");

            sendVariableToUbidots();

			break;

        case pir.EV_RELEASED:
            break;
	}
}

/*---------------------------------------------------------------------*/

void setup() {

    Serial.begin(9600);
    Serial.println("Booting");

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_HOSTNAME);

    wifiHelper.setupMqtt();
}

void loop() {

    wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    pir.serviceEvents();

    delay(10);
}

void sendVariableToUbidots() {
    String body = "{ \"value\": 1.2, \"context\":{ \"message\":\"test2\" } }";
    wifiHelper.sendValueToUbidots(UBIDOT_DEVICE, UBIDOT_LIAMROOMDOOR_ID, UBIDOT_TOKEN, body);
}
/*---------------------------------------------------------------------*/
