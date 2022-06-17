#include "application.h"
#include "SchedulerLoggerComponent.h"

/*** setup ***/

uint8_t SchedulerLoggerComponent::setupDataVector(uint8_t start_idx) { 
    // same index to allow for step transition logging
    // idx, key, units, digits
    data.push_back(LoggerData(start_idx + 1, "scheduler", strdup(id), 0));
    data.push_back(LoggerData(start_idx + 1, "scheduler", strdup(id), 0));
    return(start_idx + 1); 
}

void SchedulerLoggerComponent::init() {
    ControllerLoggerComponent::init();
    // set starting value from state
    data[0].setNewestValue(state->status == SCHEDULE_RUNNING ? 1.0 : 0.0);
    if (state->status == SCHEDULE_RUNNING) {
        // resume schedule execution where left off
        schedule_i = state->saved_step;
        schedule_last = state->saved_time;
        schedule_wait = schedule[schedule_i].wait;
    }

}

void SchedulerLoggerComponent::completeStartup() {
    ControllerLoggerComponent::completeStartup();
    // startup is complete, check schedule and start (includes log) or simply log current value
    logData();
    if (state->status == SCHEDULE_COMPLETE) {
        Serial.printf("INFO: logged %s value at startup: complete (%.1f)\n", id, data[0].getValue());
    } else if (state->status == SCHEDULE_RUNNING) {
        Serial.printf("INFO: logged %s value at startup: running (%.1f)\n", id, data[0].getValue());
    } else if (state->status == SCHEDULE_WAITING) {
        Serial.printf("INFO: logged %s value at startup: waiting (%.1f)\n", id, data[0].getValue());
    } else {
        Serial.printf("INFO: logged %s value at startup: unscheduled (%.1f)\n", id, data[0].getValue());
    }
}


/*** loop ***/
void SchedulerLoggerComponent::update() {
    ControllerLoggerComponent::update();
    // only have time available for sure after startup is complete
    if (ctrl->isStartupComplete()) {
        // continue to run schedule if we're running or testing
        if (state->status == SCHEDULE_RUNNING || testing) runSchedule();
        if (checkSchedule() || start_testing) startSchedule();
    }
}

bool SchedulerLoggerComponent::checkSchedule() {
    if (state->status == SCHEDULE_WAITING && difftime(Time.now(), state->tstart) > 0) {
        // tstart is set and has been exceeded (yet schedule is not running yet)
        return(true);
    }
    return(false);
}

void SchedulerLoggerComponent::startSchedule() {
    
    // check if we're in testing mode
    if (start_testing) {
        testing = true;
        start_testing = false;
    }

    // info
    (testing) ?
        Serial.printf("INFO: testing schedule '%s' started at ", id) :
        Serial.printf("INFO: schedule '%s' started at ", id);
    Serial.print(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
    (testing && testing_waits > 0) ?
        Serial.printlnf(" with %d second test wait times for each event", testing_waits/1000) :
        Serial.println();

    // start values
    schedule_i = 0;
    schedule_last = Time.now();
    schedule_wait = (testing && testing_waits > 0) ? testing_waits : schedule[schedule_i].wait;

    // save state if not testing
    if (!testing) {
        state->status = SCHEDULE_RUNNING;
        state->saved_step = schedule_i;
        state->saved_time = schedule_last;
        saveState();
    }

    // log step change
    logStepData(1.0, 0.1);
}

void SchedulerLoggerComponent::runSchedule() {
    if ( round(difftime(Time.now(), schedule_last)) > schedule_wait) {
        // info
        (testing) ? Serial.print("INFO: testing schedule") : Serial.print("DEBUG: schedule");
        Serial.printf(" '%s' event #%d/%d: %s (%s) at ", id, schedule_i + 1, schedule_length, schedule[schedule_i].label, schedule[schedule_i].description);
        Serial.print(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));
        Serial.printlnf(" after a %.0f min %.0f second wait.", floor(schedule_wait/60.), fmod(schedule_wait, 60.));
        // run event
        runEvent(schedule[schedule_i].event);
        schedule_i++;
        schedule_last = Time.now();
        if (schedule_i >= schedule_length) {
            finishSchedule();
        } else {
            schedule_wait = (testing && testing_waits > 0) ? testing_waits : schedule[schedule_i].wait;
            // save state if not testing
            if (!testing) {
                state->saved_step = schedule_i;
                state->saved_time = schedule_last;
                saveState();
            }
        }
    }
}

void SchedulerLoggerComponent::runEvent(uint8_t event) {
    // overload in derived classes
}

void SchedulerLoggerComponent::getSchedulerStatus(char* target, int size) {
    int diff = 0;
    if (testing || state->status == SCHEDULE_RUNNING) {
        if (!ctrl->isStartupComplete()) {
            snprintf(target, size, "%s: starting up...", id); 
        } else {
            diff = schedule_wait - round(difftime(Time.now(), schedule_last));
            (diff > 60) ?
                snprintf(target, size, "%s: %s in %.0fm%.0fs", id, schedule[schedule_i].label, floor(diff/60.), fmod(diff, 60.)) :
                snprintf(target, size, "%s: %s in %ds", id, schedule[schedule_i].label, diff);
        }
    } else if (state->status == SCHEDULE_COMPLETE) {
        snprintf(target, size, "%s: complete", id); 
    } else if (state->status == SCHEDULE_WAITING) {
        diff = round(difftime(state->tstart, Time.now()));
        snprintf(target, size, "%s: %.0fd%.0fh%.0fm%.0fs", id, floor(diff/(3600.*24.)), fmod(floor(diff/3600.), 24.), fmod(floor(diff/60.), 60.), fmod(diff, 60.));
    } else {
        snprintf(target, size, "%s: unscheduled", id);
    }
}

