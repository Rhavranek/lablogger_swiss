/**
 * LCD display adapted/simplified from
 * https://github.com/BulldogLowell/LiquidCrystal_I2C_Spark
 **/
#pragma once

// alignments
#define LCD_ALIGN_LEFT   1
#define LCD_ALIGN_RIGHT  2
//#define LCD_ALIGN_CENTER 3 // not yet implemented

// line parameters
#define LCD_LINE_END		 0 // code for line end
#define LCD_LINE_LENGTH  0 // code for full length of the text (if enough space)

// buffers
#define LCD_MAX_SIZE     80 // maximum number of characters on LCD

// custom characters
const byte LCD_UP_ARROW	= 1;
const byte LCD_DOWN_ARROW = 2;
const byte LCD_BUG = 3;

// timings
#define LCD_ERROR_MSG_WAIT  10000 // milliseconds

// lcd constants
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

// Display class handles displaying information
class LoggerDisplay : public Print 
{
private:

	// debug flag
	bool debug_display = false;

	// i2c addresses typically used for LCDs
	const uint8_t i2c_addrs[3] = {0x3f, 0x27, 0x23};
	uint8_t lcd_addr;

	// logger has a display?
	bool exists = true;

	// lcd actually present at one of the i2c addresses?
	bool present = false;

	// last time error was shown
	unsigned long last_error_msg = 0;

	// display layout
	const uint8_t cols, lines;

	// dislay pages
	uint8_t n_pages = 1;
	uint8_t current_page = 1;

	// display data
	uint8_t col_now, line_now;		 // current print position on the display
	char text[LCD_MAX_SIZE + 1];     // the current text of the lcd display
	char memory[LCD_MAX_SIZE + 1];   // the memory text of the lcd display for non temporay messages
	bool temp_pos[LCD_MAX_SIZE + 1]; // which text is only temporary

	// temporary message parameters
	bool temp_text = false;					// whether there is any temporary text
	uint16_t temp_text_show_time = 3000;	// how long current temp text is being shown for (in ms)
	unsigned long temp_text_show_start = 0; // when the last temp text was started (changes reset the start time for all temp text!)

	// keep track of position / navigation
	void moveToPos(uint8_t line, uint8_t col);
	uint16_t getPos();
	uint16_t getPos(uint8_t line, uint8_t col);

public:

	// text buffer for lcd text assembly by user --> use resetBuffer and addToBuffer
	char buffer[LCD_MAX_SIZE + 1];	 

	// empty constructor (no screen)
	LoggerDisplay() : LoggerDisplay(0, 0) {
		exists = false;
	}

	// colums and lines with default number pages (1)
	LoggerDisplay(uint8_t lcd_cols, uint8_t lcd_lines) : LoggerDisplay(lcd_cols, lcd_lines, 1) {

	}

	// standard constructor
	LoggerDisplay(uint8_t lcd_cols, uint8_t lcd_lines, uint8_t n_pages) : cols(lcd_cols), lines(lcd_lines), n_pages(n_pages)
	{
		if (cols * lines > LCD_MAX_SIZE) {
			Serial.println("ERROR: LCD size larger than text buffers, you must adjust LCD_MAX_SIZE or prepare for unexpected behaviour!!!");
		}
		// turn backlight on by default
		_backlightval = LCD_BACKLIGHT;
	}

	// turn debug on
	void debug();

	// initialize the display
	void init();

	// check for valid i2c address
	bool checkAddress();

	// check presence
	bool checkPresent();

	// set temporary text show time (in seconds)
	void setTempTextShowTime(uint8_t show_time);

	/*** paging functions ***/
	void setNumberOfPages(uint8_t n_pages);
	uint8_t getNumberOfPages();
	bool setCurrentPage(uint8_t page);
	bool nextPage(bool loop = true);
	uint8_t getCurrentPage();
	void printPageInfo();

	/*** high level printing functions ***/

	// clears the line (overwrites spaces)
	void clearLine(uint8_t line, uint8_t start = 1, uint8_t end = LCD_LINE_END);

	// move to a specific line (e.g. before adding individual text with print)
	void goToLine(uint8_t line);

	// print normal text (temp text that is still visible is only overwritten with new temp text)
	void print(const char c[], bool temp = false);

	// print a whole line (shortens text if too long, pads with spaces if too short)
	void printLine(uint8_t line, const char text[], uint8_t start, uint8_t end, uint8_t align, bool temp = false);

	// simpler version of printLine with useful defaults (left aligned, start at first character, print whole line)
	void printLine(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t start = 1L);

	// same as the simpler version of printLine but right aligned
	void printLineRight(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t end = LCD_LINE_END);

	// same as the simpler version of printLine but only temporary text
	void printLineTemp(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t start = 1L);

	// same as the simpler version of printLineRight but only tempoary text
	void printLineTempRight(uint8_t line, const char text[], uint8_t length = LCD_LINE_LENGTH, uint8_t end = LCD_LINE_END);

	// print line from buffer equivalents of the above functions
	void printLineFromBuffer(uint8_t line, uint8_t length = LCD_LINE_LENGTH, uint8_t start = 1L);
	void printLineRightFromBuffer(uint8_t line, uint8_t length = LCD_LINE_LENGTH, uint8_t end = LCD_LINE_END);
	void printLineTempFromBuffer(uint8_t line, uint8_t length = LCD_LINE_LENGTH, uint8_t start = 1L);
	void printLineTempRightFromBuffer(uint8_t line, uint8_t length = LCD_LINE_LENGTH, uint8_t end = LCD_LINE_END);

	// assemble buffer
	void resetBuffer();
	void addToBuffer(char* add);

	// clear all temporary text
	void clearTempText();

	// clear whole screen (temp text will stay until timer is up)
	void clearScreen(uint8_t start_line = 1L);

	// call in loop to keep temporary text up to date
	void update();

	/*** lcd configuration ***/
	void init_lcd();
	void clear();
	void createChar(uint8_t, uint8_t[]);
	void home();
	void setCursor(uint8_t, uint8_t);
	void noBacklight();
  	void backlight();

	/*** low level functions ***/
	virtual size_t write(uint8_t); //extended from Print class

	private:
		void command(uint8_t);
		void send(uint8_t, uint8_t);
		void write4bits(uint8_t);
		void expanderWrite(uint8_t);
		void pulseEnable(uint8_t);
		uint8_t _displayfunction;
		uint8_t _displaycontrol;
		uint8_t _displaymode;
		uint8_t _backlightval;
};
