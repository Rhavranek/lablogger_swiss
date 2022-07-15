#pragma once
#include <vector>
#include "LoggerUtils.h"
#include "LoggerCommand.h"
#include "LoggerDisplay.h"
#include "LoggerSD.h"

/*** time sync ***/
#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)

/*** spark cloud constants ***/
#define CMD_ROOT               "device" // command root (i.e. registered particle call function)
#define STATE_INFO_VARIABLE    "state" // name of the particle exposed state variable
#define STATE_INFO_MAX_CHAR    621 // how long is the state information maximally
#define STATE_LOG_WEBHOOK      "state_log"  // name of the webhook to Logger state log
#define STATE_LOG_MAX_CHAR     621  // spark.publish is limited to 622 bytes of device OS 0.8.0 (previously just 255)
#define DATA_INFO_VARIABLE     "data" // name of the particle exposed data variable
#define DATA_INFO_MAX_CHAR     621 // how long is the data information maximally
#define DATA_LOG_WEBHOOK       "data_log"  // name of the webhook to Logger data log
#define DATA_LOG_MAX_CHAR      621  // spark.publish is limited to 622 bytes of device OS 0.8.0 (previously just 255)
#define DEBUG_INFO_VARIABLE    "debug" // name of the particle exposed debug variable
#define DEBUG_INFO_MAX_CHAR    621 // how long is the debug information maximally

/*** commands ***/
// return codes:
//  -  0 : success without warning
//  - >0 : success with warnings
//  - <0 : failed with errors
#define CMD_RET_UNDEFINED                   -100 // undefined behavior
#define CMD_RET_SUCCESS                     0 // succes = 0
#define CMD_RET_ERR                         -1 // errors < 0
#define CMD_RET_ERR_TEXT                    "undefined error"
#define CMD_RET_ERR_LOCKED                  -2 // error locked
#define CMD_RET_ERR_LOCKED_TEXT             "locked"
#define CMD_RET_ERR_CMD                     -3 // invalid command
#define CMD_RET_ERR_CMD_TEXT                "invalid command"
#define CMD_RET_ERR_VAL                     -4 // invalid value
#define CMD_RET_ERR_VAL_TEXT                "invalid value"
#define CMD_RET_ERR_UNITS                   -5 // invalid units
#define CMD_RET_ERR_UNITS_TEXT              "invalid units"
#define CMD_RET_ERR_NOT_A_READER            -10 // not a data reader
#define CMD_RET_ERR_NOT_A_READER_TEXT       "logger is not a data reader"
#define CMD_RET_ERR_LOG_SMALLER_READ        -11 // log period cannot be smaller than read period!
#define CMD_RET_ERR_LOG_SMALLER_READ_TEXT   "log period must be larger than read period"
#define CMD_RET_ERR_READ_LARGER_MIN         -12 // read period cannot be smaller than its minimum
#define CMD_RET_ERR_READ_LARGER_MIN_TEXT    "read period must be larger than minimum"
#define CMD_RET_ERR_NO_PAGES                -13 // there are no display pages
#define CMD_RET_ERR_NO_PAGES_TEXT           "the display only has one page"
#define CMD_RET_ERR_PAGE_INVALID            -14 // display paging number is invalid
#define CMD_RET_ERR_PAGE_INVALID_TEXT       "invalid display page requested"
#define CMD_RET_ERR_SD_DISABLED             -15 // SD is not enabled
#define CMD_RET_ERR_SD_DISABLED_TEXT        "SD is not enabled in the controller"
#define CMD_RET_ERR_SD_UNAVAILABLE          -16 // SD card not available
#define CMD_RET_ERR_SD_UNAVAILABLE_TEXT     "SD card is not available"
#define CMD_RET_ERR_SD_TEST_FAILED          -17 // SD card test failed
#define CMD_RET_ERR_SD_TEST_FAILED_TEXT     "SD card test failed"
#define CMD_RET_WARN_NO_CHANGE              1 // state unchaged because it was already the same
#define CMD_RET_WARN_NO_CHANGE_TEXT         "state already as requested"

