#pragma once
#include "ControllerLoggerComponent.h"

/*** commands ***/

// device <relayID> on/off [msg] : turns relay on/off
#define CMD_RELAY_ON       "on" 
#define CMD_RELAY_OFF      "off" 

/* state */
struct RelayState {

  bool on;
  uint8_t version = 1;

  RelayState() {};

  RelayState (bool on) : on(on) {};

};

/*** state variable formatting ***/
static void getRelayStateText(char* variable, bool on, char* target, int size, char* pattern, bool include_key = true) {
    getStateBooleanText(variable, on, CMD_RELAY_ON, CMD_RELAY_OFF, target, size, pattern, include_key);
}

static void getRelayStateText(char* variable, bool on, char* target, int size, bool value_only = false) {
  if (value_only) getRelayStateText(variable, on, target, size, PATTERN_V_SIMPLE, false);
  else getRelayStateText(variable, on, target, size, PATTERN_KV_JSON_QUOTED, true);
}


/*** relay component ***/
#define RELAY_NORMALLY_OPEN     1
#define RELAY_NORMALLY_CLOSED   2
class RelayLoggerComponent : public ControllerLoggerComponent {

  private:

    // constants
    const int relay_type;
    const int relay_pin;
    char* cmd; 

  public:

    // state
    RelayState* state;

    /*** constructors ***/
    // derived from controllerlogger component which has NO global time offsets and manages own data clearing by default --> keep defaults
    RelayLoggerComponent (const char *id, LoggerController *ctrl, RelayState* state, int pin, int type) : ControllerLoggerComponent(id, ctrl), state(state), relay_pin(pin), relay_type(type) {}
    RelayLoggerComponent (const char *id, LoggerController *ctrl, bool on, int pin, int type) : RelayLoggerComponent (id, ctrl, new RelayState(on), pin, type) {}

    /*** setup ***/
    uint8_t setupDataVector(uint8_t start_idx);
    virtual void init();
    virtual void completeStartup();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState();
    virtual bool restoreState();
    virtual void resetState();

    /*** command parsing ***/
    bool parseCommand(LoggerCommand *command);
    bool parseRelay(LoggerCommand *command);
    
    /*** state changes ***/
    bool changeRelay(bool on);

    /*** relay functions ***/
    void updateRelay();

    /*** state changes ***/
    virtual void activateDataLogging();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** particle webhook data log ***/
    virtual void logData();

};
