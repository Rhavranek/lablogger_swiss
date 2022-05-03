#include "application.h"
#include "LoggerData.h"
#include "LoggerUtils.h"

/** DEBUG **/

void LoggerData::debug() {
  debug_data = true;
}

/** CLEARING **/

void LoggerData::clear(bool clear_persistent) {
  if (!persistent || clear_persistent) {
    setNewestValueInvalid();
    value.clear();
    data_time.clear();
  }
}

void LoggerData::makePersistent() {
  persistent = true;
}

/** DATA **/

int LoggerData::getN() {
  return value.n;
}

double LoggerData::getValue() {
  return value.mean;
}

double LoggerData::getStdDev() {
  return value.getStdDev();
}

unsigned long LoggerData::getDataTime() {
  return (unsigned long) round(data_time.mean);
}

void LoggerData::setVariable(char* var) {
  strncpy(variable, var, sizeof(variable) - 1);
  variable[sizeof(variable)-1] = 0;
}

void LoggerData::setIndex(int i) {
  idx = i;
}

void LoggerData::setNewestValue(double val) {
  newest_value = val;
  newest_value_valid = true;
}

// @param add_decimals how many decimals to add tot he infered decimals (only matters if inferred)
// @param strict checks that only white spaces are left at the end
bool LoggerData::setNewestValue(char* val, bool strict, bool infer_decimals, int add_decimals, const char* sep) {
  char* double_end;
  double d = strtod (val, &double_end);
  int converted = double_end - val;
  int remaining = strlen(val) - converted;
  // nothing converted
  if (converted == 0) {
    setNewestValueInvalid();
    return(false);
  }
  // check for remainder to be only white spaces if strict
  if (strict) {
    char c;
    for (int i = 0; i < remaining; i++) {
      c = val[converted+i];
      if (!isspace(c)) {
        setNewestValueInvalid();
        return(false);
      }
    }
  }
  // infer decimals
  if (infer_decimals) {
    decimals = strlen(val) - strcspn(val, sep) - 1 - remaining + add_decimals;
    if (decimals < 0) decimals = 0;
  }

  setNewestValue(d);
  return(true);
}


void LoggerData::setNewestValueInvalid() {
  newest_value_valid = false;
}

void LoggerData::setNewestDataTime(unsigned long dt) {
  newest_data_time = dt;
}

void LoggerData::saveNewestValue(bool average) {
  if (newest_value_valid) {

    // clear/overwrite values if not averaging or data time has overflowed (for safety)
    if (!average || newest_data_time < getDataTime()) {
      if (newest_data_time < getDataTime())
        Serial.println("WARNING: data time has overflowed --> restarting value to avoid incorrect data");
      value.clear();
      data_time.clear();
    }

    // add new values
    value.add(newest_value);
    data_time.add(newest_data_time);

    // debug
    //Serial.printf("value add: %3.10f, datatime add: %lu\nvalue    : %3.10f, datatime    : %lu, stdev  : %.10f\n",
    //  newest_value, newest_data_time, getValue(), getDataTime(), getStdDev());

    if (debug_data) {
      (average) ?
        Serial.print("DEBUG: new average value saved for ") :
        Serial.print("DEBUG: single value saved for ");
      (getN() > 1) ?
        getDataDoubleWithSigmaText(idx, variable, getValue(), getStdDev(), units, getN(), json, sizeof(json), PATTERN_IKVSUN_SIMPLE, decimals) :
        getDataDoubleText(idx, variable, getValue(), units, json, sizeof(json), PATTERN_IKVU_SIMPLE, decimals);
      Serial.printf("%s (data time = %Lu ms)\n", json, getDataTime());
    }
    
  } else {
    Serial.printf("WARNING: newest value for #%d (%s) not valid and therefore not saved\n", idx, variable);
  }
}

void LoggerData::saveRunningStatsValue(RunningStats rs) {
  if (rs.getN() > 0) {
    setNewestValue(rs.getMean());
    value.clear();
    value.set(rs);
    data_time.clear();
    data_time.add(newest_data_time);
    if (debug_data) {
      Serial.print("DEBUG: new value saved from running stats for ");
      (getN() > 1) ?
        getDataDoubleWithSigmaText(idx, variable, getValue(), getStdDev(), units, getN(), json, sizeof(json), PATTERN_IKVSUN_SIMPLE, decimals) :
        getDataDoubleText(idx, variable, getValue(), units, json, sizeof(json), PATTERN_IKVU_SIMPLE, decimals);
      Serial.printf("%s (data time = %Lu ms)\n", json, getDataTime());
    }
  } else {
    Serial.printf("WARNING: running stats for #%d (%s) has no data and is therefore not saved\n", idx, variable);
  }
}

void LoggerData::setUnits(char* u) {
  strncpy(units, u, sizeof(units) - 1);
  units[sizeof(units)-1] = 0;
}

void LoggerData::setDecimals(int d) {
  decimals = d;
}

int LoggerData::getDecimals() {
  return decimals;
}

void LoggerData::setDebugOnly(bool debug) {
  debug_only = debug;
}

bool LoggerData::isDebugOnly() {
  return debug_only;
}

void LoggerData::setEnabled(bool enable) {
  enabled = enable;
}

bool LoggerData::isEnabled() {
  return enabled;
}

/**** OPERATIONS ****/

bool LoggerData::isVariableIdentical(char* comparison) {
  if (strcmp(variable, comparison) == 0) {
    return(true);
  } else {
    return(false);
  }
}

bool LoggerData::isUnitsIdentical(char* comparison) {
  if (strcmp(units, comparison) == 0) {
    return(true);
  } else {
    return(false);
  }
}

/***** LOGGING *****/

bool LoggerData::assembleLog(bool include_time_offset) {
  if (getN() > 1) {
    // have data
    (include_time_offset) ?
      getDataDoubleWithSigmaText(idx, variable, getValue(), getStdDev(), units, getN(), millis() - getDataTime(), json, sizeof(json), PATTERN_IKVSUNT_JSON, decimals) :
      getDataDoubleWithSigmaText(idx, variable, getValue(), getStdDev(), units, getN(), json, sizeof(json), PATTERN_IKVSUN_JSON, decimals);
    return(true);
  } else if (getN() == 1) {
    // have single data point (sigma is not meaningful)
    (include_time_offset) ?
      getDataDoubleText(idx, variable, getValue(), units, getN(), millis() - getDataTime(), json, sizeof(json), PATTERN_IKVUNT_JSON, decimals) :
      getDataDoubleText(idx, variable, getValue(), units, getN(), json, sizeof(json), PATTERN_IKVUN_JSON, decimals);
    return(true);
  } else {
    return (false);// don't include if there is no data
  }
}

void LoggerData::assembleInfo() {
  if (newest_value_valid) {
    // valid data
    (strlen(units) > 0) ?
      getDataDoubleText(idx, variable, newest_value, units, json, sizeof(json), PATTERN_IKVU_JSON, decimals) :
      getDataDoubleText(idx, variable, newest_value, json, sizeof(json), PATTERN_IKV_JSON, decimals);
  } else {
    // no valid data
    getDataNullText(idx, variable, json, sizeof(json), PATTERN_IKV_JSON);
  }
}
