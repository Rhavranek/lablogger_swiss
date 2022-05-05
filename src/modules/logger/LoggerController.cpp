#include "application.h"
#include "LoggerController.h"
#include "LoggerComponent.h"

// EEPROM variables
#define STATE_ADDRESS    0 // EEPROM storage location
#define EEPROM_START    0  // EEPROM storage start
const size_t EEPROM_MAX = EEPROM.length();

/*** debugs ***/

void LoggerController::debugCloud(){
  debug_cloud = true;
}

void LoggerController::debugWebhooks(){
  debug_webhooks = true;
}

void LoggerController::debugState(){
  debug_state = true;
}

void LoggerController::debugData(){
  debug_data = true;
}

void LoggerController::debugDisplay() {
  lcd->debug();
}

void LoggerController::forceReset() {
  reset = true;
}

/*** callbacks ***/

void LoggerController::setNameCallback(void (*cb)()) {
  name_callback = cb;
}

void LoggerController::setCommandCallback(void (*cb)()) {
  command_callback = cb;
}

void LoggerController::setStateUpdateCallback(void (*cb)()) {
  state_update_callback = cb;
}

void LoggerController::setDataUpdateCallback(void (*cb)()) {
  data_update_callback = cb;
}

/*** setup ***/

void LoggerController::addComponent(LoggerComponent* component) {
    component->setEEPROMStart(eeprom_location);
    eeprom_location = eeprom_location + component->getStateSize();
    data_idx = component->setupDataVector(data_idx);
    if (debug_data) {
      for(int i = 0; i < component->data.size(); i++) {
        component->data[i].debug();
      }
    }
    if (eeprom_location >= EEPROM_MAX) {
      Serial.printf("ERROR: component '%s' state would exceed EEPROM size, cannot add component.\n", component->id);
    } else {
      Serial.printf("INFO: adding component '%s' to the controller.\n", component->id);
      components.push_back(component);
    }
}

void LoggerController::init() {
  // define pins
  pinMode(reset_pin, INPUT_PULLDOWN);

  // initialize
  Serial.printlnf("INFO: initializing controller '%s'...", version);
  Serial.printlnf("INFO: available memory: %lu", System.freeMemory());

  // capturing system reset information
  if (System.resetReason() == RESET_REASON_USER) {
      past_reset = System.resetReasonData();
      if (past_reset == RESET_RESTART) {
        Serial.println("INFO: restarting per user request");
      } else if (past_reset == RESET_STATE) {
        Serial.println("INFO: restarting for state reset");
      } else if (past_reset == RESET_WATCHDOG) {
        Serial.println("WARNING: restarting because of watchdog");
      }
  }

  // starting application watchdog
  System.enableFeature(FEATURE_RESET_INFO);
  Serial.println("INFO: starting application watchdog");
  wd = new ApplicationWatchdog(60s, watchdogHandler, 1536);

  // lcd
  lcd->init();
  lcd->printLine(1, version);

  //  check for reset
  if(digitalRead(reset_pin) == HIGH || reset) {
    reset = true;
    Serial.println("INFO: reset request detected");
    lcd->printLineTemp(1, "Resetting...");
  }

  // state and log variables
  strcpy(state_variable, "{}");
  state_variable[2] = 0;
  strcpy(data_variable, "{}");
  data_variable[2] = 0;
  strcpy(state_log, "{}");
  state_log[2] = 0;
  strcpy(data_log, "{}");
  data_log[2] = 0;
  strcpy(debug_variable, "{}");
  debug_variable[2] = 0;

  // register particle functions
  Serial.println("INFO: registering logger cloud variables");
  Particle.subscribe("spark/", &LoggerController::captureName, this, MY_DEVICES);
  Particle.function(CMD_ROOT, &LoggerController::receiveCommand, this);
  Particle.variable(STATE_INFO_VARIABLE, state_variable);
  Particle.variable(DATA_INFO_VARIABLE, data_variable);
  Particle.variable(DEBUG_INFO_VARIABLE, debug_variable);
  if (debug_webhooks) {
    // report logs in variables instead of webhooks
    Particle.variable(STATE_LOG_WEBHOOK, state_log);
    Particle.variable(DATA_LOG_WEBHOOK, data_log);
  }

  // controller state
  loadState(reset);
  loadComponentsState(reset);
  original_save_state = state->save_state;

  // check if we got a device name from state
  if (strlen(state->name) > 0) lcd->printLine(1, state->name);

  // components' init
  initComponents();
  
  // startup time info
  Serial.println(Time.format(Time.now(), "INFO: startup time: %Y-%m-%d %H:%M:%S %Z"));
  Serial.printlnf("INFO: available memory: %lu", System.freeMemory());

}

void LoggerController::initComponents() 
{
  // use local iterator to avoid issues with nested call cycles (ctrl->components->ctrl->components)
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++) 
  {
    (*components_iter)->init();
  }
}

void LoggerController::completeStartup() {
  // update state and data information now that name is available
  updateStateVariable();
  updateDataVariable();
  updateDebugVariable();

  if (state->state_logging) {
    Serial.println("INFO: start-up completed.");
    assembleStartupLog();
    queueStateLog();
  } else {
    Serial.println("INFO: start-up completed (not logged).");
  }

  // complete components' startup
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++) 
  {
    (*components_iter)->completeStartup();
  }
}

bool LoggerController::isStartupComplete() {
  return(startup_complete);
}

/*** loop ***/

