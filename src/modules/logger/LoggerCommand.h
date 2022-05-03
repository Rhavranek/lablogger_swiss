#pragma once
#include "application.h"
#include "LoggerUtils.h"

// important constants
#define CMD_MAX_CHAR          63  // spark.functions are limited to 63 char long call

struct LoggerCommand {

    // command message
    char command[CMD_MAX_CHAR];
    char buffer[CMD_MAX_CHAR];
    char variable[25];
    char value[20];
    char units[20];
    char notes[CMD_MAX_CHAR];

    // command outcome
    char type[20]; // command type
    char type_short[10]; // short version of the command type (for lcd)
    char msg[100]; // log message
    char data[50]; // data text
    int ret_val; // return value

    // constructors
    LoggerCommand() {};

    // command extraction
    void reset();
    void load(String& command_string);
    void extractParam(char* param, uint size, uint n_space = 1);
    void extractVariable(uint n_space = 1);
    void extractValue(uint n_space = 1);
    void extractUnits(uint n_space = 1);
    void assignNotes();

    // command parsing
    bool parseVariable(char* cmd);
    bool parseValue(char* cmd);
    bool parseUnits(char* cmd);

    // command status
    bool isTypeDefined(); // whether the command type was found
    bool hasStateChanged(); // whether state has been changed successfully by the command
    void success(bool state_changed);
    void success(bool state_changed, bool capture_notes);
    void warning(int code, const char* text);
    void error(int code, const char* text);
    void error();
    void errorLocked();
    void errorCommand();
    void errorValue();
    void errorUnits();

    // set a log message
    void setLogMsg(const char* log_msg);

};

