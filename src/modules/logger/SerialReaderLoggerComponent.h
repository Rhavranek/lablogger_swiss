#pragma once
#include "DataReaderLoggerComponent.h"

/*** serial data parameters ***/

// special ascii characters (actual byte values)
#define SERIAL_B_TAB        9  // tab
#define SERIAL_B_CR         13 // \r
#define SERIAL_B_NL         10 // \n
#define SERIAL_B_PLUS       43 // +
#define SERIAL_B_MINUS      45 // -
#define SERIAL_B_DOT        46 // .
#define SERIAL_B_0          48 // 0
#define SERIAL_B_9          57 // 9
#define SERIAL_B_C_START    32 // first regular character (space)
#define SERIAL_B_C_END      126 // last regular character (~)

// common serial patterns
#define SERIAL_P_ANY        -10 // any byte --> > 0
#define SERIAL_P_ASCII      -11 // ascii character --> 32-126
#define SERIAL_P_DIGIT      -12 // [0-9] --> 48 - 57
#define SERIAL_P_NUMBER     -13 // [+-.0-9] --> 43, 45, 46, 48 - 57


/* component */
class SerialReaderLoggerComponent : public DataReaderLoggerComponent
{

  protected:

    // serial communication config
    const long serial_baud_rate;
    const long serial_config;
    unsigned int min_request_delay = 200; // recommended minimum delay since last data received [in ms]
    unsigned int timeout = 1000; // timeout if no serial data receivedå

    // serial data
    const char *request_command;
    unsigned int n_byte = 0;
    unsigned int data_pattern_pos = 0;
    unsigned int data_pattern_size = 0;
    bool stay_on = false;
    int stay_on_pattern = 0;
    byte prev_byte;
    byte new_byte;

    // buffers
    char data_buffer[2000];
    int data_charcounter;
    char variable_buffer[50];
    int variable_charcounter;
    char value_buffer[50];
    int value_charcounter;
    char units_buffer[50];
    int units_charcounter;

  public:

    /*** constructors ***/
    // serial data readers are by default all sequential (last parameter to DataReaderLoggerComponent) because they usually receive/transmit over the same line
    SerialReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, const long baud_rate, const long serial_config, const char *request_command, unsigned int data_pattern_size) : 
      DataReaderLoggerComponent(id, ctrl, data_have_same_time_offset, true), serial_baud_rate(baud_rate), serial_config(serial_config), request_command(request_command), data_pattern_size(data_pattern_size) {}
    SerialReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, const long baud_rate, const long serial_config, const char *request_command) : 
      SerialReaderLoggerComponent(id, ctrl, data_have_same_time_offset, baud_rate, serial_config, request_command, 0) {}

    /*** setup ***/
    virtual void init();

    /*** read data ***/
    virtual bool isPastRequestDelay();
    virtual bool isTimeForRequest();
    virtual bool isTimedOut();
    virtual void sendSerialDataRequest();
    virtual void idleDataRead();
    virtual void initiateDataRead();
    virtual void readData();
    virtual void completeDataRead();
    virtual void registerDataReadError();
    virtual void handleDataReadTimeout();

    /*** manage data ***/
    virtual void startData();
    virtual void processNewByte();
    virtual void finishData();

    /*** debug variable ***/
    virtual void assembleDebugVariable();

    /*** work with data patterns ***/
    void nextPatternPos();
    void stayOnPattern(int pattern);
    bool matchesPattern(byte b, int pattern);
    bool moveStayedOnPattern();

    /*** interact with serial data buffers ***/
    void resetSerialBuffers(); // reset all buffers
    void resetSerialDataBuffer();
    void resetSerialVariableBuffer();
    void resetSerialValueBuffer();
    void resetSerialUnitsBuffer();

    void appendToSerialDataBuffer (byte b); // append to total serial data buffer
    void appendToSerialVariableBuffer (byte b);
    void appendToSerialValueBuffer (byte b);
    void appendToSerialUnitsBuffer (byte b);

    void setSerialVariableBuffer (char* var);
    void setSerialValueBuffer(char* val);
    void setSerialUnitsBuffer(char* u);

};
