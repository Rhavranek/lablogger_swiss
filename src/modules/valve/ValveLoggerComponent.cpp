#include "application.h"
#include "ValveLoggerComponent.h"



/*** setup ***/

uint8_t ValveLoggerComponent::setupDataVector(uint8_t start_idx) { 
    // same index to allow for step transition logging
    // idx, key, units, digits
    data.push_back(LoggerData(start_idx + 1, "valve", strdup(id), 0));
    data.push_back(LoggerData(start_idx + 1, "valve", strdup(id), 0));
    return(start_idx + 1); 
}

void ValveLoggerComponent::init() {
    SerialReaderLoggerComponent::init();
    cmd = strdup(id);
    // set starting value and schedule valve for update
    data[0].setNewestValue(state->pos);
    update_valve = true;
}

void ValveLoggerComponent::completeStartup() {
    SerialReaderLoggerComponent::completeStartup();
    // log starting position
    logData();
    Serial.printlnf("INFO: logged %s position at startup - #%.0f", id, data[0].getValue());
}

/*** loop ***/

void ValveLoggerComponent::update() {
    if (update_valve && data_read_status == DATA_READ_IDLE && isPastRequestDelay()) {
        updateValve();
        update_valve = false;
        triggerDataRead(); // trigger serial data read for logging purposes (if anything has changed)
    }
    SerialReaderLoggerComponent::update();
}

/*** state management ***/
    
size_t ValveLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void ValveLoggerComponent::saveState(bool always) { 
    if (ctrl->state->save_state || always) {
        EEPROM.put(eeprom_start, *state);
        if (ctrl->debug_state) {
            Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
        }
    } else {
        Serial.printlnf("DEBUG: component '%s' state NOT saved because state saving is off", id);
    }
} 

bool ValveLoggerComponent::restoreState() {
    ValveState *saved_state = new ValveState();
    EEPROM.get(eeprom_start, *saved_state);
    bool recoverable = saved_state->version == state->version;
    if(recoverable) {
        EEPROM.get(eeprom_start, *state);
        Serial.printf("INFO: successfully restored component state from memory (state version %d)\n", state->version);
    } else {
        Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
        saveState(true);
    }
    return(recoverable);
}

void ValveLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState(true);
}

/*** command parsing ***/

bool ValveLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseMovement(command)) {
    // valve position and direction
  }
  return(command->isTypeDefined());
}

bool ValveLoggerComponent::parseMovement(LoggerCommand *command) {
    if (command->parseVariable(cmd)) {
        command->extractValue();
        command->extractUnits();
        int pos = atoi(command->units);
        if (command->parseValue(CMD_VALVE_DIRECTION)) {
            if (command->parseUnits(CMD_VALVE_DIRECTION_CW))
                command->success(changeDirection(true));
            else if (command->parseUnits(CMD_VALVE_DIRECTION_CC))
                command->success(changeDirection(false));
            else
                command->errorValue(); // invalid value
            getValveStateDirText(cmd, state->cw, command->data, sizeof(command->data));
        } else if (command->parseValue(CMD_VALVE_POSITION) && pos > max_pos) {
            command->error(CMD_VALVE_RET_WARN_MAX_POS, CMD_VALVE_RET_WARN_MAX_POS_TEXT);
        } else if (command->parseValue(CMD_VALVE_POSITION) && pos > 0) {
            command->success(changePosition(pos));
            getValveStatePosText(cmd, state->pos, command->data, sizeof(command->data));
        } else {
            command->errorValue(); // invalid value
        }
    } 
    return(command->isTypeDefined());
}

/*** state/valve changes ***/

bool ValveLoggerComponent::changePosition(uint8_t pos) {
  // only update if necessary
  bool changed = pos != state->pos;
  if (changed) {
    Serial.printlnf("INFO: %s position updating to %d", id, pos);
    state->pos = pos;
    saveState();
  } else {
    Serial.printlnf("INFO: %s position unchanged (%d)", id, state->pos);
  }
  // always update valve just to make sure we're in the correct position
  // NOTE: this assumes that the valve knows NOT to move if it's already in the correct position!!
  update_valve = true;
  return(changed);
}

bool ValveLoggerComponent::changeDirection(bool cw) {
  // only update if necessary
  bool changed = cw != state->cw;
  if (changed) {
    state->cw = cw;
    state->cw ?
        Serial.printlnf("INFO: %s direction changed to %s", id, CMD_VALVE_DIRECTION_CW):
        Serial.printlnf("INFO: %s direction changed to %s", id, CMD_VALVE_DIRECTION_CC);
    saveState();
  } else {
    state->cw ?
        Serial.printlnf("INFO: %s direction unchanged (%s)", id, CMD_VALVE_DIRECTION_CW):
        Serial.printlnf("INFO: %s direction unchanged (%s)", id, CMD_VALVE_DIRECTION_CC);
  }
  return(changed);
}

void ValveLoggerComponent::updateValve() {
    // implement in derived classes to communicate state->pos to the valve
}

/*** logger state variable ***/

void ValveLoggerComponent::assembleStateVariable() {
  char pair[60];
  getValveStateDirText(cmd, state->cw, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getValveStatePosText(cmd, state->pos, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}

/*** manage serial data ***/

void ValveLoggerComponent::finishData() {
    // position
    if (error_counter == 0) {
        // note that this is were the step change is recorded after sending the position signal to the valve
        logStepData( (float) atoi(value_buffer), 0.1);
    }
}

/*** particle webhook data log ***/

void ValveLoggerComponent::logData() {
  // always log data[0] with latest current time
  data[0].setNewestDataTime(millis());
  data[0].saveNewestValue(false);
  SerialReaderLoggerComponent::logData();
}
