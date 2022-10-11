#include "application.h"
#include "LoggerDisplay.h"
#include "LoggerController.h"

/*** constructors ***/

LoggerDisplay::LoggerDisplay (LoggerController *ctrl, uint8_t lcd_cols, uint8_t lcd_lines) : LoggerDisplay(ctrl, new DisplayState(), lcd_cols, lcd_lines, 1) {
}

LoggerDisplay::LoggerDisplay (LoggerController *ctrl, DisplayState *state, uint8_t lcd_cols, uint8_t lcd_lines) : LoggerDisplay(ctrl, state, lcd_cols, lcd_lines, 1) {
}

LoggerDisplay::LoggerDisplay (LoggerController *ctrl, uint8_t lcd_cols, uint8_t lcd_lines, uint8_t n_pages) : LoggerDisplay(ctrl, new DisplayState(), lcd_cols, lcd_lines, n_pages) {
}

LoggerDisplay::LoggerDisplay (LoggerController *ctrl, DisplayState *state, uint8_t lcd_cols, uint8_t lcd_lines, uint8_t n_pages) : LoggerComponent("lcd", ctrl, false, false), Display(lcd_cols, lcd_lines, n_pages), state(state) {
}


/*** setup ***/

void LoggerDisplay::init() {
    Display::initDisplay();
    state->on ? turnDisplayOn() : turnDisplayOff();
    setColor(state->backlight_red, state->backlight_green, state->backlight_blue);
  	setContrast(state->contrast); 
    LoggerComponent::init();
}

/*** loop ***/

void LoggerDisplay::update() {
    Display::updateDisplay();
    LoggerComponent::update();
}

/*** state management ***/
    
size_t LoggerDisplay::getStateSize() { 
    return(sizeof(*state));
}

void LoggerDisplay::saveState(bool always) { 
    if (ctrl->state->save_state || always) {
        EEPROM.put(eeprom_start, *state);
        if (ctrl->debug_state) {
            Serial.printf("DEBUG: component '%s' state saved in memory (if any updates were necessary)\n", id);
        }
    } else {
        Serial.printlnf("DEBUG: component '%s' state NOT saved because state saving is off", id);
    }
} 

bool LoggerDisplay::restoreState() {
    DisplayState *saved_state = new DisplayState();
    EEPROM.get(eeprom_start, *saved_state);
    bool recoverable = saved_state->version == state->version;
    if(recoverable) {
        EEPROM.get(eeprom_start, *state);
        Serial.printf("INFO: successfully restored component state from memory (state version %d)\n", state->version);
    } else {
        Serial.printf("INFO: could not restore state from memory (found state version %d instead of %d), sticking with initial default\n", saved_state->version, state->version);
        saveState(true);
    }
    return(recoverable);
}

void LoggerDisplay::resetState() {
    state->version = 0; // force reset of state on restart
    saveState(true);
}

/*** logger state variable ***/

