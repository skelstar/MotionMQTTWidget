#include <myWifiHelper.h>
#include <myPushButton.h>
#include <TaskScheduler.h>
#include <TimeLib.h>
#include <myUbidotVariable.h>
#include <Adafruit_NeoPixel.h>


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
// #define     WIFI_HOSTNAME   "PIR_LIAMROOM_LID"
// #define     TOPIC_MOTION   	"/dev/liamroom-lid-motion"
// #define     TOPIC_ONLINE    "/dev/liamroom-lid-online"
// #define     SENSOR_NAME    "LID"
// #define     PIR_PIN         D0    // ESP8266-01: 2, WEMOS D0
// SensorNameType sensor = PIR_LID;

#define     WIFI_HOSTNAME   "PIR_LIAMROOM_BOXEND"
#define     TOPIC_MOTION   "/dev/liamroom-boxend-motion"
#define     TOPIC_ONLINE    "/dev/liamroom-boxend-online"
#define     SENSOR_NAME    	"BOXEND"
#define     PIR_PIN         D0    // ESP8266-01: 2, WEMOS D0
#define 	PIXEL_PIN		D5
SensorNameType sensor = PIR_BOXEND;

bool send_enabled = false;

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
#define NUM_PIXELS	1

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOUR_OFF = pixel.Color(0, 0, 0);
uint32_t COLOUR_MOTION = pixel.Color(10, 0, 0);
uint32_t COLOUR_READY = pixel.Color(0, 10, 0);

/*---------------------------------------------------------------------*/

Scheduler runner;

#define RUN_ONCE    2
#define RUN_TWICE   4

void pirOfflinePeriodCallback();

void tCallback_FlashTriggerLEDON();
void tCallback_FlashTriggerLEDOFF();
Task tFlashMotionLED(50, RUN_ONCE, &tCallback_FlashTriggerLEDON, &runner, false);
void tCallback_FlashTriggerLEDON() {
    pixel.begin();
    pixel.setPixelColor(0, COLOUR_MOTION);
    pixel.show();
	tFlashMotionLED.setCallback(tCallback_FlashTriggerLEDOFF);
}
void tCallback_FlashTriggerLEDOFF() {
    pixel.begin();
    pixel.setPixelColor(0, COLOUR_OFF);
    pixel.show();
	tFlashMotionLED.setCallback(tCallback_FlashTriggerLEDON);
}

void mqttcallback_Timestamp(byte* payload, unsigned int length) {
	wifiHelper.mqttPublish(TOPIC_ONLINE, "1");
}

volatile bool checkPirNow = false;

void tServicePirCallback();
Task tServicePir(500, TASK_FOREVER, &tServicePirCallback, false);
void tServicePirCallback() {
	pir.serviceEvents();
}

/*--------------------------------------------------------------*/

void pir_callback(int eventCode, int eventParams) {

	Serial.println("pir_callback");

	if (eventParams == pir.EV_BUTTON_PRESSED) {
		tFlashMotionLED.restart();
	}

	if (eventParams == pir.EV_BUTTON_PRESSED ||
		eventParams == pir.ST_WAIT_FOR_HELD_TIME ||
		eventParams == pir.EV_HELD_FOR_LONG_ENOUGH ||
		eventParams == pir.ST_WAITING_FOR_RELEASE ||
		eventParams == pir.EV_RELEASED) {

		wifiHelper.mqttPublish(TOPIC_MOTION, "1");
	}
}

/* ----------------------------------------------------------- */

void setup() {

    pixel.begin();
    pixel.setPixelColor(0, COLOUR_READY);
    pixel.show();

    delay(50);

	Serial.begin(9600);
	Serial.println("Booting");

	wifiHelper.setupWifi();

	wifiHelper.setupOTA(WIFI_HOSTNAME);

	wifiHelper.setupMqtt();
	wifiHelper.mqttAddSubscription(TOPIC_TIMESTAMP, mqttcallback_Timestamp);

    pixel.begin();
    pixel.setPixelColor(0, COLOUR_OFF);
    pixel.show();

	//runner.addTask(tFlashMotionLED);
	runner.addTask(tServicePir);

	tServicePir.restart();
}

void loop() {

	runner.execute();

	wifiHelper.loopMqtt();

	ArduinoOTA.handle();

	// if (checkPirNow) {
	// 	pir.serviceEvents();
	// 	checkPirNow = false;
	// }

	delay(10);
}

/*---------------------------------------------------------------------*/
