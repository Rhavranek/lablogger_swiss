#pragma once
#include "SwissScheduler.h"

/*** class ***/
#define SCHEDULER_DT_PATTERN_YMD_24HM "%Y-%m-%d %H:%M"
class SwissScheduler : public SchedulerLoggerComponent {

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
    unsigned long schedule_last = 0;
    unsigned int schedule_wait = 0;
    bool start_testing = false;
    bool testing = false;
    unsigned int testing_waits = 0;

  public:

    // state
    SchedulerState* state;

    /*** constructors ***/
    // derived from controllerlogger component which has NO global time offsets and manages own data clearing by default --> keep defaults
    SwissScheduler (const char *id, LoggerController *ctrl, SchedulerState* state, const SchedulerEvent* schedule, const uint8_t schedule_length, RelayLoggerComponent* power_relay, ) : 
      SchedulerLoggerComponent(id, ctrl, state, SCHEDULER_DT_PATTERN_YMD_24HM, schedule, schedule_length), state(state), pattern(pattern), schedule(schedule), schedule_length(schedule_length) {}

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
    virtual void saveState();
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
