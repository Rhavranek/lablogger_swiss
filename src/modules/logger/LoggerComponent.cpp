#include "application.h"
#include "LoggerComponent.h"
#include "LoggerController.h"

/*** debug ***/
void LoggerComponent::debug() {
    debug_component = true;
}

/*** setup ***/

// setup data vector - override in derived clases, has to return the new index
uint8_t LoggerComponent::setupDataVector(uint8_t start_idx) { 
    Serial.printf("INFO: constructing data vector for component '%s' starting at index %d\n", id, start_idx);
    return(start_idx); 
};

void LoggerComponent::init() {
    Serial.printf("INFO: initializing component '%s'...\n", id);
};

void LoggerComponent::completeStartup() {
    Serial.printf("INFO: completing startup for component '%s'...\n", id);
}

/*** loop ***/

void LoggerComponent::update() {
}

/*** state management ***/

void LoggerComponent::setEEPROMStart(size_t start) { 
    eeprom_start = start; 
}

size_t LoggerComponent::getStateSize() { 
    return(0); 
}

void LoggerComponent::loadState(bool reset) {
  if (getStateSize() > 0) {
    if (!reset){
        Serial.printf("INFO: trying to restore state from memory for component '%s'\n", id);
        restoreState();
    } else {
        Serial.printf("INFO: resetting state for component '%s' back to default values\n", id);
        saveState();
    }
  }
};

void LoggerComponent::saveState(){ 

};

bool LoggerComponent::restoreState(){ 
    return(false); 
};

void LoggerComponent::resetState() {

};

/*** command parsing ***/

bool LoggerComponent::parseCommand(LoggerCommand *command) {
    return(false);
};

/*** state changes ***/

void LoggerComponent::activateDataLogging() {
    
};

/*** state info to LCD display ***/

void LoggerComponent::updateDisplayStateInformation() {
    
};

/*** logger state variable ***/

void LoggerComponent::assembleStateVariable() {

};

/*** logger data variable ***/

void LoggerComponent::assembleDataVariable() {
  int last_idx = -1;
  for (int i=0; i<data.size(); i++) {
    if (data[i].isEnabled() && (!data[i].isDebugOnly() || ctrl->state->debug_mode)) {
        if (data[i].idx != last_idx || data[i].newest_value_valid) {
          // skip data variables that are index repeats for step logging with no data in the second step
          data[i].assembleInfo();
          ctrl->addToDataVariableBuffer(data[i].json);
        }
        last_idx = data[i].idx;
    }
  }
};

/*** logger debug variable ***/

void LoggerComponent::assembleDebugVariable() {
  
};


/*** particle webhook data log ***/

void LoggerComponent::clearData(bool clear_persistent) {
    if (auto_clear_data) {
        if (ctrl->debug_data) {
            (clear_persistent) ?
                Serial.printf("DEBUG: clearing all component '%s' data at ", id):
                Serial.printf("DEBUG: clearing only non-persistant component '%s' data at ", id);
            Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
        }
        for (int i=0; i<data.size(); i++) data[i].clear(clear_persistent);
    }
};

void LoggerComponent::logData() {
    last_data_log_index = -1;
    // chunked data logging
    while (assembleDataLog()) {
        if (ctrl->debug_cloud) {
            Serial.printf("DEBUG: assembled data log for component '%s' from index %d to %d\n", id, first_data_log_index, last_data_log_index);
        }
        ctrl->queueDataLog();
    }
};

void LoggerComponent::logStepData(float new_value, float change_cutoff, uint8_t data_i, uint8_t step_i) {
    // make change if data not yet set or if it has changed
    
    if (!data[data_i].newest_value_valid || fabs(new_value - data[data_i].getValue()) > change_cutoff ) {
    
        // save previous data in data[step_i] for the data log step transition
        if (data[data_i].newest_value_valid) {
            data[step_i].setNewestDataTime(millis() - 1); // old value logged 1 ms before new value
            data[step_i].setNewestValue(data[data_i].getValue());
            data[step_i].saveNewestValue(false); // no averaging
        } 

        // set new value
        data[data_i].setNewestValue(new_value);
        data[data_i].setNewestDataTime(millis());
        data[data_i].saveNewestValue(false);

        // log data
        logData();

        // clear the step transition data[step_i]
        data[step_i].clear();

        // update controller data variable
        ctrl->updateDataVariable();
    }
}

bool LoggerComponent::assembleDataLog() {

    // first reporting index
    first_data_log_index = last_data_log_index + 1;
    
    // check if we're already done with all data
    if (first_data_log_index >= data.size()) return(false);

    // check first next data (is there at least one with data?)
    bool something_to_report = false;
    int i = first_data_log_index;
    for (; i < data.size(); i++) {
        if(data[i].isEnabled() && (!data[i].isDebugOnly() || ctrl->state->debug_mode) && data[i].getN() > 0) {
            // found data that has something to report
            something_to_report = true;
            break;
        }
    }

    // nothing to report
    if (!something_to_report) return(false);

    // reset the log & buffers
    ctrl->resetDataLog();

    // all data that fits
    i = first_data_log_index;
    for(; i < data.size(); i++) {
        if(data[i].isEnabled() && (!data[i].isDebugOnly() || ctrl->state->debug_mode) && data[i].assembleLog(!data_have_same_time_offset)) {
            if (!ctrl->addToDataLogBuffer(data[i].json)) {
                // no more space - stop here for this log
                break;
            }
        }
        last_data_log_index = i;
    }

    // finalize data log
    if (data_have_same_time_offset) {
        unsigned long common_time = millis() - (unsigned long) data[first_data_log_index].getDataTime();
        return(ctrl->finalizeDataLog(true, common_time));
    } else {
        return(ctrl->finalizeDataLog(false));
    }
    return(true);
}
