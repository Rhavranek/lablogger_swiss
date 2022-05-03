/*
 * Wiring from LCD backpack to photon:
 *   - SDA to pin D0
 *   - SCL to pin D1
 *   - VCC to VIN
 *   - GND to GND
 *
 */
#include "application.h"
#include "LoggerDisplay.h"

// Which display to debug?
//LoggerDisplay *lcd = new LoggerDisplay(16, 2);
LoggerDisplay *lcd = new LoggerDisplay(20, 4);

int last_second = 0;
int last_message = 0;

char device_name[20];
char device_public_ip[20];
byte macAddress[6];

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
unsigned long last_sync = millis();

void info_handler(const char *topic, const char *data)
{
	if (strcmp(topic, "spark/device/name") == 0)
	{
		Serial.println("Device name: " + String(data));
		strncpy(device_name, data, sizeof(device_name));
		lcd->printLine(1, "Name: " + String(device_name));
	}
	else if (strcmp(topic, "spark/device/ip") == 0)
	{
		Serial.println("Device public IP: " + String(data));
		strncpy(device_public_ip, data, sizeof(device_public_ip));
		lcd->printLine(3, "IP address: ");
		lcd->printLine(4, device_public_ip);
	}
	// unsubscribe from events
	if (strlen(device_name) > 0 && strlen(device_public_ip) > 0)
	{
		Particle.unsubscribe();
	}
}

// manual connectivity management
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);
bool connect = false;
bool connected = false;
byte mac[6];

void setup(void) {

	// turn communications module on
	#if PLATFORM_ID == PLATFORM_PHOTON || PLATFORM_ID == PLATFORM_ARGON
	WiFi.on();
	#elif PLATFORM_ID == PLATFORM_BORON
	Cellular.on();
	#else
	#error "PLATFORM_ID not supported by this library"
	#endif

	// start serial
	Serial.begin(9600);
	waitFor(Serial.isConnected, 5000); // give monitor 5 seconds to connect
	delay(500);
	Serial.println("Starting up...");

	// timezone
	Time.zone(-4); // to get the time correctly

	// LCD screen
	lcd->init();
	lcd->setTempTextShowTime(3); // 3 seconds temporary text show time
	lcd->printLine(1, "Starting up...");
	lcd->printLine(2, "extra long super testing");

	// particle subscriptions
	Particle.subscribe("spark/", info_handler, ALL_DEVICES);
	
	// finished
	Serial.println("Setup complete");
}

int test = 0;

void loop(void)
{

	// cloud connection
	if (Particle.connected()) {
		if (!connected) {
			// connection just made
			#if (PLATFORM_ID == PLATFORM_PHOTON || PLATFORM_ID == PLATFORM_ARGON)
			WiFi.macAddress(mac);
			Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x, IP: ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			Serial.println(WiFi.localIP().toString());
			#elif PLATFORM_ID == PLATFORM_BORON
			Serial.print("Cellular connected, IP: ");
			Serial.println(Cellular.localIP().toString());
			#endif
			Serial.println(Time.format(Time.now(), "Cloud connection established at %H:%M:%S %d.%m.%Y"));
			Particle.publish("spark/device/name", PRIVATE);
			Particle.publish("spark/device/ip", PRIVATE);
			connected = true;
		}
		Particle.process();
	} else if (connected) {
		// should be connected but isn't
		Serial.println(Time.format(Time.now(), "Lost cloud connected at %H:%M:%S %d.%m.%Y"));
		connect = false;
		connected = false;
	} else if (!connect) {
		// start cloud connection
		Serial.println(Time.format(Time.now(), "Initiate connection at %H:%M:%S %d.%m.%Y"));
		Particle.connect();
		connect = true;
	}

	// Time sync (once a day)
	if (Particle.connected() && millis() - last_sync > ONE_DAY_MILLIS)
	{
		// Request time synchronization from the Particle Cloud
		Particle.syncTime();
		last_sync = millis();
	}

	// Time update
	if (Time.second() != last_second)
	{
		Serial.println(Time.format(Time.now(), "%H:%M:%S %d.%m.%Y %Z"));
		lcd->printLine(2, Time.format(Time.now(), "%H:%M:%S %d.%m.%Y %Z"));
		last_second = Time.second();
	}

	// Other test messages
	if (millis() - last_message > 5000)
	{

		if (test == 0)
		{
			lcd->resetBuffer();
			lcd->addToBuffer("line 1");
			lcd->addToBuffer(" temp msg");
			lcd->addToBuffer(" extra long");
			lcd->printLineTempFromBuffer(1);
		}
		else if (test == 1)
		{
			lcd->printLineTemp(2, "full left temp");
		}
		else if (test == 2)
		{
			lcd->printLineTempRight(2, "right temp", 11);
		}
		else if (test == 3)
		{
			lcd->printLineTempRight(1, "2 line temp", 12);
			lcd->printLineTempRight(2, "2 line temp", 12);
		}
		else if (test == 4)
		{
			lcd->printLineRight(1, "right perm", 11);
		}
		else if (test == 5)
		{
			lcd->printLineTempRight(1, "TEMP", 4);
		}
		else if (test == 6)
		{
			lcd->printLine(1, " add part left ", 10, 4);
		}
		else if (test == 7)
		{
			lcd->goToLine(1);
			lcd->print("1 ");
			lcd->print("2 ");
			lcd->print("3...");
		}
		else if (test == 8)
		{
			lcd->printLine(1, "Name: " + String(device_name));
		}

		test++;
		last_message = millis();
	}

	lcd->update();
}
