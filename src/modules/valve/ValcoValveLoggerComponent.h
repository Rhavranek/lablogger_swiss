#pragma once
#include "ValveLoggerComponent.h"

/*** serial data parameters ***/
// FIXME not sure this is true
const int VALVE_DATA_PATTERN[] = {SERIAL_P_DIGIT, SERIAL_B_CR};

/*** component ***/
class ValcoValveLoggerComponent : public ValveLoggerComponent
{

  public:

    /*** constructors ***/
    ValcoValveLoggerComponent (const char *id, LoggerController *ctrl, ValveState* state, uint8_t max_pos) : 
        ValveLoggerComponent(
            id, ctrl, state, 
            /* baud rate */             9600,
            /* serial config */         SERIAL_8N1,
            /* max pos */               max_pos,
            /* request command */       "CP\r",
            /* data pattern size */     sizeof(VALVE_DATA_PATTERN) / sizeof(VALVE_DATA_PATTERN[0])
        ) {}
    ValcoValveLoggerComponent (const char *id, LoggerController *ctrl, uint8_t max_pos) : 
        ValcoValveLoggerComponent(id, ctrl, new ValveState(), max_pos) {}

    /*** manage data ***/
    virtual void processNewByte();
    
    /*** valve functions ***/
    virtual void updateValve(); 

};

/*** manage data ***/

void ValcoValveLoggerComponent::processNewByte() {

    // keep track of all data
    SerialReaderLoggerComponent::processNewByte();

    // check whether to move on from stayed on pattern
    moveStayedOnPattern();

    // process current pattern
    int pattern = VALVE_DATA_PATTERN[data_pattern_pos];
    if ( pattern == SERIAL_P_DIGIT && matchesPattern(new_byte, pattern)) {
        // number value (don't move pattern forward, could be multiple numbers)
        appendToSerialValueBuffer(new_byte);
        stayOnPattern(pattern);
    } else if (pattern > 0 && new_byte == pattern) {
        // specific ascii characters
        data_pattern_pos++;
    } else {
        // unrecognized part of data --> error
        registerDataReadError();
        data_pattern_pos++;
    }
}

/*** valve functions ***/

void ValcoValveLoggerComponent::updateValve() {
    ValveLoggerComponent::updateValve();
    char cmd[20];
    if (state->cw) {
        Serial.printlnf("INFO: moving valve '%s' in %s direction to pos #%d", id, CMD_VALVE_DIRECTION_CW, state->pos);
        snprintf(cmd, sizeof(cmd), "CW%d\r", state->pos);
        Serial1.print(cmd); 
    } else {
        Serial.printlnf("INFO: moving valve '%s' in %s direction to pos #%d", id, CMD_VALVE_DIRECTION_CC, state->pos);
        snprintf(cmd, sizeof(cmd), "CC%d\r", state->pos);
        Serial1.print(cmd); 
    }
}