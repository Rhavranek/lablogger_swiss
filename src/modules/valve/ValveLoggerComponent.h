#pragma once
#include "SerialReaderLoggerComponent.h"

/*** commands ***/

// device <valveID> pos x [msg] : moves valve to indicated position
#define CMD_VALVE_POSITION  "pos" 
// device <valveID> dir cw/cc [msg] : change directionality of valve
#define CMD_VALVE_DIRECTION "dir"
    #define CMD_VALVE_DIRECTION_CW  "cw" 
    #define CMD_VALVE_DIRECTION_CC  "cc" 

// return codes
#define CMD_VALVE_RET_WARN_MAX_POS      -101
#define CMD_VALVE_RET_WARN_MAX_POS_TEXT "exceeds max pos"

/* state */
struct ValveState {

  uint8_t pos = 1;
  bool cw = true; // which direction to move, true= clockwise, false = counterclockwise
  uint8_t version = 1;

  ValveState() {};

  ValveState (uint8_t pos, bool cw) : pos(pos), cw(cw) {};

};

/*** state variable formatting ***/
static void getValveStatePosText(char* variable, uint8_t pos, char* target, int size, char* pattern, bool include_key = true) {
    char var_cmd[20];
    snprintf(var_cmd, sizeof(var_cmd), "%s-%s", variable, CMD_VALVE_POSITION);
    getStateIntText(var_cmd, pos, "", target, size, pattern, include_key);
}

static void getValveStatePosText(char* variable, uint8_t pos, char* target, int size, bool value_only = false) {
  if (value_only) getValveStatePosText(variable, pos, target, size, PATTERN_V_SIMPLE, false);
  else getValveStatePosText(variable, pos, target, size, PATTERN_KV_JSON, true);
}

static void getValveStateDirText(char* variable, bool cw, char* target, int size, char* pattern, bool include_key = true) {
    char var_cmd[20];
    snprintf(var_cmd, sizeof(var_cmd), "%s-%s", variable, CMD_VALVE_DIRECTION);
    getStateBooleanText(var_cmd, cw, CMD_VALVE_DIRECTION_CW, CMD_VALVE_DIRECTION_CC, target, size, pattern, include_key);
}

static void getValveStateDirText(char* variable, bool cw, char* target, int size, bool value_only = false) {
  if (value_only) getValveStateDirText(variable, cw, target, size, PATTERN_V_SIMPLE, false);
  else getValveStateDirText(variable, cw, target, size, PATTERN_KV_JSON_QUOTED, true);
}


/*** valve component ***/
class ValveLoggerComponent : public SerialReaderLoggerComponent {

  private:

    // constants
    const uint_fast8_t max_pos;
    char* cmd; 

    // flag for stirrer update as soon as serial is IDLE
    bool update_valve = false; 
    bool check_pos = false;

  public:

    // state
    ValveState* state;

    /*** constructors ***/
    // vavle doesn't have global offset, it uses individual data points with different time offsets to report step change
    ValveLoggerComponent (const char *id, LoggerController *ctrl, ValveState* state, const long baud_rate, const long serial_config, uint8_t max_pos, const char *request_command, unsigned int data_pattern_size) : 
      SerialReaderLoggerComponent(id, ctrl, false, baud_rate, serial_config, request_command, data_pattern_size), state(state), max_pos(max_pos) {}
    ValveLoggerComponent (const char *id, LoggerController *ctrl, ValveState* state, const long baud_rate, const long serial_config, uint8_t max_pos, const char *request_command) : 
      ValveLoggerComponent(id, ctrl, state, baud_rate, serial_config, max_pos, request_command, 0) {}
    ValveLoggerComponent (const char *id, LoggerController *ctrl, ValveState* state, const long baud_rate, const long serial_config, uint8_t max_pos, unsigned int data_pattern_size) : 
      ValveLoggerComponent(id, ctrl, state, baud_rate, serial_config, max_pos, "", data_pattern_size) {}
    ValveLoggerComponent (const char *id, LoggerController *ctrl, ValveState* state, const long baud_rate, const long serial_config, uint8_t max_pos) : 
      ValveLoggerComponent(id, ctrl, state, baud_rate, serial_config, max_pos, "", 0) {}

    /*** setup ***/
    uint8_t setupDataVector(uint8_t start_idx);
    virtual void init();
    virtual void completeStartup();

    /*** loop ***/
    virtual void update();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    bool parseCommand(LoggerCommand *command);
    bool parseMovement(LoggerCommand *command);
    
    /*** state/valve changes ***/
    virtual bool changePosition(uint8_t pos);
    virtual bool changeDirection(bool cw);
    virtual void updateValve();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** manage data ***/
    virtual void finishData();

    /*** particle webhook data log ***/
    virtual void logData();

};
