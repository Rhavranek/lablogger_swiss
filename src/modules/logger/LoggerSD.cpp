#include "application.h"
#include "LoggerSD.h"

void LoggerSD::init() {
    present = false;   
    Serial.printf("INFO: checking for SD card reader at I2C address 0x%02X... ", i2c_address);
    Wire.begin();
    Wire.beginTransmission(i2c_address);
    present = (Wire.endTransmission() == 0);
    (present) ? Serial.println("found.") : Serial.println("NOT found.");
    if (present) {
        Serial.print("INFO: initializing SD card...");    
        present = OpenLog::begin(i2c_address);
        (present) ? Serial.println("complete.") : Serial.println("FAILED - card missing?");
    }
}

bool LoggerSD::available() {
    if (!present) init();
    return(present);
}

bool LoggerSD::syncFile() {
    // reset present if card available but sync fails
    if (available()) present = OpenLog::syncFile();
    if (!present) Serial.println("ERROR: could not write to SD card.");
    return(present);
}