// command log types
#define CMD_LOG_TYPE_UNDEFINED              "undefined"
#define CMD_LOG_TYPE_UNDEFINED_SHORT        "UDEF"
#define CMD_LOG_TYPE_ERROR                  "error"
#define CMD_LOG_TYPE_ERROR_SHORT            "ERR"
#define CMD_LOG_TYPE_STATE_CHANGED          "state changed"
#define CMD_LOG_TYPE_STATE_CHANGED_SHORT    "OK"
#define CMD_LOG_TYPE_STATE_UNCHANGED        "state unchanged"
#define CMD_LOG_TYPE_STATE_UNCHANGED_SHORT  "SAME"
#define CMD_LOG_TYPE_STARTUP                "startup"

// locking
#define CMD_LOCK            "lock" // device "lock on/off [notes]" : locks/unlocks the Logger
  #define CMD_LOCK_ON         "on"
  #define CMD_LOCK_OFF        "off"

// debug
#define CMD_DEBUG           "debug" // device "debug on/off [notes]" : turns debug mode on / off
  #define CMD_DEBUG_ON        "on"
  #define CMD_DEBUG_OFF       "off"

// persistance
#define CMD_SAVE_STATE      "save-state" // device "save-state on/off [notes]" : turns state saving (=persistance) on/off
  #define CMD_SAVE_STATE_ON   "on"
  #define CMD_SAVE_STATE_OFF  "off"

// logging
#define CMD_STATE_LOG       "state-log" // device "state-log on/off [notes]" : turns state logging on/off
  #define CMD_STATE_LOG_ON     "on"
  #define CMD_STATE_LOG_OFF    "off"
#define CMD_DATA_LOG       "data-log" // device "data-log on/off [notes]" : turns data logging on/off
  #define CMD_DATA_LOG_ON     "on"
  #define CMD_DATA_LOG_OFF    "off"
#define CMD_SD_LOG         "sd-log" // device "sd-log on/off [notes]" : turns logging to sd card on/off (only relevant if state-log and/or data-log is on)
  #define CMD_SD_LOG_ON       "on"
  #define CMD_SD_LOG_OFF      "off"
#define CMD_SD_TEST        "sd-test" // device "sd-test"

// timezone
#define CMD_TIMEZONE        "tz" // device "tz number [notes]" : sets a timezone for internal day/time display (logs to are always in UTC no matter the tz setting)

// time units
#define CMD_TIME_MS     "ms" // milli seconds
#define CMD_TIME_SEC    "s"  // seconds
#define CMD_TIME_MIN    "m"  // minutes
#define CMD_TIME_HR     "h"  // hours
#define CMD_TIME_DAY    "d"  // days

// logging rate
#define CMD_DATA_LOG_PERIOD            "log-period" // device log-period number unit [notes] : timing between each data logging (if log is on)
  #define CMD_DATA_LOG_PERIOD_NUMBER   "x"          // device log-period 5x : every 5 data recordings
  #define CMD_DATA_LOG_PERIOD_SEC      CMD_TIME_SEC // device log-period 20s : every 20 seconds
  #define CMD_DATA_LOG_PERIOD_MIN      CMD_TIME_MIN // device log-period 3m : every 3 minutes
  #define CMD_DATA_LOG_PERIOD_HR       CMD_TIME_HR  // device log-period 1h : every hour

// reading rate
#define CMD_DATA_READ_PERIOD          "read-period" // device read-period number unit [notes] : timing between each data read, may not be smaller than device defined minimum and may not be smaller than log period (if a time)
  #define CMD_DATA_READ_PERIOD_MS     CMD_TIME_MS   // device read-period 200ms : read every 200 milli seconds
  #define CMD_DATA_READ_PERIOD_SEC    CMD_TIME_SEC  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MIN    CMD_TIME_MIN  // device read-period 5s : read every 5 seconds
  #define CMD_DATA_READ_PERIOD_MANUAL "manual"      // read only upon manual trigger from the device (may not be available on all devices), typically most useful with 'log-period 1x'

// reset
#define CMD_RESET      "reset" 
  #define CMD_RESET_DATA  "data" // device "reset data" : reset the data stored in the device
  #define CMD_RESET_STATE "state" // device "reset state" : resets state (causes a device restart)
  #define CDM_RESET_SD    "sd"    // device "reset sd" : resets sd card if connected

// restart
#define CMD_RESTART    "restart" // device "restart" : restarts the device

// paging
#define CMD_PAGE       "page" // device "page [#]" : switch to the next page (or a specific page number if provided)


/*** reset codes ***/
#define RESET_UNDEF    1
#define RESET_RESTART  2
#define RESET_STATE    3
#define RESET_WATCHDOG 4

/*** state ***/

