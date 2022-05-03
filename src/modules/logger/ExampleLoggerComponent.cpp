#include "application.h"
#include "ExampleLoggerComponent.h"

/*** setup ***/

// setup data vector - override in derived clases, has to return the new index
uint8_t ExampleLoggerComponent::setupDataVector(uint8_t start_idx) { 
    start_idx = LoggerComponent::setupDataVector(start_idx);
    return(start_idx); 
};

void ExampleLoggerComponent::init() {
    LoggerComponent::init();
    Serial.println("example component init");
}

/*** state management ***/

size_t ExampleLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void ExampleLoggerComponent::saveState() { 
    EEPROM.put(eeprom_start, *state);
    if (ctrl->debug_state) {
        Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
    }
} 

bool ExampleLoggerComponent::restoreState() {
    ExampleState *saved_state = new ExampleState();
    EEPROM.get(eeprom_start, *saved_state);
    bool recoverable = saved_state->version == state->version;
    if(recoverable) {
        EEPROM.get(eeprom_start, *state);
        Serial.printf("INFO: successfully restored component state from memory (state version %d)\n", state->version);
    } else {
        Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
        saveState();
    }
    return(recoverable);
}

void ExampleLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState();
}

/*** command parsing ***/

bool ExampleLoggerComponent::parseCommand(LoggerCommand *command) {
    return(parseSetting(command));
}

bool ExampleLoggerComponent::parseSetting(LoggerCommand *command) {
    // decision tree
    if (command->parseVariable(CMD_SETTING)) {
    command->extractValue();
    if (command->parseValue(CMD_SETTING_ON)) {
        command->success(changeStateSetting(true));
    } else if (command->parseValue(CMD_SETTING_OFF)) {
        command->success(changeStateSetting(false));
    }
    getStateSettingText(state->setting, command->data, sizeof(command->data));
    }
    return(command->isTypeDefined());
}

/*** state changes ***/

bool ExampleLoggerComponent::changeStateSetting (bool on) {
    bool changed = on != state->setting;

    if (changed) state->setting = on;
    
    if (ctrl->debug_state) {
        if (changed)
            on ? Serial.println("DEBUG: setting turned yay") : Serial.println("DEBUG: setting turned nay");
        else
            on ? Serial.println("DEBUG: setting already yay") : Serial.println("DEBUG: setting already nay");
    }

    if (changed) saveState();

    return(changed);
}

/*** state info to LCD display ***/

void ExampleLoggerComponent::updateDisplayStateInformation() {
    ctrl->lcd->resetBuffer();
    getStateSettingText(state->setting, ctrl->lcd->buffer, sizeof(ctrl->lcd->buffer), true);
    ctrl->lcd->printLineFromBuffer(2);
}

/*** logger state variable ***/

void ExampleLoggerComponent::assembleStateVariable() {
    char pair[60];
    getStateSettingText(state->setting, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}
