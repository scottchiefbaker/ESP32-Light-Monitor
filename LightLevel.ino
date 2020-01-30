// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiManager.h>
WiFiManager wifiManager;
String MAC = "";

#include <HTTPClient.h>
HTTPClient http;

// https://github.com/scottchiefbaker/ESP-WebOTA
#include <WebOTA.h>

//////////////////////////////////////////////////////////////////////////////////////

int pin_max    = 1;  // Store the max calibrated value
const int apin = 34; // Needs to not be on ADC2 https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/

//////////////////////////////////////////////////////////////////////////////////////

void setup() {
	Serial.begin(115200);
	webota.delay(500);

	// Connect to the WiFi or go into "portal" mode to let you set it
	wifiManager.autoConnect();

	// Get the MAC address of the WiFi
	MAC = WiFi.macAddress();

	// Set the analog pin to input
	pinMode(apin, INPUT);

	// Calculate the highest light level
	calibrate();

	char msg[200];
	snprintf(msg, 200, "%s boot up... Calibrated max at %d", MAC.c_str(), pin_max);
	log_message(msg);

	Serial.print("Starting monitor\r\n");
}

void loop() {
	static bool last_report = true;
	static bool first       = true;

	int level       = analogRead(apin);
	int i_percent   = ((level * 100) / (pin_max * 1));

	if (i_percent > 100) {
		i_percent = 100;
	}

	Serial.printf("Level: %d (%d%%)\r\n", level, i_percent);

	// It's "on" if it's higher than 50%
	bool is_on = (level > (pin_max / 2));

	// If the on/off status has changed we phone home
	if (!first && (is_on != last_report)) {
		phone_home(is_on);

		last_report = is_on;
	}

	webota.delay(200);

	first = false;
}

int phone_home(bool is_on) {
	Serial.printf("Phoning home\r\n");

	char url[100] = "";
	if (is_on) {
		snprintf(url, 100, "http://scott.web-ster.com/api/lights.php?status=on&mac=%s", MAC.c_str());
	} else {
		snprintf(url, 100,"http://scott.web-ster.com/api/lights.php?status=off&mac=%s", MAC.c_str());
	}

	http.begin(url);
	int httpCode = http.GET();
	//String payload = http.getString();
	http.end();

	return 1;
}

// Read five light levels and use the max as the ceiling
void calibrate() {
	for (int i = 0; i < 5; i++) {
		int level = analogRead(apin);

		if (level > pin_max) {
			Serial.printf("Calibrating max to: %d\r\n", level);
			pin_max = level;
		}

		webota.delay(1000);
	}
}

int log_message(const char* msg) {
	char url[500] = "";
	snprintf(url, 500, "http://scott.web-ster.com/api/lights.php?msg=%s", msg);

	for (int i = 0; i < 500; i++) {
		if (url[i] == ' ') {
			url[i] = '+';
		}
	}

	http.begin(url);
	int httpCode = http.GET();
	//String payload = http.getString();
	http.end();

	return 1;
}