void LoggerController::update() {

    // cloud connection
    if (Particle.connected()) {
        if (!cloud_connected) {
            // connection freshly made
            #if (PLATFORM_ID == PLATFORM_PHOTON || PLATFORM_ID == PLATFORM_ARGON)
            // mac address
            WiFi.macAddress(mac_address);
            Serial.printf("INFO: MAC address: %02x:%02x:%02x:%02x:%02x:%02x, IP: ", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
            Serial.println(WiFi.localIP().toString());
            #elif PLATFORM_ID == PLATFORM_BORON
            Serial.print("INFO: Cellular connected, IP: ");
            Serial.println(Cellular.localIP().toString());
            #endif
            Serial.println(Time.format(Time.now(), "INFO: cloud connection established at %H:%M:%S %Z"));
            Serial.printlnf("INFO: available memory: %lu", System.freeMemory());
            cloud_connected = true;
            lcd->printLine(2, ""); // clear "connect wifi" message

            // update display
            updateDisplayStateInformation();
            updateDisplayComponentsStateInformation();
            if (state_update_callback) state_update_callback();
            if (data_update_callback) data_update_callback();

            // name capture
            if (!name_handler_registered){
                name_handler_registered = Particle.publish("spark/device/name", PRIVATE);
                if (name_handler_registered) Serial.println("INFO: name handler registered");
            }
        }
        Particle.process();
    } else if (cloud_connected) {
        // should be connected but isn't --> reconnect
        Serial.println(Time.format(Time.now(), "INFO: lost cloud connection at %H:%M:%S"));
        cloud_connection_started = false;
        cloud_connected = false;
    } else if (!cloud_connection_started) {
        // start cloud connection
        #if (PLATFORM_ID == PLATFORM_PHOTON || PLATFORM_ID == PLATFORM_ARGON)
        Serial.println(Time.format(Time.now(), "INFO: initiate cloud connection via Wifi at %H:%M:%S"));
        lcd->printLine(2, "Connect WiFi...");
        #elif PLATFORM_ID == PLATFORM_BORON
        Serial.println(Time.format(Time.now(), "INFO: initiate cloud connection via cellular at %H:%M:%S"));
        lcd->printLine(2, "Connect Cell...");
        #else
        #error "PLATFORM_ID not supported by this library"
        #endif
        updateDisplayStateInformation(); // not components, preserve connect wifi message
        Particle.connect();
        cloud_connection_started = true;
    }

    // startup complete once name handler succeeds and the time is valid (could be some time after initial particle connect)
    // FIXME: for field applications, need to store name in controller state EEPROM (update if it changed) and use RTC (connect VBAT) for time
    // to start logging data no matter whether connected to the web or not --> complete startup basically first time in the loop!
    if (!startup_complete && Particle.connected() && name_handler_succeeded && Time.isValid()) {
      startup_complete = true;
      completeStartup();
    }

    // time to generate data logs?
    if (startup_complete && isTimeForDataLogAndClear()) {
        logData();
        restartLastDataLog();
        clearData(false);
    }

    // out of memory?
    if (missed_data > 0 && !out_of_memory) {
      Serial.printlnf("INFO: no longer out of memory but missed %d data logs along the way", missed_data);
      assembleMissedDataLog();
      queueStateLog();
      missed_data = 0;
    }
    
    // time to process logs?
    if (startup_complete && Particle.connected() && millis() - last_log_published > publish_interval) {
      if (!state_log_stack.empty()) {
        // process state logs first
        publishStateLog();
      } else if (!data_log_stack.empty()) {
        publishDataLog();
      }
      last_log_published = millis();
    }

    // time for time sync?
    if (startup_complete && Particle.connected() && millis() - last_sync > ONE_DAY_MILLIS) {
      // request time synchronization from the Particle Cloud
      Particle.syncTime();
      last_sync = millis();
    }

    // restart
    if (trigger_reset != RESET_UNDEF) {
      if (millis() - reset_timer_start > reset_delay) {
        System.reset(trigger_reset, RESET_NO_WAIT);
      }
      float countdown = ((float) (reset_delay - (millis() - reset_timer_start))) / 1000;
      snprintf(lcd_buffer, sizeof(lcd_buffer), "%.0fs to restart...", countdown);
      lcd->printLineTemp(1, lcd_buffer);
    }

    // components update
    std::vector<LoggerComponent*>::iterator components_iter = components.begin();
    for(; components_iter != components.end(); components_iter++) {
        (*components_iter)->update();
    }

    // lcd update
    lcd->update();

}

/*** logger name capture ***/

void LoggerController::captureName(const char *topic, const char *data) {
  // store name and also assign it to Logger information
  name_handler_succeeded = true;
  if (strcmp(data, state->name) != 0) {
    strncpy ( state->name, data, DEVICE_NAME_MAX);
    state->name[DEVICE_NAME_MAX] = 0; // make sure it's null terminated
    saveState();
    Serial.printlnf("INFO: logger name changed to '%s'", state->name);
    lcd->printLine(1, state->name);
    if (name_callback) name_callback();
  } else {
    Serial.printlnf("INFO: logger name already saved: '%s'", state->name);
  }
}

/*** state management ***/

void LoggerController::loadState(bool reset)
{
  if (!reset) {
    Serial.printf("INFO: trying to restore state from memory for controller '%s'\n", version);
    restoreState();
  } else {
    Serial.printf("INFO: resetting state for controller '%s' back to default values\n", version);
    saveState();
  }
};

void LoggerController::loadComponentsState(bool reset)
{
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
    (*components_iter)->loadState(reset);
  }
}

void LoggerController::saveState(bool always)
{
  if (state->save_state || always) {
    EEPROM.put(eeprom_start, *state);
    if (debug_state) {
      Serial.printf("DEBUG: controller '%s' state saved in memory (if any updates were necessary)\n", version);
    }
  } else {
    Serial.printf("DEBUG: controller '%s' state NOT saved because state saving is off\n", version);
  }
};

