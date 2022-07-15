#pragma once

#include "SparkFun_Qwiic_OpenLog_Arduino_Library.h"

// Display class handles displaying information
class LoggerSD : public OpenLog {

    private:

        // default Qwiic OpenLog I2C address
        const uint8_t i2c_address = 0x2a;

        // sd card present
        bool present = false;

    public:

        // initialize the sd reader
	    void init();

        // check if card is available
        bool available();

        // sync file
        bool syncFile();

};

