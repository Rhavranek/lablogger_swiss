#pragma once
#include <vector>
#include "LoggerCommand.h"
#include "LoggerData.h"

// forward declaration for controller
class LoggerController;

// component class
class LoggerComponent
{

  protected:

    // debug
    bool debug_component = false;

    // controller
    LoggerController *ctrl;

    // state
    size_t eeprom_start;

    // time offset - whether all data have the same
    bool data_have_same_time_offset;

    // auto clear data - whether data should be cleared after data log and on data reset 
    bool auto_clear_data;

    // keep track of which data has been logged
    int first_data_log_index;
    int last_data_log_index;

  public:

    // component id
    const char *id;

    // data
    std::vector<LoggerData> data;

    /*** constructors ***/
    LoggerComponent (const char *id, LoggerController *ctrl, bool data_have_same_time_offset, bool auto_clear_data) : id(id), ctrl(ctrl), data_have_same_time_offset(data_have_same_time_offset), auto_clear_data(auto_clear_data) {}

    /*** debug ***/
    void debug();

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx); // setup data vector - override in derived clases, has to return the new index
    virtual void init();
    virtual void completeStartup();

    /*** loop ***/
    virtual void update();

    /*** state management ***/
    virtual void setEEPROMStart(size_t start);
    virtual size_t getStateSize();
    virtual void loadState(bool reset = false);
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    virtual bool parseCommand(LoggerCommand *command);

    /*** state changes ***/
    virtual void activateDataLogging();

    /*** state info to LCD display ***/
    virtual void updateDisplayStateInformation();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** logger data variable ***/
    virtual void assembleDataVariable();

    /*** logger debug variable ***/
    virtual void assembleDebugVariable();

    /*** particle webhook data log ***/
    virtual void clearData(bool clear_persistent = false);
    virtual void logData();
    virtual void logStepData(float new_value, float change_cutoff, uint8_t data_i = 0, uint8_t step_i = 1); // helper function for step logs
    virtual bool assembleDataLog();

};
