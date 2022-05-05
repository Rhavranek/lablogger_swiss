#pragma once
#include "ControllerLoggerComponent.h"

/*** commands ***/

// device <schedulerID> set datetime [msg] : schedule event
#define CMD_SCHEDULER_SET    "set"

// device <scheduleID> test [x] [msg] : test event with [x] seconds for each wait
#define CMD_SCHEDULER_TEST   "test" 

// device <scheduleID> reset [msg] : reset event
#define CMD_SCHEDULER_RESET  "reset" 

/*** state ***/
#define SCHEDULE_UNSCHEDULED 1
#define SCHEDULE_WAITING     2
#define SCHEDULE_RUNNING     3
#define SCHEDULE_COMPLETE    4
struct SchedulerState {

  uint8_t status = SCHEDULE_UNSCHEDULED; // status of the scheduler
  time_t tstart = 0; // scheduled start time
  uint8_t saved_step = 0; // last saved schedule step
  time_t saved_time = 0; // last saved schedule step time
  uint8_t version = 1;

  SchedulerState() {};

};

/*** schedule ***/

#define SECONDS       1
#define MINUTES       SECONDS * 60
#define HOURS         MINUTES * 60
#define DAYS          HOURS * 24

#define EVENT_DEFAULT 0

struct SchedulerEvent {
   const uint8_t event; // event
   const unsigned int wait; // length of wait in seconds
   const char *label;
   const char *description;
   SchedulerEvent(float wait, unsigned int time_unit, uint8_t event, const char* label, const char* description) : wait(round(wait * time_unit)), event(event), label(label), description(description) {};
   SchedulerEvent(float wait, unsigned int time_unit, uint8_t event, const char* label) : SchedulerEvent(wait, time_unit, event, label, "") {};
};

/*** state variable formatting ***/

// time start
static void getSchedulerStateTimeStart(char* variable, time_t tstart, char* target, int size, char* pattern, bool include_key = true) {
  char var_cmd[20];
  snprintf(var_cmd, sizeof(var_cmd), "%s-%s", variable, CMD_SCHEDULER_SET);
  if (tstart > 0) {
    char dt[30];
    Time.format(tstart, "%Y-%m-%d %H:%M:%S UTC").toCharArray(dt, sizeof(dt));
    getStateStringText(var_cmd, dt, target, size, pattern, include_key);
  } else {
    getDataNullText(var_cmd, target, size, pattern);
  }
}

static void getSchedulerStateTimeStart(char* variable, time_t tstart, char* target, int size, bool value_only = false) {
  if (value_only) getSchedulerStateTimeStart(variable, tstart, target, size, PATTERN_V_SIMPLE, false);
  else getSchedulerStateTimeStart(variable, tstart, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// status
static void getSchedulerStateStatus(char* variable, uint8_t status, char* target, int size, char* pattern, bool include_key = true) {
  char var_cmd[20];
  snprintf(var_cmd, sizeof(var_cmd), "%s-%s", variable, "status");
  if (status == SCHEDULE_UNSCHEDULED)
    getStateStringText(var_cmd, "unschedule", target, size, pattern, include_key);
  else if (status == SCHEDULE_WAITING)
    getStateStringText(var_cmd, "waiting", target, size, pattern, include_key);
  else if (status == SCHEDULE_RUNNING)
    getStateStringText(var_cmd, "running", target, size, pattern, include_key);
  else if (status == SCHEDULE_COMPLETE)
    getStateStringText(var_cmd, "complete", target, size, pattern, include_key);
  else
    getStateStringText(var_cmd, "undefined", target, size, pattern, include_key);
}

static void getSchedulerStateStatus(char* variable, uint8_t status, char* target, int size, bool value_only = false) {
  if (value_only) getSchedulerStateStatus(variable, status, target, size, PATTERN_V_SIMPLE, false);
  else getSchedulerStateStatus(variable, status, target, size, PATTERN_KV_JSON_QUOTED, true);
}

/*** scheduler component ***/
#define SCHEDULER_DT_PATTERN_YMD_24HM "%Y-%m-%d %H:%M"
class SchedulerLoggerComponent : public ControllerLoggerComponent {

  private:

    // schedule pattern    
    const char *pattern;

    // command
    char *cmd; 

  protected:

    // scheduler events
    const SchedulerEvent* schedule; 
    const uint8_t schedule_length;
    uint8_t schedule_i = 0;
    time_t schedule_last = 0;
    unsigned int schedule_wait = 0;
    bool start_testing = false;
    bool testing = false;
    unsigned int testing_waits = 0;

  public:

    // state
    SchedulerState* state;

    /*** constructors ***/
    // derived from controllerlogger component which has NO global time offsets and manages own data clearing by default --> keep defaults
    SchedulerLoggerComponent (const char *id, LoggerController *ctrl, SchedulerState* state, const char *pattern, const SchedulerEvent* schedule, const uint8_t schedule_length) : 
      ControllerLoggerComponent(id, ctrl), state(state), pattern(pattern), schedule(schedule), schedule_length(schedule_length) {}
    SchedulerLoggerComponent (const char *id, LoggerController *ctrl, const char *pattern, const SchedulerEvent* schedule, const uint8_t schedule_length) : 
      SchedulerLoggerComponent (id, ctrl, new SchedulerState(), pattern, schedule, schedule_length) {}
    SchedulerLoggerComponent (const char *id, LoggerController *ctrl, const SchedulerEvent* schedule, const uint8_t schedule_length) : 
      SchedulerLoggerComponent (id, ctrl, SCHEDULER_DT_PATTERN_YMD_24HM, schedule, schedule_length) {}

    /*** setup ***/
    uint8_t setupDataVector(uint8_t start_idx);
    virtual void init();
    virtual void completeStartup();

    /*** loop ***/
    virtual void update(); 
    bool checkSchedule();
    void startSchedule();
    virtual void runSchedule();
    virtual void runEvent(uint8_t event);
    void getSchedulerStatus(char* target, int size);
    void finishSchedule();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState(bool always = false);
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    bool parseCommand(LoggerCommand *command);
    bool parseSchedule(LoggerCommand *command);
    
    /*** state changes/info ***/
    bool changeSchedule(tm timeobj);
    bool resetSchedule();
    bool testSchedule(unsigned int waits = 0);
    virtual void activateDataLogging();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** particle webhook data log ***/
    virtual void logData();

};
