#pragma once
#include "LoggerMath.h"

// Logger data for spark cloud
struct LoggerData {

  // debug
  bool debug_data = false;

  // data information
  char variable[25]; // the name of the data variable
  int idx; // the index of the data
  bool debug_only; // whether this logger data should only be reported in debug mode
  bool enabled; // whether this data is enabled (i.e. actually in use)
  char units[25]; // the units the data is recorded in
  char aux[25]; // any auxiliary or temporary information 

  // newest data
  unsigned long newest_data_time; // the last recorded datetime (in ms)
  double newest_value; // the last recorded value
  bool newest_value_valid; // whether the newest value is valid

  // saved data
  RunningStats value;
  RunningStats data_time;

  // clearing
  // FIXME: consider deprecating this attribute (formerly auto_clear) and all related functionality
  // UPDATE: see use case in Scale! should remain
  // check if it is needed / useded anywhere?
  bool persistent;

  // output parameters
  int decimals; // what should the decimals be? (idxitive = decimals, negative = integers)
  char json[100]; // full data log text

  LoggerData() {
    idx = 0;
    variable[0] = 0;
    units[0] = 0;
    decimals = 0;
    persistent = false;
    debug_only = false;
    enabled = true;
    clear(true);
  };

  LoggerData(int idx) : LoggerData() { setIndex(idx); }
  LoggerData(int idx, bool enabled) : LoggerData(idx) { setEnabled(enabled); }
  LoggerData(int idx, char* var) : LoggerData(idx) { setVariable(var); }
  LoggerData(int idx, char* var, bool debug) : LoggerData(idx, var) { setDebugOnly(debug); }
  LoggerData(int idx, char* var, char* units) : LoggerData(idx, var) { setUnits(units); }
  LoggerData(int idx, char* var, char* units, bool debug) : LoggerData(idx, var, debug) { setUnits(units); }
  LoggerData(int idx, int d) : LoggerData(idx) { setDecimals(d); }
  LoggerData(int idx, char* var, int d) : LoggerData(idx, var) { setDecimals(d); }
  LoggerData(int idx, char* var, int d, bool debug) : LoggerData(idx, var, d) { setDebugOnly(debug); }
  LoggerData(int idx, char* var, char* units, int d) : LoggerData(idx, var, units) { setDecimals(d); }
  LoggerData(int idx, char* var, char* units, int d, bool debug) : LoggerData(idx, var, units, debug) { setDecimals(d); }
  LoggerData(int idx, char* var, char* units, int d, bool debug, bool enabled) : LoggerData(idx, var, units, d, debug) { setEnabled(enabled); }

  // debug
  void debug();

  // clearing
  void clear(bool clear_persistent = false);
  void makePersistent();

  // data
  int getN();
  double getValue();
  double getStdDev();
  unsigned long getDataTime();
  void setVariable(char* var);
  void setIndex(int idx);
  void setNewestValue(double val);
  // returns whether the value is a valid number or not (if strict, expects only white spaces after the value)
  bool setNewestValue(char* val, bool strict = true, bool infer_decimals = false, int add_decimals = 1, const char* sep = ".");
  void setNewestValueInvalid();
  void saveNewestValue(bool average); // set value based on current newest_value (calculate average if true)
  void saveRunningStatsValue(RunningStats rs); // set value from existing running stats
  void setNewestDataTime(unsigned long dt);
  void setUnits(char* u);
  void setDecimals(int d);
  int getDecimals();
  void setDebugOnly(bool debug);
  bool isDebugOnly();
  void enable();
  void setEnabled(bool enable);
  bool isEnabled();

  // operations
  bool isVariableIdentical(char* comparison);
  bool isUnitsIdentical(char* comparison);

  // logging
  bool assembleLog(bool include_time_offset = true); // assemble log (with our without time offset, in seconds)
  void assembleInfo(); // assemble data info
};
