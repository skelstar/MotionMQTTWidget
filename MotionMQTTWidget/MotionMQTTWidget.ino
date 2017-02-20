#include <myWifiHelper.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <myPushButton.h>
#include <TaskScheduler.h>


char versionText[] = "MotionMQTTWidget v0.9";

#define 	WIFI_HOSTNAME   "MotionMQTTWidget"
#define 	LINE_HEIGHT		12
#define 	U8G2_START_Y	12

#define PIR_PIN    		13
#define PULL_UP     	true

/*---------------------------------------------------------------------*/

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R1, SCL, SDA, U8X8_PIN_NONE);

/*---------------------------------------------------------------------*/

void listener_PIR(int eventCode, int eventParams);

myPushButton pir(PIR_PIN, PULL_UP, 100000, LOW, listener_PIR);

/* ------------------------------------------------------------------- */

struct timeType {
	int hour;
	int minute;
};

int timesPtr = 0;
#define MAX_NUM_TIMES	10
timeType t[MAX_NUM_TIMES];

void addTime(int hour, int min) {
	if (timesPtr + 1 < MAX_NUM_TIMES) {
		t[timesPtr].hour = hour;
		t[timesPtr].minute = min;
		timesPtr++;
	}
}

void clearTimes() {
	timesPtr = 0;
}

void listener_PIR(int eventCode, int eventParams) {

	switch (eventParams) {

		case pir.EV_BUTTON_PRESSED:

			Serial.println("PIR");

			addTime(11, 03);

			for (int i=0; i<timesPtr; i++) {
			 	Serial.print(t[i].hour);
			 	Serial.print(':');
			 	Serial.println(t[i].minute);
			}
			break;

        case pir.EV_RELEASED:
            break;
	}
}

/*---------------------------------------------------------------------*/

void tDisplayTimesCallback() {

	Serial.println("tDisplayTimesCallback");
	u8g2.clearBuffer();
	u8g2.setFont(u8g2_font_profont12_tf); // choose a suitable font (10,11,12,15,17,22,29)

	for (int i=0; i<timesPtr; i++) {
		u8g2.setCursor(2, U8G2_START_Y + (i*LINE_HEIGHT));
		u8g2.print(t[i].hour/10);
		u8g2.print(t[i].hour%10);
		u8g2.print(":");
		u8g2.print(t[i].minute/10);
		u8g2.print(t[i].minute%10);
	}
	u8g2.sendBuffer();
}

Scheduler runner;

Task tDisplayTimes(1000, TASK_FOREVER, &tDisplayTimesCallback, &runner, true);

/*---------------------------------------------------------------------*/

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

/*---------------------------------------------------------------------*/

void setup() {
    Serial.begin(9600);
    Serial.println("Booting");

    wifiHelper.setupWifi();
    wifiHelper.setupOTA(WIFI_HOSTNAME);

    wifiHelper.setupMqtt();

    runner.startNow();

    u8g2.begin();
}

void loop() {

    wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    pir.serviceEvents();
    
    runner.execute();

    delay(10);
}

int readPIR() {
	pinMode(PIR_PIN, INPUT);
	int result = digitalRead(PIR_PIN) == 1;
	if (result == 1) {
		Serial.println("PIR ON");
	} else {
		Serial.println("PIR OFF");
	}
	return result;
}