void SchedulerLoggerComponent::finishSchedule() {
    // info
    (testing) ?
        Serial.printf("INFO: testing schedule '%s' finished at ", id) :
        Serial.printf("INFO: schedule '%s' finished at ", id);
    Serial.println(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z"));

    // save state if not testing
    if (!testing) {
        state->status = SCHEDULE_COMPLETE;
        state->saved_time = Time.now();
        saveState();
    }
    testing = false;

    // log step change
    logStepData(0.0, 0.1);
}

/*** state management ***/
    
size_t SchedulerLoggerComponent::getStateSize() { 
    return(sizeof(*state));
}

void SchedulerLoggerComponent::saveState(bool always) { 
    if (ctrl->state->save_state || always) {
        EEPROM.put(eeprom_start, *state);
        if (ctrl->debug_state) {
            Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
        }
    } else {
        Serial.printlnf("DEBUG: component '%s' state NOT saved because state saving is off", id);
    }
} 

bool SchedulerLoggerComponent::restoreState() {
    SchedulerState *saved_state = new SchedulerState();
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

void SchedulerLoggerComponent::resetState() {
    state->version = 0; // force reset of state on restart
    saveState(true);
}

/*** command parsing ***/

bool SchedulerLoggerComponent::parseCommand(LoggerCommand *command) {
  if (parseSchedule(command)) {
    // check for scheduling command
  }
  return(command->isTypeDefined());
}

bool SchedulerLoggerComponent::parseSchedule(LoggerCommand *command) {
    if (command->parseVariable(cmd)) {
        command->extractValue();
        if (command->parseValue(CMD_SCHEDULER_SET)) {
            command->extractUnits(2);

            // create empty time object
            struct tm timeobj;
            timeobj.tm_year = 0;
            timeobj.tm_mon = 0;
            timeobj.tm_mday = 0;
            timeobj.tm_hour = 0;
            timeobj.tm_min = 0;
            timeobj.tm_sec = 0;

            // parse with pattern
            if (debug_component) Serial.printlnf("DEBUG: parsing scheduler %s set schedule command with value '%s' and pattern %s", id, command->units, pattern);
            if (strptime(command->units, pattern, &timeobj)) {
                command->success(changeSchedule(timeobj));
                getSchedulerStateTimeStart(cmd, state->tstart, command->data, sizeof(command->data));
            } else {
                command->errorValue();
            }
        } else if (command->parseValue(CMD_SCHEDULER_RESET)) {
            command->success(resetSchedule());
            getStateStringText(cmd, CMD_SCHEDULER_RESET, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
        } else if (command->parseValue(CMD_SCHEDULER_TEST)) {
            command->extractUnits();
            int waits = atoi(command->units); // in seconds
            command->success(testSchedule(waits));
            getStateStringText(cmd, CMD_SCHEDULER_TEST, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
        } else {
            command->errorValue(); // invalid value
        }
    } 
    return(command->isTypeDefined());
}

/*** state changes ***/

bool SchedulerLoggerComponent::changeSchedule(tm timeobj) {
    // correct for timezone from state->tz
    timeobj.tm_hour = timeobj.tm_hour - ctrl->state->tz;
    time_t timestamp = mktime(&timeobj);
    bool changed = difftime(timestamp, state->tstart) != 0;

    // only update if necessary
    if (changed) {
        state->tstart = timestamp;
        state->status = SCHEDULE_WAITING;
        state->saved_step = 0;
        state->saved_time = 0;
        saveState(true);
        Serial.printf("INFO: new date & time scheduled for %s: ", id);
        Serial.println(Time.format(state->tstart, "%Y-%m-%d %H:%M (UTC)"));
    } else {
        Serial.printf("INFO: date & time for %s unchanged: ", id);
        Serial.println(Time.format(state->tstart, "%Y-%m-%d %H:%M (UTC)"));
    }
    return(changed);
}

bool SchedulerLoggerComponent::resetSchedule() {
    state->status = SCHEDULE_UNSCHEDULED;
    state->tstart = 0;
    state->saved_step = 0;
    state->saved_time = 0;
    saveState(true);
    return(true);
}

bool SchedulerLoggerComponent::testSchedule(unsigned int waits) {
    start_testing = true;
    testing = false;
    testing_waits = (waits > 0) ? waits : 0;
    return(true);
}

void SchedulerLoggerComponent::activateDataLogging() {
  logData();
}

/*** logger state variable ***/

void SchedulerLoggerComponent::assembleStateVariable() {
  char pair[60];
  getSchedulerStateTimeStart(cmd, state->tstart, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getSchedulerStateStatus(cmd, state->status, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}

/*** particle webhook data log ***/

void SchedulerLoggerComponent::logData() {
  // always log data[0] with latest current time
  data[0].setNewestDataTime(millis());
  data[0].saveNewestValue(false);
  ControllerLoggerComponent::logData();
}