void LoggerDisplay::assembleStateVariable() {
  char pair[60];
  getDisplayStatePowerText(state->on, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getDisplayStateContrastText(state->contrast, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
  getDisplayStateRGBText(state->backlight_red, state->backlight_green, state->backlight_blue, pair, sizeof(pair)); ctrl->addToStateVariableBuffer(pair);
}

/*** command parsing ***/

bool LoggerDisplay::parseCommand(LoggerCommand *command) {
  if (command->parseVariable(CMD_DISPLAY_ROOT)) {    
    command->extractValue();
    if (!checkPresent()) {
        // no display
        command->error(CMD_DISPLAY_RET_ERR_NO_DISPLAY, CMD_DISPLAY_RET_ERR_NO_DISPLAY_TEXT);
    } else if (parsePower(command)) {
        // display on/off
    } else if (parseReset(command)) {
        // reset display
    } else if (parseContrast(command)) {
        // contrast
    } else if (parseColor(command)) {
        // lcd color
    } else {
        // unkonwn command
        command->errorCommand(); 
    }
  }
  return(command->isTypeDefined());
}

bool LoggerDisplay::parsePower(LoggerCommand *command) {
    if (command->parseValue(CMD_DISPLAY_POWER)) {    
        command->extractUnits();
        if (command->parseUnits(CMD_DISPLAY_POWER_ON)) {
            // power on
            command->success(changePower(true));
        } else if (command->parseUnits(CMD_DISPLAY_POWER_OFF)) {
            // power off
            command->success(changePower(false));
        } else {
            // invalid value
            command->errorValue(); 
        }
    } 
    return(command->isTypeDefined());
}

bool LoggerDisplay::parseReset(LoggerCommand *command) {
    if (command->parseValue(CMD_DISPLAY_RESET)) {    
        initDisplay(); // reset display by renitializing
        changePower(true); // make sure power is on after reset
        command->success(true);
    } 
    return(command->isTypeDefined());
}

bool LoggerDisplay::parseContrast(LoggerCommand *command) {
    if (command->parseValue(CMD_DISPLAY_CONTRAST)) {    
        command->extractUnits();
        byte contrast = atoi(command->units);
        if (command->parseUnits("0") || (contrast > 0 && contrast <= 100)) {
            // set contrast
            command->success(changeContrast(contrast));
        } else {
            // invalid value
            command->errorValue(); 
        }
    } 
    return(command->isTypeDefined());
}

bool LoggerDisplay::parseColor(LoggerCommand *command) {
    if (command->parseValue(CMD_DISPLAY_COLOR)) {    
        command->extractUnits();
        bool valid = false;
        // check if known color
        int i = 0;
        for (; i < sizeof(CMD_DISPLAY_COLORS) / sizeof(DisplayColor); i++) {
            if (strcmp(CMD_DISPLAY_COLORS[i].name, command->units) == 0) break;
        }
        if (i < sizeof(CMD_DISPLAY_COLORS) / sizeof(DisplayColor)) {
            // found a color
            valid = true;
            command->success(changeColor(CMD_DISPLAY_COLORS[i].red, CMD_DISPLAY_COLORS[i].green, CMD_DISPLAY_COLORS[i].blue));
        } else {
            // process rgb
            uint r_end = strcspn(command->units, ",");
            uint g_end = r_end + strcspn(command->units + r_end + 1, ",") + 1;
            if (r_end < strlen(command->units) && (g_end + 1) < strlen(command->units)) {
                uint r = atoi(command->units);
                uint g = atoi(command->units + r_end + 1);
                uint b = atoi(command->units + g_end + 1);
                if (r >= 0 && r < 256 && g >= 0 && g < 256 & b >= 0 && b < 256) {
                    valid = true;
                    command->success(changeColor(r, g, b));
                }
            } 
        }
    
        // invalid value
        if (!valid) command->errorValue(); 
    } 
    return(command->isTypeDefined());
}


/*** state changes ***/

bool LoggerDisplay::changePower(bool on) {
  // only update if necessary
  bool changed = on != state->on;
  if (changed) {
    state->on = on;
    state->on ?
        Serial.printlnf("INFO: %s display turned on", id):
        Serial.printlnf("INFO: %s display turned off", id);
    saveState();
  } else {
    state->on ?
        Serial.printlnf("INFO: %s display already on", id):
        Serial.printlnf("INFO: %s display already off", id);
  }
  state->on ? turnDisplayOn() : turnDisplayOff();
  return(changed);
}

bool LoggerDisplay::changeContrast(byte contrast) {
  // only update if necessary
  bool changed = contrast != state->contrast;
  if (changed) {
    state->contrast = contrast;
    Serial.printlnf("INFO: %s display contrast changed to %d %%", id, state->contrast);
    saveState();
    setContrast(state->contrast);
  } else {
    Serial.printlnf("INFO: %s display contrast unchhanged (%d %%)", id, state->contrast);
  }
  return(changed);
}

bool LoggerDisplay::changeColor(byte r, byte g, byte b) {
  // only update if necessary
  bool changed = r != state->backlight_red || g != state->backlight_green || b != state->backlight_blue;
  if (changed) {
    state->backlight_red = r;
    state->backlight_green = g;
    state->backlight_blue = b;
    Serial.printlnf("INFO: %s display color changed to rgb(%d,%d,%d)", id, state->backlight_red, state->backlight_green, state->backlight_blue);
    saveState();
    setColor(state->backlight_red, state->backlight_green, state->backlight_blue);
  } else {
    Serial.printlnf("INFO: %s display color unchanged rgb(%d,%d,%d)", id, state->backlight_red, state->backlight_green, state->backlight_blue);
  }
  return(changed);
}