bool LoggerController::restoreState()
{
  LoggerControllerState *saved_state = new LoggerControllerState();
  EEPROM.get(eeprom_start, *saved_state);
  bool recoverable = saved_state->version == state->version;
  if (recoverable)
  {
    EEPROM.get(eeprom_start, *state);
    Serial.printf("INFO: successfully restored controller state from memory (state version %d)\n", state->version);
  }
  else
  {
    Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
    saveState(true);
  }
  return (recoverable);
};

void LoggerController::resetState() {
  state->version = 0; // force reset of state on restart
  saveState(true);
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
    (*components_iter)->resetState();
  }
}

/*** command parsing ***/

int LoggerController::receiveCommand(String command_string) {

  // load, parse and finalize command
  command->load(command_string);
  command->extractVariable();
  Serial.printlnf("COMMAND parsing: %s ...", command->command);
  parseCommand();

  // mark error if type still undefined
  if (!command->isTypeDefined()) command->errorCommand();

  // lcd info & serial
  updateDisplayCommandInformation();
  (command->ret_val != 0) ?
    Serial.printlnf("COMMAND %s (return code %d = %s).", lcd_buffer, command->ret_val, command->msg) :
    Serial.printlnf("COMMAND %s (return code %d).", lcd_buffer, command->ret_val);

  // assemble and publish log
  if (debug_webhooks) {
    Serial.printlnf("DEBUG: webhook debugging is on --> always assemble state log and publish to variable '%s'\n", STATE_LOG_WEBHOOK);
    override_state_log = true;
  }
  if (state->state_logging | override_state_log) {
    assembleStateLog();
    queueStateLog();
  }
  override_state_log = false;

  // state information
  if (command->hasStateChanged()) {
    updateStateVariable();
  }

  // command reporting callback
  if (command_callback) command_callback();

  // return value
  return(command->ret_val);
}

void LoggerController::parseCommand() {

  // decision tree
  if (parseLocked()) {
    // locked is getting parsed
  } else if (parseDebug()) {
    // debug state getting parsed
  } else if (parseTimezone()) {
    // timezone getting parsed
  } else if (parseStateSaving()) {
    // save-state parsed
  } else if (parseSdLogging()) {
    // SD logging getting parsed
  } else if (parseStateLogging()) {
    // state logging getting parsed
  } else if (parseDataLogging()) {
    // data logging getting parsed
  } else if (parseDataLoggingPeriod()) {
    // parsing logging period
  } else if (parseDataReadingPeriod()) {
    // parsing reading period
  } else if (parseReset()) {
    // reset getting parsed
  } else if (parseRestart()) {
    // restart getting parsed
  } else if (parsePage()) {
    // lcd paging
  } else {
    parseComponentsCommand();
  }

}

void LoggerController::parseComponentsCommand() {
  bool success = false;
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
     success = (*components_iter)->parseCommand(command);
     if (success) break;
  }
}

