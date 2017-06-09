#include <myWifiHelper.h>
#include <myPushButton.h>
#include <ArduinoJson.h>            // https://github.com/bblanchon/ArduinoJson
#include <TaskScheduler.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>


char versionText[] = "MotionMQTTWidget v3.0";

#define     WIFI_HOSTNAME   	"/liamshouse/motion"

#define 	TOPIC_TIMESTAMP     "/dev/timestamp"
#define 	TOPIC_COMMAND		"/liamshouse/motion/command"
#define     TOPIC_ONLINE        "/liamshouse/motion/online"
#define     TOPIC_EVENT         "/liamshouse/motion/event"

#define 	PIR_PIN		2
#define 	PIXEL_PIN	0

// #define     WIFI_HOSTNAME   	"/liamsroom/motion"

// #define 	TOPIC_TIMESTAMP     "/dev/timestamp"
// #define 	TOPIC_COMMAND		"/liamsroom/motion/command"
// #define     TOPIC_ONLINE        "/liamsroom/motion/online"
// #define     TOPIC_EVENT         "/liamsroom/motion/event"

// #define     PIR_PIN         D0	// WEMOS D0, ESP 2
// #define 	PIXEL_PIN		D1	// WEMOS D1

//--------------------------------------------------------------------------------

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

//--------------------------------------------------------------------------------

#define     PULL_UP                 true
#define     NO_PULL_UP              false
#define     NORMAL_LOW              LOW
#define     NORMAL_HIGH             HIGH

void pir_callback(int eventCode, int eventParams);
myPushButton pir(PIR_PIN, PULL_UP, 100000, NORMAL_LOW, pir_callback);
void pir_callback(int eventCode, int eventParams) {

	switch (eventParams) {
		case pir.EV_BUTTON_PRESSED:
			wifiHelper.mqttPublish(TOPIC_EVENT, "Motion");
			break;
	}
}

//--------------------------------------------------------------------------------
#define 	NUM_PIXELS		1

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(NUM_PIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOUR_OFF = pixel.Color(0, 0, 0);
uint32_t COLOUR_MOTION = pixel.Color(10, 0, 0);

//--------------------------------------------------------------------------------
Scheduler runner;

#define RUN_ONCE    2

void tFlashLED_ON_Callback();
void tFlashLED_OFF_Callback();
Task tFlashLED(50, RUN_ONCE, &tFlashLED_OFF_Callback, &runner, false);
// for some reason it is triggering ON_Callback first. Weird.
void tFlashLED_ON_Callback() {
    pixel.begin();
    pixel.setPixelColor(0, COLOUR_MOTION);
    pixel.show();
    tFlashLED.setCallback(tFlashLED_OFF_Callback);
}
void tFlashLED_OFF_Callback() {
    pixel.begin();
    pixel.setPixelColor(0, COLOUR_OFF);
    pixel.show();
    tFlashLED.setCallback(tFlashLED_ON_Callback);
}

//--------------------------------------------------------------------------------
void mqttcallback_Timestamp(byte* payload, unsigned int length) {
	wifiHelper.mqttPublish(TOPIC_ONLINE, "1", false);
}

void mqttcallback_Command(byte *payload, unsigned int length) {

    JsonObject& root = wifiHelper.mqttGetJson(payload);
    const char* command = root["command"];
    const char* value = root["value"];

    if (strcmp(command, "LED") == 0) {
    	if (strcmp(value, "FLASH") == 0) {
    		tFlashLED.restart();
    	}
    }
}

/* ----------------------------------------------------------- */

void setup() {

	Serial.begin(9600);
	Serial.println("Booting");

	pixel.begin();
	pixel.setPixelColor(0, pixel.Color(0,10,0));
	pixel.show();

	wifiHelper.setupWifi();

	wifiHelper.setupOTA(WIFI_HOSTNAME);

	wifiHelper.setupMqtt();
	wifiHelper.mqttAddSubscription(TOPIC_TIMESTAMP, mqttcallback_Timestamp);
    wifiHelper.mqttAddSubscription(TOPIC_COMMAND, mqttcallback_Command);

    delay(100);
	wifiHelper.loopMqtt();
	wifiHelper.mqttPublish(TOPIC_EVENT, "Ready");

	pixel.begin();
	pixel.setPixelColor(0, COLOUR_OFF);
	pixel.show();
}

void loop() {

	wifiHelper.loopMqtt();

	ArduinoOTA.handle();

    pir.serviceEvents();

    runner.execute();

	delay(10);
}

/*---------------------------------------------------------------------*/
