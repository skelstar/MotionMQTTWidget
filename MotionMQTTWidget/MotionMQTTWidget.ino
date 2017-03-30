#include <myWifiHelper.h>
#include <myPushButton.h>
#include <TaskScheduler.h>
#include <TimeLib.h>
#include <myUbidotVariable.h>


char versionText[] = "MotionMQTTWidget v2.0";

#define TOPIC_TIMESTAMP     "/dev/timestamp"

enum SensorNameType {
	PIR_LID,
	PIR_BOXEND,
	PIR_BOXEND2,
	PIR_ROOM
};

// struct PirUnitType {
// 	wifiHostName;
// 	topicPublish;
// 	topicOnline;
// 	sensorName; 
// 	pirPin;      
// };

// ====================================================
#define     WIFI_HOSTNAME   "PIR_LIAMROOM_LID"
#define     TOPIC_MOTION   "/dev/liamroom-lid-motion"
#define     TOPIC_ONLINE    "/dev/liamroom-lid-online"
#define     SENSOR_NAME    "LID"
#define     PIR_PIN         D0    // ESP8266-01: 2, WEMOS D0
SensorNameType sensor = PIR_LID;

// #define     WIFI_HOSTNAME   "PIR_LIAMROOM_BOXEND"
// #define     TOPIC_MOTION   "/dev/liamroom-boxend-motion"
// #define     TOPIC_ONLINE    "/dev/liamroom-boxend-online"
// #define     SENSOR_NAME    	"BOXEND"
// #define     PIR_PIN         D0    // ESP8266-01: 2, WEMOS D0
// SensorNameType sensor = PIR_BOXEND;

// #define     WIFI_HOSTNAME   "PIR_LIAMROOM_BOXEND_2"
// #define     TOPIC_MOTION   "/dev/liamroom-boxend2-motion"
// #define     TOPIC_ONLINE    "/dev/liamroom-boxend2-online"
// #define     SENSOR_NAME    	"BOXEND2"
// #define     PIR_PIN         0    // ESP8266-01: 2, WEMOS D0
// SensorNameType sensor = PIR_BOXEND2;

// ====================================================

#define     MQTT_FEED_TIMESTAMP "/dev/timestamp"

#define     SECONDS             1000
#define     PIR_OFFLINE_PERIOD  10 * SECONDS  // ten seconds

#define     BLUE_LED         1 // pin 1 on -01

#define     PIR_TRIGGER_LED 14

bool        pirsEnabled = true;

/*---------------------------------------------------------------------*/

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

int status = WL_IDLE_STATUS;

/*---------------------------------------------------------------------*/

#define     PULL_UP                 true
#define     NO_PULL_UP              false
#define     NORMAL_LOW              LOW
#define     NORMAL_HIGH             HIGH

void pir_callback(int eventCode, int eventParams);

myPushButton pir(PIR_PIN, NO_PULL_UP, 100000, NORMAL_LOW, pir_callback);

/*---------------------------------------------------------------------*/

Scheduler runner;

#define RUN_ONCE    2
#define RUN_TWICE   4

void tReadPIRCallback();
void pirOfflinePeriodCallback();
// void tCallback_FlashTriggerLEDON();
// void tCallback_FlashTriggerLEDOFF();
void tSendMotionToMQTTCallback();

// Task tFlashTriggerLED(70, RUN_TWICE, &tCallback_FlashTriggerLEDON, &runner, false);
// void tCallback_FlashTriggerLEDON() {
// 	pinMode(PIR_TRIGGER_LED, OUTPUT);
// 	digitalWrite(PIR_TRIGGER_LED, LOW);
// 	tFlashTriggerLED.setCallback(tCallback_FlashTriggerLEDOFF);
// }
// void tCallback_FlashTriggerLEDOFF() {
// 	pinMode(PIR_TRIGGER_LED, OUTPUT);
// 	digitalWrite(PIR_TRIGGER_LED, HIGH);
// 	tFlashTriggerLED.setCallback(tCallback_FlashTriggerLEDON);
// }


Task tPirOfflinePeriod(5000, RUN_TWICE, &pirOfflinePeriodCallback, &runner, false);
void pirOfflinePeriodCallback() {

	if (tPirOfflinePeriod.isFirstIteration()) {
		pirsEnabled = false;
		//Serial.println("PIR Disabled");
		//tFlashTriggerLED.restart();
	}
	else if (tPirOfflinePeriod.isLastIteration()) {
		pirsEnabled = true;
		//Serial.println("PIR Enabled");
	}
}

void mqttcallback_Timestamp(byte* payload, unsigned int length) {
	wifiHelper.mqttPublish(TOPIC_ONLINE, "1");
}

Task tSendMotionToMQTT(10, 1, &tSendMotionToMQTTCallback, &runner, false);
void tSendMotionToMQTTCallback() {
	char* topic = TOPIC_MOTION;
	char* payload;
	if (sensor == PIR_LID) {
		payload = "MOTION: LID";
	} else if (sensor == PIR_BOXEND) {
		payload = "MOTION: BOXEND";
	} else if (sensor == PIR_BOXEND2) {
		payload = "MOTION: BOXEND2";
	}
	sendTopicToMQTT(topic, payload);
}

void sendTopicToMQTT(char* topic, char* payload) {
	
	wifiHelper.mqttPublish(topic, payload);
}

int oldPinVal = 0;

Task tReadPIR(200, TASK_FOREVER, &tReadPIRCallback, &runner, false);
void tReadPIRCallback() {
	pir.serviceEvents();
}


void pir_callback(int eventCode, int eventParams) {

	switch (eventParams) {

		case pir.EV_BUTTON_PRESSED: {

			tSendMotionToMQTT.restart();
			Serial.println("pir_callback");

			runner.execute();
			}
			break;

		case pir.EV_RELEASED:{}
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
	wifiHelper.mqttAddSubscription(TOPIC_TIMESTAMP, mqttcallback_Timestamp);

	runner.addTask(tPirOfflinePeriod);
	runner.addTask(tReadPIR);
	//runner.addTask(tFlashTriggerLED);
	runner.addTask(tSendMotionToMQTT);

	pinMode(PIR_PIN, INPUT);

	tReadPIR.restart();
}

void loop() {

	runner.execute();

	wifiHelper.loopMqtt();

	ArduinoOTA.handle();

	pir.serviceEvents();
	
	// int pinValue = digitalRead(PIR_PIN);
	// if (pinValue != oldPinVal) {

	// 	if (pinValue == 1) {
	// 		tSendMotionToMQTT.restart();
	// 		Serial.println("pir_callback");
	// 	}
	// 	runner.execute();

	// 	oldPinVal = pinValue;
	// }

	delay(10);
}

/*---------------------------------------------------------------------*/
