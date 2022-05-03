#include "application.h"
#include "Particle.h"

// generic blink test

// constants
const int led = D7; // This one is the little blue LED on your board. On the Photon it is next to D7, and on the Core it is next to the USB jack.
const unsigned int interval = 2000; // how many ms to wait between blinks
byte mac[6];

// process variables
long unsigned int timer = 0;
bool on = true;
bool connect = false;
bool connected = false;

// device info
char device_name[20];
char device_public_ip[20];

// info handler
void info_handler(const char *topic, const char *data) {
    if ( strcmp (topic, "spark/device/name") == 0) {
        Serial.println("Found device name: " + String(data));
        strncpy ( device_name, data, sizeof(device_name) );
    } else if ( strcmp(topic, "spark/device/ip") == 0) {
        Serial.println("Found device public IP: " + String(data));
        strncpy (device_public_ip, data, sizeof(device_public_ip));
    }
    // unsubscribe from events
    if ( strlen(device_name) > 0 && strlen(device_public_ip) > 0) {
        Particle.unsubscribe();
    }
}

// enable sysstem treading
SYSTEM_THREAD(ENABLED);

// manual mode
SYSTEM_MODE(MANUAL);

void setup() {
  
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
  Time.zone(-6); // to get the time correctly

  // led setup
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);

  // particle subscriptions
  Particle.subscribe("spark/", info_handler, ALL_DEVICES);
  
  // finished
  Serial.println("Setup complete");

  #if PLATFORM_ID == PLATFORM_PHOTON || PLATFORM_ID == PLATFORM_ARGON
  WiFi.macAddress(mac);
  Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  #endif
}

void loop() {

  // blinker
  if (millis() - timer > interval) {
    if (on) {
      // turn off
      digitalWrite(led, LOW);
      Serial.println(Time.format(Time.now(), "Turning LED off at %H:%M:%S %d.%m.%Y"));
    } else {
      // turn on
      digitalWrite(led, HIGH);
      Serial.println(Time.format(Time.now(), "Turning LED on at %H:%M:%S %d.%m.%Y"));
    }
    on = !on;
    timer = millis();
  }

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



}