// log types
#define LOG_BY_TIME    0 // log period is a time in seconds
#define LOG_BY_EVENT   1 // log period is a number (x times)

// mnaual read constant
#define READ_MANUAL    0 // data reading is manual

// state constants
#define DEVICE_NAME_MAX 20 // ma characters for device name
struct LoggerControllerState {
  bool locked = false; // whether state is locked
  int8_t tz = 0; // timezone offset (for display and calcluations, data logging always in UTC)
  bool save_state = true; // whether state is saved in EEPROM to enable peristance
  bool sd_logging = false; // whether anything is logged to an external flash drive
  bool state_logging = false; // whether state is logged (whenever there is a change)
  bool data_logging = false; // whether data is logged
  uint data_logging_period ; // period between logs (in seconds!)
  uint8_t data_logging_type; // what the data logging period signifies
  bool data_reader = false; // whether this controller is a data reader
  uint data_reading_period_min; // minimum time between reads (in ms) [only relevant if it is a data_reader]
  uint data_reading_period; // period between reads (stored in ms!!!) [only relevant if it is a data_reader]
  bool debug_mode = false; // whether controller is in debug mode for user purposes
  char name[DEVICE_NAME_MAX + 1];
  uint8_t version = 5;

  LoggerControllerState() {};

  // with data reading settings
  LoggerControllerState(bool locked, int8_t tz, bool sd_logging, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, uint data_reading_period_min, uint data_reading_period, bool debug_mode) :
    locked(locked), tz(tz), sd_logging(sd_logging), state_logging(state_logging), data_logging(data_logging), data_logging_period(data_logging_period), data_logging_type(data_logging_type), data_reader(true), data_reading_period_min(data_reading_period_min), data_reading_period(data_reading_period), debug_mode(debug_mode) {
      name[0] = 0;
    }
  LoggerControllerState(bool locked, int8_t tz, bool sd_logging, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, uint data_reading_period_min, uint data_reading_period) :
    LoggerControllerState(locked, tz, sd_logging, state_logging, data_logging, data_logging_period, data_logging_type, data_reading_period_min, data_reading_period, false) {}
  
  // without data reading settings
  LoggerControllerState(bool locked, int8_t tz, bool sd_logging, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type, bool debug_mode) :
    locked(locked), tz(tz), sd_logging(sd_logging), state_logging(state_logging), data_logging(data_logging), data_logging_period(data_logging_period), data_logging_type(data_logging_type), data_reader(false), debug_mode(debug_mode)  {
      name[0] = 0;
    }
  LoggerControllerState(bool locked, int8_t tz, bool sd_logging, bool state_logging, bool data_logging, uint data_logging_period, uint8_t data_logging_type) :
    LoggerControllerState(locked, tz, sd_logging, state_logging, data_logging, data_logging_period, data_logging_type, tz, false) {}

};

/*** state variable formatting ***/