bool LoggerController::parseLocked() {
  // decision tree
  if (command->parseVariable(CMD_LOCK)) {
    // locking
    command->extractValue();
    if (command->parseValue(CMD_LOCK_ON)) {
      command->success(changeLocked(true));
    } else if (command->parseValue(CMD_LOCK_OFF)) {
      command->success(changeLocked(false));
    }
    getStateLockedText(state->locked, command->data, sizeof(command->data));
  } else if (state->locked) {
    // Logger is locked --> no other commands allowed
    command->errorLocked();
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDebug() {
  // debug mode
  if (command->parseVariable(CMD_DEBUG)) {
    command->extractValue();
    if (command->parseValue(CMD_DEBUG_ON)) {
      command->success(changeDebug(true));
    } else if (command->parseValue(CMD_DEBUG_OFF)) {
      command->success(changeDebug(false));
    }
    getStateDebugText(state->debug_mode, command->data, sizeof(command->data));
  } 
  return(command->isTypeDefined());
}

bool LoggerController::parseTimezone() {
  if (command->parseVariable(CMD_TIMEZONE)) {
    // timezone
    // parse read period
    command->extractValue();
    int tz = atoi(command->value);
    if (command->value[0] == '0' || tz != 0) {
      // save timezone
      command->success(changeTimezone(tz));
    } else {
      // invalid value
      command->errorValue();
    }
    getStateTimezoneText(state->tz, command->data, sizeof(command->data), false);
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseStateSaving() {
  if (command->parseVariable(CMD_SAVE_STATE)) {
    // save state (=persistance)
    command->extractValue();
    if (command->parseValue(CMD_SAVE_STATE_ON)) {
      command->success(changeStateSaving(true));
    } else if (command->parseValue(CMD_SAVE_STATE_OFF)) {
      command->success(changeStateSaving(false));
    }
    getStateSaveStateText(state->save_state, command->data, sizeof(command->data), false);
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseSdLogging() {
  if (command->parseVariable(CMD_SD_LOG)) {
    // SD logging
    command->extractValue();
    if (command->parseValue(CMD_SD_LOG_ON)) {
      command->success(changeSdLogging(true));
    } else if (command->parseValue(CMD_SD_LOG_OFF)) {
      command->success(changeSdLogging(false));
    }
    getStateSdLoggingText(state->sd_logging, command->data, sizeof(command->data), false, debug_webhooks);
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseStateLogging() {
  if (command->parseVariable(CMD_STATE_LOG)) {
    // state logging
    command->extractValue();
    if (command->parseValue(CMD_STATE_LOG_ON)) {
      command->success(changeStateLogging(true));
    } else if (command->parseValue(CMD_STATE_LOG_OFF)) {
      command->success(changeStateLogging(false));
    }
    getStateStateLoggingText(state->state_logging, command->data, sizeof(command->data), false, debug_webhooks);
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDataLogging() {
  if (command->parseVariable(CMD_DATA_LOG)) {
    // state logging
    command->extractValue();
    if (command->parseValue(CMD_DATA_LOG_ON)) {
      command->success(changeDataLogging(true));
    } else if (command->parseValue(CMD_DATA_LOG_OFF)) {
      command->success(changeDataLogging(false));
    }
    getStateDataLoggingText(state->data_logging, command->data, sizeof(command->data), false, debug_webhooks);
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseReset() {
  if (command->parseVariable(CMD_RESET)) {
    command->extractValue();
    if (command->parseValue(CMD_RESET_DATA)) {
      restartLastDataLog(); 
      clearData(true); // clear all data
      updateDataVariable(); // update data variable
      command->success(true);
      getStateStringText(CMD_RESET, CMD_RESET_DATA, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
    } else  if (command->parseValue(CMD_RESET_STATE)) {
      resetState();
      command->success(true);
      getStateStringText(CMD_RESET, CMD_RESET_STATE, command->data, sizeof(command->data), PATTERN_KV_JSON_QUOTED, false);
      command->setLogMsg("restarting system...");
      trigger_reset = RESET_STATE;
      reset_timer_start = millis();
    }
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseRestart() {
  if (command->parseVariable(CMD_RESTART)) {
    command->success(true);
    getInfoValue(command->data, sizeof(command->data), CMD_RESTART);
    command->setLogMsg("restarting system...");
    trigger_reset = RESET_RESTART;
    reset_timer_start = millis();
    
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDataLoggingPeriod() {
  if (command->parseVariable(CMD_DATA_LOG_PERIOD)) {
    // parse read period
    command->extractValue();
    int log_period = atoi(command->value);
    if (log_period > 0) {
      command->extractUnits();
      uint8_t log_type = LOG_BY_TIME;
      if (command->parseUnits(CMD_DATA_LOG_PERIOD_NUMBER)) {
        // events
        log_type = LOG_BY_EVENT;
      } else if (command->parseUnits(CMD_DATA_LOG_PERIOD_SEC)) {
        // seconds (the base unit)
        log_period = log_period;
      } else if (command->parseUnits(CMD_DATA_LOG_PERIOD_MIN)) {
        // minutes
        log_period = 60 * log_period;
      } else if (command->parseUnits(CMD_DATA_LOG_PERIOD_HR)) {
        // minutes
        log_period = 60 * 60 * log_period;
      } else {
        // unrecognized units
        command->errorUnits();
      }
      // assign read period
      if (!command->isTypeDefined()) {
        if (log_type == LOG_BY_EVENT || (log_type == LOG_BY_TIME && (log_period * 1000) > state->data_reading_period))
          command->success(changeDataLoggingPeriod(log_period, log_type));
        else
          // make sure smaller than log period
          command->error(CMD_RET_ERR_LOG_SMALLER_READ, CMD_RET_ERR_LOG_SMALLER_READ_TEXT);
      }
    } else {
      // invalid value
      command->errorValue();
    }
    getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type, command->data, sizeof(command->data));
  }
  return(command->isTypeDefined());
}

bool LoggerController::parseDataReadingPeriod() {
  if (command->parseVariable(CMD_DATA_READ_PERIOD)) {

    // parse read period
    command->extractValue();

    if(!state->data_reader) {
      // not actually a data reader
      command->error(CMD_RET_ERR_NOT_A_READER, CMD_RET_ERR_NOT_A_READER_TEXT);
    } else if (command->parseValue(CMD_DATA_READ_PERIOD_MANUAL)){
      // manual reads
      command->success(changeDataReadingPeriod(READ_MANUAL));
    } else {
      // specific read period
      int read_period = atoi(command->value);
      if (read_period > 0) {
        command->extractUnits();
        if (command->parseUnits(CMD_DATA_READ_PERIOD_MS)) {
          // milli seconds (the base unit)
          read_period = read_period;
        } else if (command->parseUnits(CMD_DATA_READ_PERIOD_SEC)) {
          // seconds
          read_period = 1000 * read_period;
        } else if (command->parseUnits(CMD_DATA_READ_PERIOD_MIN)) {
          // minutes
          read_period = 1000 * 60 * read_period;
        } else {
          // unrecognized units
          command->errorUnits();
        }
        // assign read period
        if (!command->isTypeDefined()) {
          if (read_period < state->data_reading_period_min)
            // make sure bigger than minimum
            command->error(CMD_RET_ERR_READ_LARGER_MIN, CMD_RET_ERR_READ_LARGER_MIN_TEXT);
          else if (state->data_logging_type == LOG_BY_TIME && state->data_logging_period * 1000 <= read_period)
            // make sure smaller than log period
            command->error(CMD_RET_ERR_LOG_SMALLER_READ, CMD_RET_ERR_LOG_SMALLER_READ_TEXT);
          else
            command->success(changeDataReadingPeriod(read_period));
        }
      } else {
        // invalid value
        command->errorValue();
      }
    }

    // include current read period in data
    if(state->data_reader) {
      getStateDataReadingPeriodText(state->data_reading_period, command->data, sizeof(command->data));
    }
  }
  return(command->isTypeDefined());
}

bool LoggerController::parsePage() {
  if (command->parseVariable(CMD_PAGE)) {
    if (lcd->getNumberOfPages() > 1) {
      // parse page to jump to
      command->extractValue();
      int page = atoi(command->value);
      bool changed = false;
      if (page > 0) {
        // jumpt to specific page
        if (page == lcd->getCurrentPage()) {
          // already on that page
          command->success(false);
          return(command->isTypeDefined());
        } else {
          changed = lcd->setCurrentPage(page);
        }
      } else {
        // just page by one
        changed = lcd->nextPage();
      }
      if (changed)
        // successfully jumped to requested page
        command->success(true);
      else
        // invalid page requested
        command->error(CMD_RET_ERR_PAGE_INVALID, CMD_RET_ERR_PAGE_INVALID_TEXT);
    } else {
      // doesn't have pages
      command->error(CMD_RET_ERR_NO_PAGES, CMD_RET_ERR_NO_PAGES_TEXT);
    }
  }
  return(command->isTypeDefined());
}

/*** state changes ***/

// locking
bool LoggerController::changeLocked(bool on) {
  bool changed = on != state->locked;

  if (changed) {
    state->locked = on;
  }

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: locking Logger") : Serial.println("DEBUG: unlocking Logger");
    else
      on ? Serial.println("DEBUG: Logger already locked") : Serial.println("DEBUG: Logger already unlocked");
  }

  if (changed) saveState();

  return(changed);
}

// change debug mode
bool LoggerController::changeDebug (bool on) {
  bool changed = on != state->debug_mode;

  if (changed) state->debug_mode = on;

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: debug mode turned on") : Serial.println("DEBUG: debug mode turned off");
    else
      on ? Serial.println("DEBUG: debug mode already on") : Serial.println("DEBUG: debug mode already off");
  }

  if (changed) {
    saveState();
    updateDataVariable();
    updateDebugVariable();
  }

  return(changed);
}

// timezone
bool LoggerController::changeTimezone(int8_t tz) {
  bool changed = tz != state->tz;

  if (changed) state->tz = tz;

  if (debug_state) {
    if (changed) Serial.printlnf("DEBUG: setting timezone to %d", tz);
    else Serial.printlnf("DEBUG: timezone unchanged (%d)", tz);
  }

  if (changed) saveState();

  return(changed);
}

// state saving
bool LoggerController::changeStateSaving (bool on) {
  bool changed = on != state->save_state;

  if (changed) state->save_state = on;

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: save state turned on") : Serial.println("DEBUG: save state turned off");
    else
      on ? Serial.println("DEBUG: save state already on") : Serial.println("DEBUG: save state already off");
  }

  // always save changes in this parameter
  if (changed) saveState(true); 

  return(changed);
}

// pause state saving
void LoggerController::pauseStateSaving() {
  original_save_state = state->save_state;
  if (original_save_state) Serial.println("INFO: pausing state saving");
  state->save_state = false;
}

// resume state saving
void LoggerController::resumeStateSaving() {
  if (original_save_state) Serial.println("INFO: resuming state saving");
  state->save_state = original_save_state;
}

// sd log
bool LoggerController::changeSdLogging (bool on) {
  bool changed = on != state->sd_logging;

  if (changed) state->sd_logging = on;

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: sd logging turned on") : Serial.println("DEBUG: sd logging turned off");
    else
      on ? Serial.println("DEBUG: sd logging already on") : Serial.println("DEBUG: sd logging already off");
  }

  if (changed) saveState();

  return(changed);
}

// state log
bool LoggerController::changeStateLogging (bool on) {
  bool changed = on != state->state_logging;

  if (changed) {
    state->state_logging = on;
    override_state_log = true; // always log this event no matter what
  }

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: state logging turned on") : Serial.println("DEBUG: state logging turned off");
    else
      on ? Serial.println("DEBUG: state logging already on") : Serial.println("DEBUG: state logging already off");
  }

  if (changed) saveState();

  return(changed);
}

// data log
bool LoggerController::changeDataLogging (bool on) {
  bool changed = on != state->data_logging;

  if (changed) {
    state->data_logging = on;
    if (on) {
      std::vector<LoggerComponent*>::iterator components_iter = components.begin();
      for(; components_iter != components.end(); components_iter++)
      {
        (*components_iter)->activateDataLogging();
      }
    }
  }

  if (debug_state) {
    if (changed)
      on ? Serial.println("DEBUG: data logging turned on") : Serial.println("DEBUG: data logging turned off");
    else
      on ? Serial.println("DEBUG: data logging already on") : Serial.println("DEBUG: data logging already off");
  }

  if (changed) saveState();

  // make sure all data is cleared
  if (changed && on) clearData(true);

  return(changed);
}

// logging period
bool LoggerController::changeDataLoggingPeriod(int period, int type) {
  bool changed = period != state->data_logging_period | type != state->data_logging_type;

  if (changed) {
    state->data_logging_period = period;
    state->data_logging_type = type;
  }

  if (debug_state) {
    if (changed) Serial.printf("DEBUG: setting data logging period to %d %s\n", period, type == LOG_BY_TIME ? "seconds" : "reads");
    else Serial.printf("DEBUG: data logging period unchanged (%d)\n", type == LOG_BY_TIME ? "seconds" : "reads");
  }

  if (changed) saveState();

  return(changed);
}

// reading period
bool LoggerController::changeDataReadingPeriod(int period) {

  // safety check (should never get here)
  if (!state->data_reader) {
    Serial.println("ERROR: not a data reader! cannot change reading period.");
    return(false);
  }

  bool changed = period != state->data_reading_period;

  if (changed) {
    state->data_reading_period = period;
  }

  if (debug_state) {
    if (changed) Serial.printf("DEBUG: setting data reading period to %d ms\n", period);
    else Serial.printf("DEBUG: data reading period unchanged (%d ms)\n", period);
  }

  if (changed) saveState();

  return(changed);
}

/*** command info to display ***/

void LoggerController::updateDisplayCommandInformation() {
  assembleDisplayCommandInformation();
  showDisplayCommandInformation();
}

void LoggerController::assembleDisplayCommandInformation() {
  if (command->ret_val == CMD_RET_ERR_LOCKED)
    // make user aware of locked status since this may be a confusing error
    snprintf(lcd_buffer, sizeof(lcd_buffer), "LOCK%s: %s", command->type_short, command->command);
  else
    snprintf(lcd_buffer, sizeof(lcd_buffer), "%s: %s", command->type_short, command->command);
}

void LoggerController::showDisplayCommandInformation() {
  lcd->printLineTemp(1, lcd_buffer);
}

/*** state info to LCD display ***/

void LoggerController::updateDisplayStateInformation() {
  lcd_buffer[0] = 0; // reset buffer
  assembleDisplayStateInformation();
  showDisplayStateInformation();
}

void LoggerController::assembleDisplayStateInformation() {
  uint i = 0;
  if (state->debug_mode) {
    // debug mode
    lcd_buffer[i] = LCD_BUG;
    i++;
  } 
  if (Particle.connected()) {
    // wifi connection
    lcd_buffer[i] = 'W';
  } else {
    lcd_buffer[i] = '!';
  }
  i++;
  if (state->locked) {
    // state lock
    lcd_buffer[i] = 'L';
    i++;
  }
  if (!state->save_state) {
    // state saving is off
    lcd_buffer[i] = 'X';
    i++;
  }
  if (state->state_logging) {
    // state logging
    lcd_buffer[i] = 'S';
    i++;
  }
  if (state->data_logging) {
    // data logging
    lcd_buffer[i] = 'D';
    i++;
    getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type,
        lcd_buffer + i, sizeof(lcd_buffer) - i, true);
    i = strlen(lcd_buffer);
  }

  // data reading period
  if (state->data_reader) {
    lcd_buffer[i] = 'R'; 
    i++;
    if (state->data_reading_period == READ_MANUAL) {
      lcd_buffer[i] = 'M';
      i++;
    } else {
      getStateDataReadingPeriodText(state->data_reading_period, 
        lcd_buffer + i, sizeof(lcd_buffer) - i, true);
      i = strlen(lcd_buffer);
    }
  }

  lcd_buffer[i] = 0;
}

void LoggerController::showDisplayStateInformation() {
  if (strlen(state->name) > 0) lcd->printLine(1, state->name);
  lcd->printLineRight(1, lcd_buffer, strlen(lcd_buffer) + 1);
}

void LoggerController::updateDisplayComponentsStateInformation() {
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
     (*components_iter)->updateDisplayStateInformation();
  }
}

/*** logger state variable ***/

void LoggerController::updateStateVariable() {
  updateDisplayStateInformation();
  updateDisplayComponentsStateInformation();
  if (state_update_callback) state_update_callback();
  state_variable_buffer[0] = 0; // reset buffer
  assembleStateVariable();
  assembleComponentsStateVariable();
  postStateVariable();
}

void LoggerController::assembleStateVariable() {
  char pair[60];
  getStateLockedText(state->locked, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  getStateDebugText(state->debug_mode, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  getStateTimezoneText(state->tz, pair, sizeof(pair), false); addToStateVariableBuffer(pair);
  getStateSaveStateText(state->save_state, pair, sizeof(pair), false); addToStateVariableBuffer(pair);
  getStateSdLoggingText(state->sd_logging, pair, sizeof(pair), false, debug_webhooks); addToStateVariableBuffer(pair);
  getStateStateLoggingText(state->state_logging, pair, sizeof(pair), false, debug_webhooks); addToStateVariableBuffer(pair);
  getStateDataLoggingText(state->data_logging, pair, sizeof(pair), false, debug_webhooks); addToStateVariableBuffer(pair);
  getStateDataLoggingPeriodText(state->data_logging_period, state->data_logging_type, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  if (state->data_reader) {
    getStateDataReadingPeriodText(state->data_reading_period, pair, sizeof(pair)); addToStateVariableBuffer(pair);
  }
}

void LoggerController::assembleComponentsStateVariable() {
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
     (*components_iter)->assembleStateVariable();
  }
}

void LoggerController::addToStateVariableBuffer(char* info) {
  if (state_variable_buffer[0] == 0) {
    strncpy(state_variable_buffer, info, sizeof(state_variable_buffer));
  } else {
    snprintf(state_variable_buffer, sizeof(state_variable_buffer),
        "%s,%s", state_variable_buffer, info);
  }
}

void LoggerController::postStateVariable() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, s = state information
  snprintf(state_variable, sizeof(state_variable), 
    "{\"dt\":\"%s\",\"version\":\"%s\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"mem\":%lu,\"sls\":%d,\"dls\":%d,\"s\":[%s]}",
    date_time_buffer, version, 
    mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5],
    System.freeMemory(), state_log_stack.size(), data_log_stack.size(),
    state_variable_buffer);
  if (debug_cloud) {
    Serial.printf("DEBUG: updated state variable: %s\n", state_variable);
  }
  if (!Particle.connected()) {
    Serial.println("WARNING: particle not (yet) connected, state variable only available when connected.");
  }
}

/*** particle webhook state log ***/

void LoggerController::assembleStartupLog() {
  // include restart details in startup log message
  command->reset();
  strcpy(command->type, CMD_LOG_TYPE_STARTUP);
  strcpy(command->data, "{\"k\":\"startup\",\"v\":\"complete\"}");
  if (past_reset == RESET_RESTART) {
    strcpy(command->msg, "after user-requested restart");
  } else if (past_reset == RESET_STATE) {
    strcpy(command->msg, "for user-requested state reset");
  } else if (past_reset == RESET_WATCHDOG) {
    strcpy(command->msg, "triggered by application watchdog");
  }
  assembleStateLog();
}

void LoggerController::assembleMissedDataLog() {
  command->reset();
  strcpy(command->type, CMD_LOG_TYPE_ERROR);
  char data[40];
  snprintf(data, sizeof(data), "{\"k\":\"missed_data_logs\",\"v\":\"%d\"}", missed_data);
  strcpy(command->data, data);
  strcpy(command->msg, "lack of cloud connection and low memory lead to missing data logs");
  assembleStateLog();
}

void LoggerController::assembleStateLog() {
  state_log[0] = 0;
  if (command->data[0] == 0) strcpy(command->data, "{}"); // empty data entry
  // id = Logger name, dt = log datetime, t = state log type, s = state change, m = message, n = notes
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  int buffer_size = snprintf(state_log, sizeof(state_log),
     "{\"id\":\"%s\",\"dt\":\"%s\",\"t\":\"%s\",\"s\":[%s],\"m\":\"%s\",\"n\":\"%s\"}",
     state->name, date_time_buffer, command->type, command->data, command->msg, command->notes);
  if (buffer_size < 0 || buffer_size >= sizeof(state_log)) {
    Serial.println("ERROR: state log buffer not large enough for state log");
    lcd->printLineTemp(1, "ERR: statelog too big");
    // FIXME: implement better size checks!!, i.e. split up call --> malformatted JSON will crash the webhook
  }
}

void LoggerController::queueStateLog() {
  if (!startup_complete) {
    Serial.printlnf("WARNING: state log '%s' NOT queued because startup is not yet complete.", state_log);
  } else if (debug_webhooks) {
    Serial.printlnf("WARNING: state log '%s' NOT queued because in WEBHOOKS_DEBUG_ON mode.", state_log);
  } else {
    state_log_stack.push_back(state_log);
    if (debug_cloud) {
      Serial.printlnf("DEBUG: added log #%d to state log stack: '%s'", state_log_stack.size(), state_log_stack.back().c_str());
    }
  }
  postStateVariable(); // update state variable stack info
}

void LoggerController::publishStateLog() {
  
  if (!state_log_stack.empty()) {

    // process from back to front (i.e. always latest log first) for speed and to avoid memory fragmentation
    if (debug_cloud) {
      Serial.printf("DEBUG: publishing last state log (#%d) to event '%s': '%s'... ", 
        state_log_stack.size(), STATE_LOG_WEBHOOK, state_log_stack.back().c_str());
    }
    
    bool success = Particle.publish(STATE_LOG_WEBHOOK, state_log_stack.back().c_str(), WITH_ACK);
    if (debug_cloud) {
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    }

    if (success) {
      state_log_stack.pop_back();
      postStateVariable(); // update state variable stack info
    }

  }
  
}

/*** logger data variable ***/

void LoggerController::updateDataVariable() {
  if (data_update_callback) data_update_callback();
  data_variable_buffer[0] = 0; // reset buffer
  assembleComponentsDataVariable();
  postDataVariable();
}

void LoggerController::assembleComponentsDataVariable() {
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
     (*components_iter)->assembleDataVariable();
  }
}

void LoggerController::addToDataVariableBuffer(char* info) {
  if (data_variable_buffer[0] == 0) {
    strncpy(data_variable_buffer, info, sizeof(data_variable_buffer));
  } else {
    snprintf(data_variable_buffer, sizeof(data_variable_buffer),
        "%s,%s", data_variable_buffer, info);
  }
}

void LoggerController::postDataVariable() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, d = structured data
  snprintf(data_variable, sizeof(data_variable), "{\"dt\":\"%s\",\"d\":[%s]}",
    date_time_buffer, data_variable_buffer);
  if (debug_cloud) {
    Serial.printf("DEBUG: updated data variable: %s\n", data_variable);
  }
  if (!Particle.connected()) {
    Serial.println("WARNING: particle not (yet) connected, data variable only available when connected.");
  }
}

/*** particle webhook data log ***/

bool LoggerController::isTimeForDataLogAndClear() {

  if (state->data_logging_type == LOG_BY_TIME) {
    // go by time
    unsigned long log_period = state->data_logging_period * 1000;
    if ((millis() - last_data_log) > log_period) {
      if (debug_data) {
        Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
        Serial.printf("DEBUG: triggering data log at %s (after %d seconds)\n", date_time_buffer, state->data_logging_period);
      }
      return(true);
    }
  } else if (state->data_logging_type == LOG_BY_EVENT) {
    /*
    // not implemented! this needs to be handled by each component individually so requires a mechanism for components to trigger a specific partial data log
    // go by read number
    if (data[0].getN() >= state->data_logging_period) {
      if (debug_data) {
      Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
      Serial.printf("INFO: triggering data log at %s (after %d reads)\n", date_time_buffer, state->data_logging_period);
      }
      return(true);
    }
    */
    Serial.println("ERROR: LOG_BY_EVENT data logging type is not yet implemented!");
  } else {
    Serial.printf("ERROR: unknown logging type stored in state - this should be impossible! %d\n", state->data_logging_type);
  }
  return(false);
}

void LoggerController::restartLastDataLog() {
  last_data_log = millis();
}

void LoggerController::clearData(bool clear_persistent) {
  // clear data for components
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++) {
     (*components_iter)->clearData(clear_persistent);
  }
}

void LoggerController::logData() {
  // publish data log
  bool override_data_log = false;
  if (debug_webhooks) {
    Serial.printf("DEBUG: webhook debugging is on --> always assemble data log and publish to variable '%s'\n", DATA_LOG_WEBHOOK);
    override_data_log = true;
  }
  if (state->data_logging | override_data_log) {
      // log data for components
      std::vector<LoggerComponent*>::iterator components_iter = components.begin();
      for(; components_iter != components.end(); components_iter++) {
        (*components_iter)->logData();
      }
  } else {
    if (debug_cloud) {
      Serial.println("DEBUG: data log is turned off --> continue without logging");
    }
  }
}

void LoggerController::resetDataLog() {
  data_log[0] = 0;
  data_log_buffer[0] = 0;
}

bool LoggerController::addToDataLogBuffer(char* info) {

  // debug
  if (debug_data) Serial.printf("DEBUG: trying to add '%s' to data log... ", info);

  // characters reserved for rest of data log
  const uid_t reserve = 50;
  if (strlen(data_log_buffer) + strlen(info) + reserve >= sizeof(data_log)) {
    // not enough space in the data log to add more to the buffer
    if (debug_data) Serial.println("but log is at the size limit.");
    return(false);
  }

  // still enough space
  if (data_log_buffer[0] == 0) {
    // buffer empty, start from scratch
    if (debug_data) Serial.println("success (first data).");
    strncpy(data_log_buffer, info, sizeof(data_log_buffer));
  } else {
    // concatenate existing buffer with new info
    if (debug_data) Serial.println("success.");
    snprintf(data_log_buffer, sizeof(data_log_buffer),
        "%s,%s", data_log_buffer, info);
  }
  return(true);
}

bool LoggerController::finalizeDataLog(bool use_common_time, unsigned long common_time) {
  // data
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  int buffer_size;
  if (use_common_time) {
    // id = Logger name, dt = log datetime, to = time offset from log datetime (global), d = structured data
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"id\":\"%s\",\"dt\":\"%s\",\"to\":%lu,\"d\":[%s]}", 
      state->name, date_time_buffer, common_time, data_log_buffer);
  } else {
    // indivudal time
    buffer_size = snprintf(data_log, sizeof(data_log), "{\"id\":\"%s\",\"dt\":\"%s\",\"d\":[%s]}", 
      state->name, date_time_buffer, data_log_buffer);
  }
  if (buffer_size < 0 || buffer_size >= sizeof(data_log)) {
    Serial.println("ERROR: data log buffer not large enough for data log - this should NOT be possible to happen");
    lcd->printLineTemp(1, "ERR: datalog too big");
    return(false);
  }
  return(true);
}

void LoggerController::queueDataLog() {
  if (!state->data_logging) {
    Serial.println("WARNING: no data log queued because data_logging is OFF.");
  } else if (strlen(data_log) == 0) {
    Serial.println("WARNING: no data log queued because there is none.");
  } else if (!startup_complete) {
    Serial.printlnf("WARNING: data log '%s' NOT queued because startup is not yet complete.", data_log);
  } else if (debug_webhooks) {
    Serial.printlnf("WARNING: data log '%s' NOT queued because in WEBHOOKS_DEBUG_ON mode.", data_log);
  } else if (System.freeMemory() < memory_reserve) {
    out_of_memory = true;
    missed_data++;
    Serial.printlnf("WARNING: data log '%s' NOT queued because free memory < memory reserve (%d bytes), total %d data logs missed.", 
      data_log, memory_reserve, missed_data);
  } else {
    out_of_memory = false;
    data_log_stack.push_back(data_log);
    if (debug_cloud) {
      Serial.printlnf("DEBUG: added log #%d to data log stack: '%s'", data_log_stack.size(), data_log_stack.back().c_str());
    }
  }
  postStateVariable(); // update state variable stack info
}

void LoggerController::publishDataLog() {
  
  if (!data_log_stack.empty()) {

    size_t log_n = data_log_stack.size();

    // process from back to front (i.e. always latest log first) for speed and to avoid memory fragmentation
    if (debug_cloud) {
      Serial.printf("DEBUG: publishing last data log (#%d) to event '%s': '%s'... ", 
        log_n, DATA_LOG_WEBHOOK, data_log_stack.back().c_str());
    }

    // particle is connected, try to publish the latest log
    bool success = Particle.publish(DATA_LOG_WEBHOOK, data_log_stack.back().c_str(), WITH_ACK);
    
    if (debug_cloud) {
      if (success) Serial.println("successful.");
      else Serial.println("failed!");
    }

    if (success) {
      (log_n > 1) ?
        snprintf(lcd_buffer, sizeof(lcd_buffer), "INFO: data log %d sent", log_n) :
        snprintf(lcd_buffer, sizeof(lcd_buffer), "INFO: data log sent");
      lcd->printLineTemp(1, lcd_buffer);
      data_log_stack.pop_back();
      postStateVariable(); // update state variable stack info
    } else {
      snprintf(lcd_buffer, sizeof(lcd_buffer), "ERR: data log %d error", log_n);
      lcd->printLineTemp(1, lcd_buffer);
    }

  }
  
}

/*** logger debug variable ***/

void LoggerController::updateDebugVariable() {
  debug_variable_buffer[0] = 0; // reset buffer
  assembleComponentsDebugVariable();
  postDebugVariable();
}

void LoggerController::assembleComponentsDebugVariable() {
  std::vector<LoggerComponent*>::iterator components_iter = components.begin();
  for(; components_iter != components.end(); components_iter++)
  {
    if (debug_variable_buffer[0] == 0) {
      snprintf(debug_variable_buffer, sizeof(debug_variable_buffer), "\"id\":\"%s\"", (*components_iter)->id);
    } else {
      snprintf(debug_variable_buffer, sizeof(debug_variable_buffer), "%s},{\"id\":\"%s\"", debug_variable_buffer, (*components_iter)->id);
    }
    (*components_iter)->assembleDebugVariable();
  }
}

void LoggerController::addToDebugVariableBuffer(char* var, char* info) { 
  snprintf(debug_variable_buffer, sizeof(debug_variable_buffer), "%s,\"%s\":\"%s\"", debug_variable_buffer, var, info);
}

void LoggerController::postDebugVariable() {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S %Z").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  // dt = datetime, d = structured debug info
  snprintf(debug_variable, sizeof(debug_variable), "{\"dt\":\"%s\",\"cs\":[{%s}]}",
    date_time_buffer, debug_variable_buffer);
  if (debug_cloud) {
    Serial.printf("DEBUG: updated debug variable: %s\n", debug_variable);
  }
  if (!Particle.connected()) {
    Serial.println("WARNING: particle not (yet) connected, debug variable only available when connected.");
  }
}
