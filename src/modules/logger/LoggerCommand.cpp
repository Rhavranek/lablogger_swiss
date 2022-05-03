#include "LoggerCommand.h"
#include "LoggerController.h"

/****** COMMAND PARSING *******/

void LoggerCommand::load(String& command_string) {
  reset();
  command_string.toCharArray(command, sizeof(command));
  strcpy(buffer, command);
}

void LoggerCommand::reset() {
  buffer[0] = 0;
  command[0] = 0;
  value[0] = 0;
  units[0] = 0;
  notes[0] = 0;

  strcpy(type, CMD_LOG_TYPE_UNDEFINED);
  strcpy(type_short, CMD_LOG_TYPE_UNDEFINED_SHORT);
  msg[0] = 0;
  data[0] = 0;
  ret_val = CMD_RET_UNDEFINED;
}

// capture command excerpt (until space #n_space) in param
// using char array pointers instead of String to make sure we don't get memory leaks here
// providing size to be sure to be on the safe side
void LoggerCommand::extractParam(char* param, uint size, uint n_space) {
  uint space = strcspn(buffer, " ");
  // skip to later spaces
  if (n_space > 1) {
    for(uint s = 2; s <= n_space; s+=1) {
      space += strcspn(buffer + space + 1, " ") + 1;
    }
  }
  // size safety check
  if (space < size) {
    strncpy (param, buffer, space);
    param[space] = 0;
  } else {
    strncpy (param, buffer, size);
    param[size] = 0;
  }
  // clean up buffer
  if (space == strlen(buffer)) {
    buffer[0] = 0;
  } else {
    for(uint i = space+1; i <= strlen(buffer); i+=1) {
      buffer[i-space-1] = buffer[i];
    }
  }
}

// assigns the next extractable parameter to variable
void LoggerCommand::extractVariable(uint n_space) {
  extractParam(variable, sizeof(variable), n_space);
}

// assigns the next extractable paramter to value
void LoggerCommand::extractValue(uint n_space) {
  extractParam(value, sizeof(value), n_space);
}

// assigns the next extractable parameter to units
void LoggerCommand::extractUnits(uint n_space) {
  extractParam(units, sizeof(units), n_space);
}

// takes the remainder of the command buffer and assigns it to the message
void LoggerCommand::assignNotes() {
  strncpy(notes, buffer, sizeof(notes));
}

// check if variable has the specific value
bool LoggerCommand::parseVariable(char* cmd) {
  if (strcmp(variable, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}

// check if variable has the specific value
bool LoggerCommand::parseValue(char* cmd) {
  if (strcmp(value, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}

// check if units has the specific value
bool LoggerCommand::parseUnits(char* cmd) {
  if (strcmp(units, cmd) == 0) {
    return(true);
  } else {
    return(false);
  }
}

/****** COMMAND STATUS *******/

bool LoggerCommand::isTypeDefined() {
  return(ret_val != CMD_RET_UNDEFINED);
}

bool LoggerCommand::hasStateChanged() {
  return(ret_val >= CMD_RET_SUCCESS && ret_val != CMD_RET_WARN_NO_CHANGE);
}

void LoggerCommand::success(bool state_changed) { success(state_changed, true); }
void LoggerCommand::success(bool state_changed, bool capture_notes) {
  if (state_changed) {
    ret_val = CMD_RET_SUCCESS;
    strcpy(type, CMD_LOG_TYPE_STATE_CHANGED);
    strcpy(type_short, CMD_LOG_TYPE_STATE_CHANGED_SHORT);
  } else {
    warning(CMD_RET_WARN_NO_CHANGE, CMD_RET_WARN_NO_CHANGE_TEXT);
    strcpy(type, CMD_LOG_TYPE_STATE_UNCHANGED);
    strcpy(type_short, CMD_LOG_TYPE_STATE_UNCHANGED_SHORT);
  }
  if (capture_notes) {
    assignNotes();
  }
}

void LoggerCommand::warning(int code, const char* text) {
  // warning affects return code and adds warning message
  ret_val = code;
  setLogMsg(text);
}

void LoggerCommand::error(int code, const char* text) {
  // error changes type and stores entire command in notes
  ret_val = code;
  setLogMsg(text);
  strncpy(type, CMD_LOG_TYPE_ERROR, sizeof(type) - 1);
  strcpy(type_short, CMD_LOG_TYPE_ERROR_SHORT);
  strcpy(notes, command); // store entire command in notes
}

void LoggerCommand::error() {
  error(CMD_RET_ERR, CMD_RET_ERR_TEXT);
}

void LoggerCommand::errorLocked() {
  error(CMD_RET_ERR_LOCKED, CMD_RET_ERR_LOCKED_TEXT);
}

void LoggerCommand::errorCommand() {
  error(CMD_RET_ERR_CMD, CMD_RET_ERR_CMD_TEXT);
}

void LoggerCommand::errorValue() {
  error(CMD_RET_ERR_VAL, CMD_RET_ERR_VAL_TEXT);
}

void LoggerCommand::errorUnits() {
  error(CMD_RET_ERR_UNITS, CMD_RET_ERR_UNITS_TEXT);
}

void LoggerCommand::setLogMsg(const char* log_msg) {
  strncpy(msg, log_msg, sizeof(msg) - 1);
  msg[sizeof(msg)-1] = 0;
}
