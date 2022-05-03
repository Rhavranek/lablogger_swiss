#include "application.h"
#include "SerialReaderLoggerComponent.h"

/*** setup ***/

void SerialReaderLoggerComponent::init() {
    DataReaderLoggerComponent::init();
    // initialize serial communication
    Serial.printlnf("INFO: initializing serial communication, baud rate '%ld'", serial_baud_rate);
    Serial1.begin(serial_baud_rate, serial_config);

    // empty serial read buffer
    while (Serial1.available()) Serial1.read();
    resetSerialBuffers();
}

/*** read data ***/

bool SerialReaderLoggerComponent::isPastRequestDelay() {
    // check for min request delay
    return(
        (millis() - data_received_last) > min_request_delay &&
        // and if sequential reader that overall idle is at least the min request delay
        (!sequential || (millis() - ctrl->sequential_data_idle_start) > min_request_delay)
    );
}

bool SerialReaderLoggerComponent::isTimeForRequest() {
    // check for min request delay
    return(DataReaderLoggerComponent::isTimeForRequest() && isPastRequestDelay());
}

bool SerialReaderLoggerComponent::isTimedOut() {
    // whether the reader is timed out - by default if it's been longer than the timeout period
    return((millis() - data_received_last) > timeout);
}

void SerialReaderLoggerComponent::sendSerialDataRequest() {
  if (strlen(request_command) > 0) {
    if (ctrl->debug_data) {
        Serial.printlnf("DEBUG: sending the following command over serial connection for component '%s': %s", id, request_command);
    }
    Serial1.print(request_command);
  }
}

void SerialReaderLoggerComponent::idleDataRead() {
    // discard everyhing coming from the serial connection
    if (Serial1.available()) {
      unsigned int i = 0;
      while (Serial1.available()) { 
        byte b = Serial1.read();
        if (debug_component) {
          i++;
          (b >= SERIAL_B_C_START && b <= SERIAL_B_C_END) ?
            Serial.printlnf("SERIAL: IDLE byte #%d: %i (dec) = %x (hex) = '%c' (char)", i, (int) b, b, (char) b) :
            Serial.printlnf("SERIAL: IDLE byte #%d: %i (dec) = %x (hex) = (special char)", i, (int) b, b, (char) b);
        }
      }
      data_received_last = millis();
      if (sequential) ctrl->sequential_data_idle_start = millis(); // reset idle start counter
    }
}

void SerialReaderLoggerComponent::initiateDataRead() {
    // initiate data read by sending command and registering resetting number of received bytes
    DataReaderLoggerComponent::initiateDataRead();
    if (!isManualDataReader()) sendSerialDataRequest();
    n_byte = 0;
}

void SerialReaderLoggerComponent::readData() {
    // check serial connection for data
    if (data_read_status == DATA_READ_WAITING && Serial1.available()) {
      while (data_read_status == DATA_READ_WAITING && Serial1.available()) {

          // read byte
          prev_byte = (n_byte > 0) ? new_byte : 0;
          new_byte = Serial1.read();
          n_byte++;

          // first byte
          if (n_byte == 1) startData();

          // proces byte
          processNewByte();

          // if working with a data pattern --> mark completion
          if (data_pattern_size > 0 && data_pattern_pos >= data_pattern_size) {
              data_read_status = DATA_READ_COMPLETE;
          }

      }
      data_received_last = millis();
    }
}

void SerialReaderLoggerComponent::completeDataRead() {
    DataReaderLoggerComponent::completeDataRead();
}

void SerialReaderLoggerComponent::registerDataReadError() {
    Serial.printlnf("WARNING: registering data read error at byte# %d: %x = %x", n_byte, new_byte, (char) new_byte);
    ctrl->lcd->printLineTemp(1, "ERR: serial error");
    error_counter++;
}

void SerialReaderLoggerComponent::handleDataReadTimeout() {
    DataReaderLoggerComponent::handleDataReadTimeout();
    if (ctrl->debug_data) {
        Serial.printlnf("DEBUG: registering read timeout with serial data at byte# %d and buffer = '%s'", n_byte, data_buffer);
    }
}

/*** manage data ***/

void SerialReaderLoggerComponent::startData() {
  DataReaderLoggerComponent::startData();
  resetSerialBuffers();
  data_pattern_pos = 0;
  stay_on = false;
}

void SerialReaderLoggerComponent::processNewByte() {
  if (debug_component) {
    (new_byte >= SERIAL_B_C_START && new_byte <= SERIAL_B_C_END) ?
      Serial.printlnf("SERIAL: byte# %03d: %i (dec) = %x (hex) = '%c' (char)", n_byte, (int) new_byte, new_byte, (char) new_byte) :
      Serial.printlnf("SERIAL: byte# %03d: %i (dec) = %x (hex) = (special char)", n_byte, (int) new_byte, new_byte);
  }
  if (new_byte >= SERIAL_B_C_START && new_byte <= SERIAL_B_C_END) {
    appendToSerialDataBuffer(new_byte); // all data
  } else if (new_byte == 13 || new_byte == 10) {
    // 13 = carriage return, 10 = line feed
    appendToSerialDataBuffer(10); // add new line to all data
  }
  // extend in derived classes
}

void SerialReaderLoggerComponent::finishData() {
    // extend in derived classes, typically only save values if error_count == 0
}

