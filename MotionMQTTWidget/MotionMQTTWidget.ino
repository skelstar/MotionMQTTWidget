#include <myWifiHelper.h>
#include <myPushButton.h>

char versionText[] = "MotionMQTTWidget v0.9";

#define 	WIFI_HOSTNAME   "LiamRoomDoorPIR"

#define 	TOPIC_LIAMROOMDOOR_PIR	"/dev/liam-room-door-pir"

#define 	PIR_PIN    		D0
#define		PULL_UP     	true

/*---------------------------------------------------------------------*/

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

/*---------------------------------------------------------------------*/

void listener_PIR(int eventCode, int eventParams);

myPushButton pir(PIR_PIN, PULL_UP, 100000, LOW, listener_PIR);

void listener_PIR(int eventCode, int eventParams) {

	switch (eventParams) {

		case pir.EV_BUTTON_PRESSED:

            wifiHelper.mqttPublish(TOPIC_LIAMROOMDOOR_PIR, "1");

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

/*---------------------------------------------------------------------*/