// locked text
static void getStateLockedText(bool locked, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_LOCK, locked, CMD_LOCK_ON, CMD_LOCK_OFF, target, size, pattern, include_key);
}
static void getStateLockedText(bool locked, char* target, int size, int value_only = false) {
  if (value_only) getStateLockedText(locked, target, size, PATTERN_V_SIMPLE, false);
  else getStateLockedText(locked, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// debug mode text
static void getStateDebugText(bool debug_mode, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_DEBUG, debug_mode, CMD_DEBUG_ON, CMD_DEBUG_OFF, target, size, pattern, include_key);
}
static void getStateDebugText(bool debug_mode, char* target, int size, int value_only = false) {
  if (value_only) getStateDebugText(debug_mode, target, size, PATTERN_V_SIMPLE, false);
  else getStateDebugText(debug_mode, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// timezine
static void getStateTimezoneText(int8_t tz, char* target, int size, char* pattern, int include_key = true) {
  getStateIntText(CMD_TIMEZONE, tz, "", target, size, pattern, include_key);
}

// timezone
static void getStateTimezoneText(int8_t tz, char* target, int size, int value_only = false) {
  if (value_only) getStateTimezoneText(tz, target, size, PATTERN_V_SIMPLE, false);
  else getStateTimezoneText(tz, target, size, PATTERN_KV_JSON, true);
}

// save state
static void getStateSaveStateText(bool save_state, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_SAVE_STATE, save_state, CMD_SD_LOG_ON, CMD_SD_LOG_OFF, target, size, pattern, include_key);
}

static void getStateSaveStateText(bool save_state, char* target, int size, int value_only = false) {
  if (value_only) getStateSaveStateText(save_state, target, size, PATTERN_V_SIMPLE, false);
  else getStateSaveStateText(save_state, target, size, PATTERN_KV_JSON_QUOTED, true);
}


// sd logging
static void getStateSdLoggingText(bool sd_logging, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_SD_LOG, sd_logging, CMD_SD_LOG_ON, CMD_SD_LOG_OFF, target, size, pattern, include_key);
}

static void getStateSdLoggingText(bool sd_logging, char* target, int size, int value_only = false) {
  if (value_only) getStateSdLoggingText(sd_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateSdLoggingText(sd_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// state logging
static void getStateStateLoggingText(bool state_logging, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_STATE_LOG, state_logging, CMD_STATE_LOG_ON, CMD_STATE_LOG_OFF, target, size, pattern, include_key);
}

static void getStateStateLoggingText(bool state_logging, char* target, int size, int value_only = false) {
  if (value_only) getStateStateLoggingText(state_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateStateLoggingText(state_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// data logging
static void getStateDataLoggingText(bool data_logging, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_DATA_LOG, data_logging, CMD_DATA_LOG_ON, CMD_DATA_LOG_OFF, target, size, pattern, include_key);
}

static void getStateDataLoggingText(bool data_logging, char* target, int size, int value_only = false) {
  if (value_only) getStateDataLoggingText(data_logging, target, size, PATTERN_V_SIMPLE, false);
  else getStateDataLoggingText(data_logging, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// data logging period (any pattern)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, char* pattern, int include_key = true) {
  // specific logging period
  char units[] = "?";
  if (logging_type == LOG_BY_EVENT) {
    // by read
    strcpy(units, CMD_DATA_LOG_PERIOD_NUMBER);
  } else  if (logging_type == LOG_BY_TIME) {
    // by time
    if (logging_period >= 86400 && logging_period % 86400 < 50) {
      // hours
      strcpy(units, "d");
      logging_period = round(logging_period/86400.);
    } else if (logging_period >= 3600 && logging_period % 3600 < 5) {
      // hours
      strcpy(units, "h");
      logging_period = round(logging_period/3600.);
    } else if (logging_period >= 60 && logging_period % 60 < 2) {
      // minutes
      strcpy(units, "m");
      logging_period = round(logging_period/60.);

    } else {
      strcpy(units, "s");
    }
  } else {
    // not supported!
    Serial.printf("ERROR: unknown logging type %d\n", logging_type);
  }

  getStateIntText(CMD_DATA_LOG_PERIOD, logging_period, units, target, size, pattern, include_key);
}

// logging period (standard patterns)
static void getStateDataLoggingPeriodText(int logging_period, uint8_t logging_type, char* target, int size, int value_only = false) {
  if (value_only) {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_VU_SIMPLE, false);
  } else {
    getStateDataLoggingPeriodText(logging_period, logging_type, target, size, PATTERN_KVU_JSON, true);
  }
}

// logging_period (any pattern)
static void getStateDataReadingPeriodText(int reading_period, char* target, int size, char* pattern, int include_key = true) {
  if (reading_period == 0) {
    // manual mode
    getStateStringText(CMD_DATA_READ_PERIOD, CMD_DATA_READ_PERIOD_MANUAL, target, size, pattern, include_key);
  } else {
    // specific reading period
    char units[] = "ms";
    if (reading_period >= 86400000 && reading_period % 86400000 < 50000) {
      // days
      strcpy(units, "d");
      reading_period = round(reading_period/86400000.);
    } else if (reading_period >= 3600000 && reading_period % 3600000 < 5000) {
      // hours
      strcpy(units, "h");
      reading_period = round(reading_period/3600000.);
    } else if (reading_period >= 60000 && reading_period % 60000 < 500) {
      // minutes
      strcpy(units, "m");
      reading_period = round(reading_period/60000.);
    } else if (reading_period >= 1000 && reading_period % 1000 < 5) {
      // seconds
      strcpy(units, "s");
      reading_period = round(reading_period/1000.);
    }
    getStateIntText(CMD_DATA_READ_PERIOD, reading_period, units, target, size, pattern, include_key);
  }
}

// read period (standard patterns)
static void getStateDataReadingPeriodText(int reading_period, char* target, int size, int value_only = false) {
  if (value_only) {
    (reading_period == 0) ?
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_V_SIMPLE, false) : // manual
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_VU_SIMPLE, false); // number
  } else {
    (reading_period == 0) ?
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_KV_JSON_QUOTED, true) : // manual
      getStateDataReadingPeriodText(reading_period, target, size, PATTERN_KVU_JSON, true); // number
  }
}

/*** watchdog ***/

static void watchdogHandler() {
  System.reset(RESET_WATCHDOG, RESET_NO_WAIT);
}

/*** class definition ***/

// forward declaration for component
class LoggerComponent;

// controller class
class LoggerController {

  private:

    // system reset & application watchdog
    const int reset_delay = 5000; // in ms - how long to delay the reset
    unsigned long reset_timer_start = 0; // start of the reset timer
    uint32_t trigger_reset = RESET_UNDEF; // what kind of reset to trigger
    uint32_t past_reset = RESET_UNDEF; // what kind of reset was triggered
    ApplicationWatchdog *wd;

    // reset
    const int reset_pin;
    bool reset = false;
    

    // sd card
    bool sd_enabled = false;

    // time sync
    unsigned long last_sync = 0;

    // state log exceptions
    bool override_state_log = false;

    // logger info
    bool name_handler_registered = false;
    bool name_handler_succeeded = false;

    // cloud connection
    bool cloud_connection_started = false;
    bool cloud_connected = false;

    // mac address
    byte mac_address[6];

    // state info
    const size_t eeprom_start = 0;
    size_t eeprom_location = 0;

    // startup
    bool startup_complete = false;

    // data indices
    uint8_t data_idx = 0;

    // state saving pause
    bool original_save_state = false;

  protected:

    // lcd buffer (for cross-method msg assembly that might not be safe to do with lcd->buffer)
    char lcd_buffer[21];

    // call backs
    void (*name_callback)() = 0;
    void (*command_callback)() = 0;
    void (*state_update_callback)() = 0;
    void (*data_update_callback)() = 0;

    // buffer for date time
    char date_time_buffer[25];

    // buffer and information variables
    char state_variable[STATE_INFO_MAX_CHAR];
    char state_variable_buffer[STATE_INFO_MAX_CHAR-50];
    char data_variable[DATA_INFO_MAX_CHAR];
    char data_variable_buffer[DATA_INFO_MAX_CHAR-50];
    char debug_variable[DEBUG_INFO_MAX_CHAR];
    char debug_variable_buffer[DEBUG_INFO_MAX_CHAR-50];

    // buffers for log events
    char state_log[STATE_LOG_MAX_CHAR];
    char data_log[DATA_LOG_MAX_CHAR];
    char data_log_buffer[DATA_LOG_MAX_CHAR-10];

    // data logging tracker
    unsigned long last_data_log = 0;

    // log stacks
    std::vector<std::string> state_log_stack;
    std::vector<std::string> data_log_stack;

    // log stack processing
    unsigned long last_log_published = 0;
    const int publish_interval = 1000; // 1/s is the max frequency for particle cloud publishing

    // memory reserve
    uint memory_reserve = 6000; // memory reserve in bytes (should be enough for wifi which takes 3619 normally)
    bool out_of_memory = false; // whether out of memory
    uint missed_data = 0; // how many data points missed b/c no internet and out of memory

  public:

    // debug flags
    bool debug_cloud = false;
    bool debug_state = false;
    bool debug_data = false;

    // controller version
    const char *version;

    // public variables
    LoggerDisplay* lcd;
    LoggerSD* sd = new LoggerSD();
    LoggerControllerState* state;
    LoggerCommand* command = new LoggerCommand();
    std::vector<LoggerComponent*> components;

    // global tracker of sequential data reader
    bool sequential_data_read_in_progress = false;
    unsigned long sequential_data_idle_start = 0;

    /*** constructors ***/
    LoggerController (const char *version, int reset_pin) : LoggerController(version, reset_pin, new LoggerDisplay()) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd) : LoggerController(version, reset_pin, lcd, new LoggerControllerState(), false) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd, bool enable_sd) : LoggerController(version, reset_pin, lcd, new LoggerControllerState(), enable_sd) {}
    LoggerController (const char *version, int reset_pin, LoggerControllerState *state) : LoggerController(version, reset_pin, new LoggerDisplay(), state, false) {}
    LoggerController (const char *version, int reset_pin, LoggerControllerState *state, bool enable_sd) : LoggerController(version, reset_pin, new LoggerDisplay(), state, enable_sd) {}
    LoggerController (const char *version, int reset_pin, LoggerDisplay* lcd, LoggerControllerState *state, bool enable_sd) : version(version), reset_pin(reset_pin), lcd(lcd), state(state), sd_enabled(enable_sd) {
      eeprom_location = eeprom_start + sizeof(*state);
    }

    /*** debugs ***/
    void debugCloud();
    void debugWebhooks();
    void debugState();
    void debugData();
    void debugDisplay();
    void forceReset();

    /*** callbacks ***/
    void setNameCallback(void (*cb)()); // callback executed after name retrieved from cloud
    void setCommandCallback(void (*cb)()); // callback executed after a command is received and processed
    void setStateUpdateCallback(void (*cb)()); // callback executed when state variable is updated
    void setDataUpdateCallback(void (*cb)()); // callback executed when data variable is updated

    /*** setup ***/
    void addComponent(LoggerComponent* component);
    void init(); 
    virtual void initComponents();
    virtual void completeStartup();
    bool isStartupComplete();

    /*** loop ***/
    void update();

    /*** logger name capture ***/
    void captureName(const char *topic, const char *data);

    /*** state management ***/
    virtual size_t getStateSize() { return(sizeof(*state)); }
    virtual void loadState(bool reset);
    virtual void loadComponentsState(bool reset);
    virtual void saveState(bool always = false);
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    int receiveCommand (String command); // receive cloud command
    virtual void parseCommand (); // parse a cloud command
    virtual void parseComponentsCommand(); // parse a cloud command in the components    
    bool parseLocked();
    bool parseDebug();
    bool parseTimezone();
    bool parseStateSaving();
    bool parseSdLogging();
    bool parseStateLogging();
    bool parseDataLogging();
    bool parseDataLoggingPeriod();
    bool parseDataReadingPeriod();
    bool parseReset();
    bool parseRestart();
    bool parsePage();
    bool parseSdTest();

    /*** state changes ***/
    bool changeLocked(bool on);
    bool changeDebug(bool on);
    bool changeTimezone(int8_t tz);
    bool changeStateSaving(bool on);
    void pauseStateSaving();
    void resumeStateSaving();
    bool changeSdLogging(bool on);
    bool changeStateLogging(bool on);
    bool changeDataLogging(bool on);
    bool changeDataLoggingPeriod(int period, int type);
    bool changeDataReadingPeriod(int period);

    /*** command info to display ***/
    virtual void updateDisplayCommandInformation();
    virtual void assembleDisplayCommandInformation();
    virtual void showDisplayCommandInformation();

    /*** state info to LCD display ***/
    virtual void updateDisplayStateInformation();
    virtual void assembleDisplayStateInformation();
    virtual void showDisplayStateInformation();
    virtual void updateDisplayComponentsStateInformation();

    /*** logger state variable ***/
    virtual void updateStateVariable();
    virtual void assembleStateVariable();
    virtual void assembleComponentsStateVariable();
    void addToStateVariableBuffer(char* info);
    virtual void postStateVariable();

    /*** particle webhook state log ***/
    virtual void assembleStartupLog(); 
    virtual void assembleMissedDataLog();
    virtual void assembleStateLog(); 
    virtual void queueStateLog(bool log_always = false); 
    virtual void publishStateLog();
    virtual void saveStateLogToSD();

    /*** logger data variable ***/
    virtual void updateDataVariable();
    virtual void assembleComponentsDataVariable();
    void addToDataVariableBuffer(char* info);
    virtual void postDataVariable();

    /*** particle webhook data log ***/
    virtual bool isTimeForDataLogAndClear(); // whether it's time for data clear and log (if logging is on)
    virtual void restartLastDataLog(); // reset last data log
    virtual void clearData(bool clear_persistent = false); // clear data fields
    virtual void logData(); 
    virtual void resetDataLog();
    virtual bool addToDataLogBuffer(char* info);
    virtual bool finalizeDataLog(bool use_common_time, unsigned long common_time = 0);
    virtual void queueDataLog();
    virtual void publishDataLog();
    virtual void saveDataLogToSD();

    /*** logger debug variable ***/
    virtual void updateDebugVariable();
    virtual void assembleComponentsDebugVariable();
    void addToDebugVariableBuffer(char* var, char* info);
    virtual void postDebugVariable();

};
