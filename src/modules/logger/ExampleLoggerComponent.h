#pragma once
#include "DataReaderLoggerComponent.h"

/* commands */
#define CMD_SETTING           "setting" // Logger "setting yay/nay [notes]" : turns setting on/off
  #define CMD_SETTING_ON      "yay"
  #define CMD_SETTING_OFF     "nay"

/* state */
struct ExampleState {
  bool setting = false;
  uint8_t version = 3;
  ExampleState() {};
  ExampleState(bool setting) : setting(setting) {}
};

/*** state variable formatting ***/

static void getStateSettingText(bool setting, char* target, int size, char* pattern, bool include_key = true) {
  getStateBooleanText(CMD_SETTING, setting, CMD_SETTING_ON, CMD_SETTING_OFF, target, size, pattern, include_key);
}

static void getStateSettingText(bool setting, char* target, int size, bool value_only = false) {
  if (value_only) getStateSettingText(setting, target, size, PATTERN_V_SIMPLE, false);
  else getStateSettingText(setting, target, size, PATTERN_KV_JSON_QUOTED, true);
}

/* component */
class ExampleLoggerComponent : public DataReaderLoggerComponent
{

  private:

    ExampleState *state;

  public:
    
    /*** constructors ***/
    ExampleLoggerComponent (const char *id, LoggerController *ctrl, ExampleState *state) : DataReaderLoggerComponent(id, ctrl, true), state(state) {}

    /*** setup ***/
    virtual uint8_t setupDataVector(uint8_t start_idx);
    virtual void init();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    virtual bool parseCommand(LoggerCommand *command);
    bool parseSetting(LoggerCommand *command);

    /*** state changes ***/
    bool changeStateSetting (bool on);

    /*** state info to LCD display ***/
    virtual void updateDisplayStateInformation();

    /*** logger state variable ***/
    virtual void assembleStateVariable();    

};
