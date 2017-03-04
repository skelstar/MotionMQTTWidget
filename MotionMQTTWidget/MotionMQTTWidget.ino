#include <myWifiHelper.h>
#include <myPushButton.h>
#include <ESP8266HTTPClient.h>
#include <TaskScheduler.h>
#include <TimeLib.h>
#include <myUbidotVariable.h>


char versionText[] = "MotionMQTTWidget v1.1";

#define 	WIFI_HOSTNAME   "LiamRoomDoorPIR"

#define 	TOPIC_LIAMROOMDOOR_PIR	"/dev/liam-room-door-pir"

#define     UBIDOT_DEVICE           "liam-room-door-sensor"
#define     UBIDOT_VARIABLE         "motion"
#define     UBIDOT_LIAMROOMDOOR_ID  "58abde8c76254260e42f1632"
#define     UBIDOT_TOKEN            "GioE85MkDexLpsbE1Tt37F3TY2p3Fa6XM4bKvftz9OfC87MrH5bQGceJrVgF"

#define     SEND_WINDOW_START_HOUR  20
#define     SEND_WINDOW_END_HOUR    6

#define     MQTT_FEED_TIMESTAMP "/dev/timestamp"

#define     PIR_OFFLINE_PERIOD  10 * 60 * 1000  // ten minutes

#define 	PIR_PIN    		D0
#define     PIR_TRIGGER_LED D4
#define		PULL_UP     	true

bool        pirEnabled = true;

bool        DEBUG = false;

/*---------------------------------------------------------------------*/

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

MyUbidotVariable motion(wifiHelper.client, UBIDOT_LIAMROOMDOOR_ID, UBIDOT_TOKEN);

int status = WL_IDLE_STATUS;

/*---------------------------------------------------------------------*/

Scheduler runner;

#define RUN_ONCE    2
#define RUN_TWICE   4

void pirOfflinePeriodCallback();
void tCallback_FlashTriggerLEDON();
void tCallback_FlashTriggerLEDOFF();

Task tPirOfflinePeriod(DEBUG == false 
                        ? PIR_OFFLINE_PERIOD 
                        : 5000, 
                       RUN_ONCE,
                       &pirOfflinePeriodCallback, 
                       &runner, 
                       false);
Task tFlashTriggerLED(70, 
                      DEBUG == false
                        ? RUN_ONCE
                        : RUN_TWICE, 
                      &tCallback_FlashTriggerLEDON, 
                      &runner, 
                      false);

void pirOfflinePeriodCallback() {

    if (tPirOfflinePeriod.isFirstIteration()) {
        pirEnabled = false;
        Serial.println("PIR Disabled");
        tFlashTriggerLED.restart();
    }
    else if (tPirOfflinePeriod.isLastIteration()) {
        pirEnabled = true;
        Serial.println("PIR Enabled");
    }
}

void tCallback_FlashTriggerLEDON() {
    pinMode(PIR_TRIGGER_LED, OUTPUT);
    digitalWrite(PIR_TRIGGER_LED, LOW);
    tFlashTriggerLED.setCallback(tCallback_FlashTriggerLEDOFF);
}

void tCallback_FlashTriggerLEDOFF() {
    pinMode(PIR_TRIGGER_LED, OUTPUT);
    digitalWrite(PIR_TRIGGER_LED, HIGH);
    tFlashTriggerLED.setCallback(tCallback_FlashTriggerLEDON);
}

// void tCallback_FlashTriggerLED() {
//     if (tFlashTriggerLED.isFirstIteration()) {
//         pinMode(PIR_TRIGGER_LED, OUTPUT);
//         digitalWrite(PIR_TRIGGER_LED, LOW);
//     } else if (tFlashTriggerLED.isLastIteration()) {
//         pinMode(PIR_TRIGGER_LED, OUTPUT);
//         digitalWrite(PIR_TRIGGER_LED, HIGH);
//     }
// }

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
            //ubidotDeleteValuesForLast24Hours();

			break;

        case pir.EV_RELEASED:
            break;
	}
}
/* ----------------------------------------------------------- */

void mqttTimestampCallback(byte* payload, unsigned int length) {
    unsigned long pctime = strtoul((char*)payload, NULL, 10);
    setTime(pctime);
}

/*---------------------------------------------------------------------*/

void setup() {

    Serial.begin(9600);
    Serial.println("Booting");

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_HOSTNAME);

    wifiHelper.setupMqtt();
    wifiHelper.mqttAddSubscription(MQTT_FEED_TIMESTAMP, mqttTimestampCallback);

    runner.addTask(tPirOfflinePeriod);
    runner.addTask(tFlashTriggerLED);
}

void loop() {

    serviceCallback();

    wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    pir.serviceEvents();

    delay(10);
}

/*---------------------------------------------------------------------*/

void serviceCallback() {

    runner.execute();
}

void ubidotDeleteValuesForLast24Hours() {

    //#define DELETE_WINDOW     24 * 60 * 60
    #define ONE_MINUTE            60 * 1000
    #define ONE_HOUR              60 * ONE_MINUTE
    #define DELETE_WINDOW     1 * ONE_HOUR
    unsigned long start = now() - DELETE_WINDOW;
    unsigned long end = now()-(10 * 1000);
    motion.ubidotsDeleteValues(start, end);
}

void ubidotSendVariable() {

    while (tFlashTriggerLED.isEnabled()) {
        runner.execute();
    }

    // exit if our of hours
    if (hour() < SEND_WINDOW_START_HOUR && 
        hour() >= SEND_WINDOW_END_HOUR && 
        DEBUG == false) {
        return;
    }

    char digits[4];
    String body = "{ \"value\": 1.2, \"context\":{ \"message\":\"";

    body += itoa(hour(), digits, 10);
    body += ':';
    body += itoa(minute(), digits, 10);   
    body += "\" } }";

    motion.sendValue(body, serviceCallback);
}
/*---------------------------------------------------------------------*/
