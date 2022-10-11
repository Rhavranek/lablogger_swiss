/**
 * Display class with logger component type state saving
 */

#pragma once
#include "Display.h"
#include "LoggerComponent.h"

/* commands */

// device "lcd power on|off [msg]" : turn LCD screen on/off
#define CMD_DISPLAY_ROOT   "lcd" 
#define CMD_DISPLAY_POWER  "power" 
    #define CMD_DISPLAY_POWER_ON  "on" 
    #define CMD_DISPLAY_POWER_OFF "off" 

// device "lcd contrast x" : set LCD contrast from 0 to 100% 
#define CMD_DISPLAY_CONTRAST "contrast"

// device "lcd color name|x,y,z" : color name (see list below) or rgb byte values from 0 to 255
#define CMD_DISPLAY_COLOR "color"

// device "lcd reset" to reset LCD state (in case glitches have snuck in)
#define CMD_DISPLAY_RESET  "reset"

// return codes
#define CMD_DISPLAY_RET_ERR_NO_DISPLAY      -111
#define CMD_DISPLAY_RET_ERR_NO_DISPLAY_TEXT "no LCD connected"

// return codes
#define CMD_DISPLAY_RET_ERR_INVALID_COLOR         -112
#define CMD_DISPLAY_RET_ERR_NO_INVALID_COLOR_TEXT "unknown color name or invalid color code"

// predefined colors
struct DisplayColor {
   char *name;
   byte red;
   byte green;
   byte blue;
};

const DisplayColor CMD_DISPLAY_COLORS[] = {
   {"white", 255, 255, 255},
   {"red", 255, 0, 0},
   {"green", 0, 255, 0},
   {"cyan", 0, 255, 255},
   {"yellow", 255, 255, 0},
   {"pink", 255, 0, 255},
   {"orange", 255, 69, 0},
   {"purple", 138, 43, 226}
};

/* state */
struct DisplayState {

  bool on = true; // whether LCD is on or off
  byte contrast = 95; // contrast from 0 to 100
  byte backlight_red = 255; // backlight red from 0 to 255
  byte backlight_green = 255; // backlight red from 0 to 255
  byte backlight_blue = 255; // backlight red from 0 to 255

  uint8_t version = 1;

  DisplayState() {};

  DisplayState (bool on, byte contrast, byte r, byte g, byte b) : on(on), contrast(contrast), backlight_red(r), backlight_green(g), backlight_blue(b) {};

};

/*** state variable formatting ***/

// power on/off
static void getDisplayStatePowerText(bool on, char* target, int size, char* pattern, int include_key = true) {
  getStateBooleanText(CMD_DISPLAY_POWER, on, CMD_DISPLAY_POWER_ON, CMD_DISPLAY_POWER_OFF, target, size, pattern, include_key);
}
static void getDisplayStatePowerText(bool on, char* target, int size, int value_only = false) {
  if (value_only) getDisplayStatePowerText(on, target, size, PATTERN_V_SIMPLE, false);
  else getDisplayStatePowerText(on, target, size, PATTERN_KV_JSON_QUOTED, true);
}

// contrast
static void getDisplayStateContrastText(byte contrast, char* target, int size, char* pattern, bool include_key = true) {
    getStateIntText(CMD_DISPLAY_CONTRAST, contrast, "%", target, size, pattern, include_key);
}

static void getDisplayStateContrastText(byte contrast, char* target, int size, bool value_only = false) {
  if (value_only) getDisplayStateContrastText(contrast, target, size, PATTERN_VU_SIMPLE, false);
  else getDisplayStateContrastText(contrast, target, size, PATTERN_KVU_JSON, true);
}

// rgb
static void getDisplayStateRGBText(byte red, byte green, byte blue, char* target, int size, char* pattern, bool include_key = true) {
  char rgb[20];
  snprintf(rgb, sizeof(rgb), "%d,%d,%d", red, green, blue);
  getStateStringTextWithUnits(CMD_DISPLAY_COLOR, rgb, "rgb", target, size, pattern, include_key);
}

static void getDisplayStateRGBText(byte red, byte green, byte blue, char* target, int size, bool value_only = false) {
  if (value_only) getDisplayStateRGBText(red, green, blue, target, size, PATTERN_VU_SIMPLE, false);
  else getDisplayStateRGBText(red, green, blue, target, size, PATTERN_KVU_JSON, true);
}

// forward declaration for controller
class LoggerController;

/* component */
class LoggerDisplay : public LoggerComponent, public Display
{

  public:

     // state
    DisplayState* state;

    /*** constructors ***/
    LoggerDisplay (LoggerController *ctrl);
    LoggerDisplay (LoggerController *ctrl, uint8_t lcd_cols, uint8_t lcd_lines);
    LoggerDisplay (LoggerController *ctrl, DisplayState *state, uint8_t lcd_cols, uint8_t lcd_lines);
    LoggerDisplay (LoggerController *ctrl, uint8_t lcd_cols, uint8_t lcd_lines, uint8_t n_pages);
    LoggerDisplay (LoggerController *ctrl, DisplayState *state, uint8_t lcd_cols, uint8_t lcd_lines, uint8_t n_pages);

    /*** setup ***/
    virtual void init();
    
    /*** loop ***/
    virtual void update();

    /*** state management ***/
    virtual size_t getStateSize();
    virtual void saveState(bool always = false);
    virtual bool restoreState();
    virtual void resetState();

    /*** logger state variable ***/
    virtual void assembleStateVariable();

    /*** command parsing ***/
    bool parseCommand(LoggerCommand *command);
    bool parsePower(LoggerCommand *command);
    bool parseReset(LoggerCommand *command);
    bool parseContrast(LoggerCommand *command);
    bool parseColor(LoggerCommand *command);

    /*** state changes/info ***/
    bool changePower(bool on);
    bool changeContrast(byte contrast);
    bool changeColor(byte r, byte g, byte b);

};