#pragma once
#include "LoggerComponent.h"
#include "LoggerController.h"
#include "LoggerDisplay.h"

// data read status codes
#define DATA_READ_IDLE      0 // data reader not engaged
#define DATA_READ_REQUEST   1 // make a read request
#define DATA_READ_WAITING   2 // waiting for (more) data
#define DATA_READ_COMPLETE  3 // all data received
#define DATA_READ_TIMEOUT   4 // read timed out

/* component */
class DataReaderLoggerComponent : public LoggerComponent
{

  protected:

    // reader properties
    bool sequential = false; // whether this reader belongs to the sequential readers that don't run in parallel with each other

    // data reading
    uint8_t triggered_read_attempts = 0; // for out-of-timeline read triggers (makes up to x attempts to read)
    uint8_t data_read_status = DATA_READ_IDLE;
    unsigned long data_read_start = 0; // time the read started
    unsigned long data_received_last = 0; // last time data was received
    unsigned int error_counter = 0; // number of errors encountered during the read

  public:

    /*** constructors ***/
    // data clearing usually managed automatically -> set to true
    DataReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, bool sequential) : LoggerComponent(id, ctrl, data_have_same_time_offset, true), sequential(sequential) {};
    // data readers by default are non-blocking
    DataReaderLoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset) : DataReaderLoggerComponent(id, ctrl, data_have_same_time_offset, false) {};

    /*** loop ***/
    virtual void update();

    /*** read data ***/
    void triggerDataRead(uint8_t n_attempts = 10);
    bool isTriggeredDataRead();
    virtual bool isManualDataReader();
    virtual bool isTimeForRequest();
    virtual bool isTimedOut();
    virtual void returnToIdle();
    virtual void idleDataRead();
    virtual void initiateDataRead();
    virtual void readData();
    virtual void completeDataRead();
    virtual void registerDataReadError();
    virtual void handleDataReadTimeout();
    virtual void handleFailedTriggeredDataRead();
    
    /*** manage data ***/
    virtual void startData();
    virtual void finishData();

    /*** debug variable ***/
    virtual void assembleDebugVariable();

};