/*** debug variable ***/

void SerialReaderLoggerComponent::assembleDebugVariable() {
    DataReaderLoggerComponent::assembleDebugVariable();
    ctrl->addToDebugVariableBuffer("s", data_buffer);
}

/*** work with data patterns ***/

void SerialReaderLoggerComponent::stayOnPattern(int pattern) {
  stay_on = true;
  stay_on_pattern = pattern;
}

void SerialReaderLoggerComponent::nextPatternPos() {
  // next data pattern position
  data_pattern_pos++;
  stay_on = false;
}

bool SerialReaderLoggerComponent::matchesPattern(byte b, int pattern) {
  if (pattern == SERIAL_P_DIGIT && (b >= SERIAL_B_0 && b <= SERIAL_B_9)) {
    return(true);
  } else if (pattern == SERIAL_P_NUMBER && ((b >= SERIAL_B_0 && b <= SERIAL_B_9) || b == SERIAL_B_PLUS || b == SERIAL_B_MINUS || b == SERIAL_B_DOT)) {
    return(true);
  } else if (pattern == SERIAL_P_ASCII && (b >= SERIAL_B_C_START && b <= SERIAL_B_C_END)) {
    return(true);
  } else if (pattern == SERIAL_P_ANY && b > 0) {
    return(true);
  }
  return(false);
}

bool SerialReaderLoggerComponent::moveStayedOnPattern() {
  // if preivously stayed on pattern but now no longer on that same pattern
  if (stay_on && !matchesPattern(new_byte, stay_on_pattern)) {
    // be save about moving to next pattern pos
    if (data_pattern_pos + 1 < data_pattern_size) nextPatternPos();
    stay_on = false;
    return(true);
  }
  return(false);
}

/*** interact with serial data buffers ***/

void SerialReaderLoggerComponent::resetSerialBuffers() {
  resetSerialDataBuffer();
  resetSerialVariableBuffer();
  resetSerialValueBuffer();
  resetSerialUnitsBuffer();
}

void SerialReaderLoggerComponent::resetSerialDataBuffer() {
  for (int i=0; i < sizeof(data_buffer); i++) data_buffer[i] = 0;
  data_charcounter = 0;
}

void SerialReaderLoggerComponent::resetSerialVariableBuffer() {
  for (int i=0; i < sizeof(variable_buffer); i++) variable_buffer[i] = 0;
  variable_charcounter = 0;
}

void SerialReaderLoggerComponent::resetSerialValueBuffer() {
  for (int i=0; i < sizeof(value_buffer); i++) value_buffer[i] = 0;
  value_charcounter = 0;
}

void SerialReaderLoggerComponent::resetSerialUnitsBuffer() {
  for (int i=0; i < sizeof(units_buffer); i++) units_buffer[i] = 0;
  units_charcounter = 0;
}

void SerialReaderLoggerComponent::appendToSerialDataBuffer(byte b) {
  if (data_charcounter < sizeof(data_buffer) - 2) {
    data_buffer[data_charcounter] = (char) b;
    data_charcounter++;
  } else {
    Serial.println("ERROR: serial data buffer not big enough");
    registerDataReadError();
    returnToIdle();
  }
}

void SerialReaderLoggerComponent::appendToSerialVariableBuffer(byte b) {
  if (variable_charcounter < sizeof(variable_buffer) - 2) {
    variable_buffer[variable_charcounter] = (char) b;
    variable_charcounter++;
  } else {
    Serial.println("ERROR: serial variable buffer not big enough");
    registerDataReadError();
    returnToIdle();
  }
}

void SerialReaderLoggerComponent::setSerialVariableBuffer(char* var) {
  resetSerialVariableBuffer();
  strncpy(variable_buffer, var, sizeof(variable_buffer) - 1);
  variable_buffer[sizeof(variable_buffer)-1] = 0;
  variable_charcounter = strlen(variable_buffer);
}

void SerialReaderLoggerComponent::appendToSerialValueBuffer(byte b) {
  if (value_charcounter < sizeof(value_buffer) - 2) {
    value_buffer[value_charcounter] = (char) b;
    value_charcounter++;
  } else {
    Serial.println("ERROR: serial value buffer not big enough");
    registerDataReadError();
    returnToIdle();
  }
}

void SerialReaderLoggerComponent::setSerialValueBuffer(char* val) {
  resetSerialValueBuffer();
  strncpy(value_buffer, val, sizeof(value_buffer) - 1);
  value_buffer[sizeof(value_buffer)-1] = 0;
  value_charcounter = strlen(value_buffer);
}

void SerialReaderLoggerComponent::appendToSerialUnitsBuffer(byte b) {
  if (units_charcounter < sizeof(units_buffer) - 2) {
    units_buffer[units_charcounter] = (char) b;
    units_charcounter++;
  } else {
    Serial.println("ERROR: serial units buffer not big enough");
    registerDataReadError();
    returnToIdle();
  }
}

void SerialReaderLoggerComponent::setSerialUnitsBuffer(char* u) {
  resetSerialUnitsBuffer();
  strncpy(units_buffer, u, sizeof(units_buffer) - 1);
  units_buffer[sizeof(units_buffer)-1] = 0;
  units_charcounter = strlen(units_buffer);
}
