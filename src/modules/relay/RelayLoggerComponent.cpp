#include "application.h"
#include "RelayLoggerComponent.h"

/*** setup ***/

uint8_t RelayLoggerComponent::setupDataVector(uint8_t start_idx) { 
    // same index to allow for step transition logging
    // idx, key, units, digits
    data.push_back(LoggerData(start_idx + 1, "relay", strdup(id), 0));
    data.push_back(LoggerData(start_idx + 1, "relay", strdup(id), 0));
    return(start_idx + 1); 
}

void RelayLoggerComponent::init() {
    ControllerLoggerComponent::init();
    pinMode(relay_pin, OUTPUT);
    // updates relay but does not cause data log because start-up is not yet complete
    data[0].setNewestValue(state->on ? 1.0 : 0.0);
    updateRelay(); 
}

void RelayLoggerComponent::completeStartup() {
    ControllerLoggerComponent::completeStartup();
    // log starting position
    logData();
    (state->on) ?
        Serial.printf("INFO: logged '%s' value at startup: on (%.1f))\n", id, data[0].getValue()):
        Serial.printf("INFO: logged '%s' value at startup: off (%.1f)\n", id, data[0].getValue());
}

/*** state management ***/
    
size_t RelayLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void RelayLoggerComponent::saveState(bool always) { 
    if (ctrl->state->save_state || always) {
        EEPROM.put(eeprom_start, *state);
        if (ctrl->debug_state) {
            Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
        }
    } else {
        Serial.printlnf("DEBUG: component '%s' state NOT saved because state saving is off", id);
    }
} 

bool RelayLoggerComponent::restoreState() {
    RelayState *saved_state = new RelayState();
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

void RelayLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState(true);
}

/*** command parsing ***/

bool RelayLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseRelay(command)) {
    // check for relay on/off command
  }
  return(command->isTypeDefined());
}

bool RelayLoggerComponent::parseRelay(LoggerCommand *command) {
    if (command->parseVariable(cmd)) {
        command->extractValue();
        if (command->parseValue(CMD_RELAY_ON)) {
            command->success(changeRelay(true));
        } else if (command->parseValue(CMD_RELAY_OFF)) {
            command->success(changeRelay(false));
        } else {
            command->errorValue(); // invalid value
        }
        getRelayStateText(cmd, state->on, command->data, sizeof(command->data));
    } 
    return(command->isTypeDefined());
}

/*** state changes ***/

bool RelayLoggerComponent::changeRelay(bool on) {

  // only update if necessary
  bool changed = on != state->on;
  if (changed) {
    state->on = on;
    saveState();
    on ? Serial.printlnf("INFO: relay %s turned on", id) : Serial.printlnf("INFO: relay %s turned off", id);
  } else {
    on ? Serial.printlnf("INFO: relay %s already on", id) : Serial.printlnf("INFO: relay %s already off", id);
  }
  // always update relay just to make sure it's set correctly
  updateRelay();
  return(changed);
}

/*** relay functions ***/

void RelayLoggerComponent::updateRelay() {

    // set relay on/off
    if (relay_type == RELAY_NORMALLY_OPEN)
        state->on ? digitalWrite(relay_pin, HIGH) : digitalWrite(relay_pin, LOW);
    else if (relay_type == RELAY_NORMALLY_CLOSED)
        state->on ? digitalWrite(relay_pin, LOW) : digitalWrite(relay_pin, HIGH);
    else
        Serial.println("ERROR: should never get here");

    // log the data step
    logStepData(state->on ? 1.0 : 0.0, 0.1);
};

/*** state changes ***/

void RelayLoggerComponent::activateDataLogging() {
    logData();
}


/*** logger state variable ***/

void RelayLoggerComponent::assembleStateVariable() {
  char pair[60];
  getRelayStateText(cmd, state->on, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}

/*** particle webhook data log ***/

void RelayLoggerComponent::logData() {
  // always log data[0] with latest current time
  data[0].setNewestDataTime(millis());
  data[0].saveNewestValue(false);
  ControllerLoggerComponent::logData();
}
