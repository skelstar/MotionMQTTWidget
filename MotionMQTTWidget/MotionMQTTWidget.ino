#include <myWifiHelper.h>
#include <myPushButton.h>
#include <ESP8266HTTPClient.h>
#include <TaskScheduler.h>
#include <TimeLib.h>


char versionText[] = "MotionMQTTWidget v1.1";

#define 	WIFI_HOSTNAME   "LiamRoomDoorPIR"

#define 	TOPIC_LIAMROOMDOOR_PIR	"/dev/liam-room-door-pir"

#define     UBIDOT_DEVICE           "liam-room-door-sensor"
#define     UBIDOT_VARIABLE         "motion"
#define     UBIDOT_LIAMROOMDOOR_ID  "58abde8c76254260e42f1632"
#define     UBIDOT_TOKEN            "GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF"

#define     MQTT_FEED_HHmm  "/dev/HHmm"

#define     PIR_OFFLINE_PERIOD  5000 //10 * 60 * 1000  // ten minutes

#define 	PIR_PIN    		D0
#define		PULL_UP     	true

bool pirEnabled = true;
bool debug = true;

/*---------------------------------------------------------------------*/

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

int status = WL_IDLE_STATUS;

/*---------------------------------------------------------------------*/

Scheduler runner;

#define RUN_ONCE 2

void pirOfflinePeriodCallback();

Task tPirOfflinePeriod(PIR_OFFLINE_PERIOD, RUN_ONCE, &pirOfflinePeriodCallback, &runner, false);

void pirOfflinePeriodCallback() {

    if (tPirOfflinePeriod.isFirstIteration()) {
        pirEnabled = false;
        Serial.println("PIR Disabled");
    }
    else if (tPirOfflinePeriod.isLastIteration()) {
        pirEnabled = true;
        Serial.println("PIR Enabled");
    }
}

/*---------------------------------------------------------------------*/

void listener_PIR(int eventCode, int eventParams);

myPushButton pir(PIR_PIN, PULL_UP, 100000, LOW, listener_PIR);

void listener_PIR(int eventCode, int eventParams) {

    if (pirEnabled == false) {
        return;
    }

	switch (eventParams) {

		case pir.EV_BUTTON_PRESSED:

            tPirOfflinePeriod.restart();
            runner.execute();

            wifiHelper.mqttPublish(TOPIC_LIAMROOMDOOR_PIR, "1");

            ubidotSendVariable();

			break;

        case pir.EV_RELEASED:
            break;
	}
}
/* ----------------------------------------------------------- */

char timeStr[5];
int time_hour = 0;
int time_minute = 0;

time_t tm;

void devtime_mqttcallback(byte* payload, unsigned int length) {

    if (payload[0] == '-') {
        time_hour = -1;
    } else {

        int hour = (payload[0] - '0') * 10;
        hour += (payload[1] - '0');
        int minute = (payload[3] - '0') * 10;
        minute += (payload[4] - '0');

        setTime(hour, minute, 0, 0, 0, 0);    // h, m, s, days, mnths, years

        timeStr[0] = payload[0];
        timeStr[1] = payload[1];
        timeStr[2] = payload[2];
        timeStr[3] = payload[3];
        timeStr[4] = payload[4];
    }
}

/*---------------------------------------------------------------------*/

void setup() {

    Serial.begin(9600);
    Serial.println("Booting");

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_HOSTNAME);

    wifiHelper.setupMqtt();
    wifiHelper.mqttAddSubscription(MQTT_FEED_HHmm, devtime_mqttcallback);

    runner.addTask(tPirOfflinePeriod);
}

void loop() {

    wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    pir.serviceEvents();

    runner.execute();

    delay(10);
}

void ubidotOldValues() {

}

void ubidotSendVariable() {

        char digits[4];
        String body = "{ \"value\": 1.2, \"context\":{ \"message\":\"";

        body += itoa(hour(), digits, 10);
        body += ':';
        body += itoa(minute(), digits, 10);   
        body += "\" } }";

            Serial.println(body);

    if ((hour() <= 6 && hour() >= 21) || debug) {
        wifiHelper.sendValueToUbidots(UBIDOT_DEVICE, UBIDOT_LIAMROOMDOOR_ID, UBIDOT_TOKEN, body);
    }
}
/*---------------------------------------------------------------------*/
