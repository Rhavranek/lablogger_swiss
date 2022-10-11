#include "application.h"
#include "Display.h"

void Display::debugDisplay() {
	debug_display = true;
}

void Display::initDisplay()
{

	// checking that device available at the supposed address
	if (!exists) Serial.println("INFO: logger does not use an LCD screen.");
	else if (checkAddress()) present = true;
	else Serial.println("WARNING: no I2C LCD fouund, LCD functionality disabled.");

	if (present) {	
		// initialize LCD with custom characters
		init_lcd();
		// up arrow
		byte UP_ARROW_PIXEL_MAP[8] = {4, 14, 21, 4, 4, 4, 4};
		createChar(LCD_UP_ARROW, UP_ARROW_PIXEL_MAP);
		// down arrow
		byte DOWN_ARROW_PIXEL_MAP[8] = {4, 4, 4, 4, 21, 14, 4};
		createChar(LCD_DOWN_ARROW, DOWN_ARROW_PIXEL_MAP);
		// bug
		byte BUG_PIXEL_MAP[8] = {17, 21, 10, 27, 10, 10, 21, 17};
		createChar(LCD_BUG, BUG_PIXEL_MAP);
		// buffers
		for (int i = 0; i < cols * lines; i++) {
			temp_pos[i] = false;
			text[i] = ' ';
			memory[i] = ' ';
		}
		text[cols * lines] = 0;
		memory[cols * lines] = 0;
		moveToPos(1, 1);
	}
}

bool Display::checkAddress()
{
	// checking that device available at the supposed address
	Wire.begin();
	bool success = 0;
	for (int i=0; i < sizeof(i2c_addrs)/sizeof(i2c_addrs[0]); i++) {
		Serial.printf("INFO: checking for lcd at I2C address 0x%02X... ", i2c_addrs[i]);
		Wire.beginTransmission(i2c_addrs[i]);
		success = (Wire.endTransmission() == 0);
		if (success) {
			Serial.println("found -> use.");
			lcd_addr = i2c_addrs[i];
			break;
		} else {
			Serial.println("NOT found.");
		}
	}
	return(success);
}

bool Display::checkPresent()
{
	if (exists && !present)	{
		if (millis() - last_error_msg > LCD_ERROR_MSG_WAIT) {
			Serial.println("ERROR: lcd not present, restart device with LCD properly connected.");
			last_error_msg = millis();
		}
	}
	return (exists && present);
}

void Display::setTempTextShowTime(uint8_t show_time)
{
	temp_text_show_time = show_time * 1000L;
	Serial.printlnf("INFO: setting LCD temporary text timer to %d seconds (%d ms)", show_time, temp_text_show_time);
}

/*** paging functions ***/

void Display::setNumberOfPages(uint8_t n) {
	// make sure current page does no exceed the available number of pages
	if (n < current_page) current_page = n;
	n_pages = n;
}

uint8_t Display::getNumberOfPages() {
	return(n_pages);
}

bool Display::setCurrentPage(uint8_t page) {
	if (page > n_pages) {
		Serial.printlnf("ERROR: requested page %d but there are only %d pages", page, n_pages);
		return(false);
	}
	if(debug_display)
		Serial.printlnf("DEBUG: moving from page %d to page %d", current_page, page);
	current_page = page;
	printPageInfo();
	return(true);
}

bool Display::nextPage(bool loop) {
	if ((current_page + 1) > n_pages && loop) {
		// loop back to first page
		if(debug_display)
			Serial.printlnf("DEBUG: requested page %d but there are only %d pages, looping back to first page", current_page + 1, n_pages);
		return(setCurrentPage(1));
	} else {
		return(setCurrentPage(current_page + 1));
	}
}

uint8_t Display::getCurrentPage() {
	return(current_page);
}

void Display::printPageInfo() {
	// reprinting last line will automaticlaly update page info
	printLine(lines, memory + getPos(lines, 1) + 1);
}

/*** position navigation ***/

void Display::moveToPos(uint8_t line, uint8_t col)
{
	if (checkPresent() && (line_now != line || col_now != col))
	{
		line = (col > cols) ? line + 1 : line; // jump to next line if cols overflow
		col = (col > cols) ? col - cols : col; // jump to next line if cols overflow
		line = (line > lines) ? 1 : line;	  // start at beginning of screen if lines overflow
		line_now = line;
		col_now = col;
		setCursor(col - 1L, line - 1L);
	}
}

uint16_t Display::getPos(uint8_t line, uint8_t col)
{
	return ((line - 1) * cols + col - 1);
}
uint16_t Display::getPos()
{
	return (getPos(line_now, col_now));
}

void Display::goToLine(uint8_t line)
{
	if (checkPresent()) {
		if (line > lines) {
			Serial.printlnf("ERROR: requested move to line %d. Display only has %d lines.", line, lines);
		} else {
			moveToPos(line, 1);
		}
	}
}

/*** high level printing functions ***/

void Display::print(const char c[], bool temp)
{

	if (checkPresent())
	{

		// determine text length to maximally fill the line
		uint8_t length = (strlen(c) > (cols - col_now + 1)) ? cols - col_now + 1 : strlen(c);

		// position information
		uint8_t col_init = col_now;
		uint16_t pos_now = getPos();

		// update actual LCD text (but only parts that are necessary, to avoid slow i2c communication)
		char update[length + 1];
		int needs_update = -1;
		for (uint8_t i = 0; i <= length; i++) {
			if (needs_update > -1 && (i == length || text[pos_now + i] == c[i])) {
				// either at the end OR text buffer the same as new text but prior text has needs_update flag on -> write text
				strncpy(update, c + needs_update, i - needs_update);
				update[i - needs_update] = 0; // make sure it's 0-pointer terminated
				// update lcd
				moveToPos(line_now, col_init + needs_update);
				Print::print(update);
				// store new text in text buffer
				strncpy(text + pos_now + needs_update, update, i - needs_update);
				needs_update = -1; // reset
			} else if (needs_update == -1 && i < length && (temp || !temp_pos[pos_now + i]) && text[pos_now + i] != c[i]) {
				// either a new temp or NOT overwriting a temp position + text buffer not the same as new text (and not at end yet)
				needs_update = i; // mark beginning of update
			}
		}

		// update final position
		moveToPos(line_now, col_init + length);

		// update memory information
		if (temp)
		{
			// temporary message --> start counter and store memory info
			temp_text = true;
			temp_text_show_start = millis();
			if (debug_display) {
				Serial.printlnf(" - flagging positions %d to %d as TEMPORARY", pos_now, pos_now + length - 1);
			}

			for (uint8_t i = pos_now; i < pos_now + length; i++)
			{
				temp_pos[i] = true;
			}
		}
		else
		{
			// non temporary message --> store in memory (don't include null pointer)
			strncpy(memory + pos_now, c, length);
		}

		if (debug_display) {
			Serial.printlnf(" - finished (new cursor location = line %d, col %d), text buffer:\n[1]%s[%d]", line_now, col_now, text, strlen(text));
		}
	}
}

void Display::printLine(uint8_t line, const char text[], uint8_t start, uint8_t end, uint8_t align, bool temp)
{

	if (checkPresent())
	{

		// safety check
		if (line > lines)
		{
			Serial.printlnf("ERROR: requested print on line %d. Display only has %d lines.", line, lines);
			return;
		}

		// move to correct position
		moveToPos(line, start);

		// ensure legitemate start and end points
		end = (end == LCD_LINE_END || end > cols) ? cols : end;
		start = (start > end) ? end : start; // essentially leads to NO print
		uint8_t length = end - start + 1;

		// assemble print text (pad the start/end according to align)
		char full_text[length + 1] = "";
		uint8_t space_start, space_end = 0;
		if (align == LCD_ALIGN_LEFT) {
			space_start = strlen(text);
			space_end = length;
			strncpy(full_text, text, length);
		} else if (align == LCD_ALIGN_RIGHT) {
			space_start = 0;
			space_end = (strlen(text) < length) ? length - strlen(text) : 0;
			strncpy(full_text + space_end, text, length - space_end);
		} else {
			Serial.println("ERROR: unsupported alignment");
		}

		// spaces
		for (int i = space_start; i < space_end; i++) {
			full_text[i] = ' ';
		}

		// paging information - include if we're printing the end part of the line
		if (n_pages > 1 && line == lines && end == cols && length >= 3) {
			if (debug_display)
				Serial.printlnf("DEBUG: updating paging info with current page %d", current_page);
			snprintf(full_text + length - 3, 4, "  %d", current_page);
			if (current_page == n_pages)
				full_text[length - 2] = LCD_UP_ARROW;
			else 
				full_text[length - 2] = LCD_DOWN_ARROW;
		}

		// make sure 0 pointer at the end
		full_text[length] = 0;

		if (debug_display) {
			if (align == LCD_ALIGN_LEFT)
				Serial.printf("Info @ %Lu: printing%s '%s' LEFT on line %u (%u to %u)\n",
							millis(), (temp ? " TEMPORARY" : ""), full_text, line, start, end);
			else if (align == LCD_ALIGN_RIGHT)
				Serial.printf("Info @ %Lu: printing%s '%s' RIGHT on line %u (%u to %u)\n",
							millis(), (temp ? " TEMPORARY" : ""), full_text, line, start, end);
		}

		// send to print
		print(full_text, temp);
	}
}

void Display::printLine(uint8_t line, const char text[], uint8_t length, uint8_t start)
{
	printLine(line, text, start, start + (length == LCD_LINE_LENGTH ? cols : length) - 1, LCD_ALIGN_LEFT, false);
}

void Display::printLineRight(uint8_t line, const char text[], uint8_t length, uint8_t end)
{
	end = (end == LCD_LINE_END || end > cols) ? cols : end;
	uint8_t start = length == LCD_LINE_LENGTH ? 1 : (length <= end ? end - length + 1 : 1);
	printLine(line, text, start, end, LCD_ALIGN_RIGHT, false);
}

void Display::printLineTemp(uint8_t line, const char text[], uint8_t length, uint8_t start)
{
	printLine(line, text, start, start + (length == LCD_LINE_LENGTH ? cols : length) - 1, LCD_ALIGN_LEFT, true);
}

void Display::printLineTempRight(uint8_t line, const char text[], uint8_t length, uint8_t end)
{
	end = (end == LCD_LINE_END || end > cols) ? cols : end;
	uint8_t start = length == LCD_LINE_LENGTH ? 1 : (length <= end ? end - length + 1 : 1);
	printLine(line, text, start, end, LCD_ALIGN_RIGHT, true);
}

// print line from buffer equivalents of the above functions
void Display::printLineFromBuffer(uint8_t line, uint8_t length, uint8_t start) {
	printLine(line, buffer, length, start);
}

void Display::printLineRightFromBuffer(uint8_t line, uint8_t length, uint8_t end) {
	printLineRight(line, buffer, length, end);
}

void Display::printLineTempFromBuffer(uint8_t line, uint8_t length, uint8_t start) {
	printLineTemp(line, buffer, length, start);
}

void Display::printLineTempRightFromBuffer(uint8_t line, uint8_t length, uint8_t end) {
	printLineTempRight(line, buffer, length, end);
}

// assemble buffer
void Display::resetBuffer() {
	buffer[0] = 0; // reset buffer
}

void Display::addToBuffer(char* add) {
  if (buffer[0] == 0) {
    strncpy(buffer, add, sizeof(buffer));
  } else {
    snprintf(buffer, sizeof(buffer),
        "%s%s", buffer, add);
  }
}


void Display::clearLine(uint8_t line, uint8_t start, uint8_t end)
{
	printLine(line, "", start, end);
}

void Display::clearDisplay(uint8_t start_line)
{
	if (checkPresent())
	{
		for (uint8_t i = start_line; i <= lines; i++)
		{
			clearLine(i);
		}
	}
}

void Display::clearTempText()
{

	// revert data
	char revert[cols];
	int needs_revert = -1;
	uint16_t pos, i;

	if (debug_display) {
		Serial.printf("Info @ %Lu: clearing temp messages...\n", millis());
		for (uint8_t line = 1; line <= lines; line++)
		{
			for (uint8_t col = 1; col <= cols; col++)
			{
				pos = getPos(line, col);
				if (col == 1)
					Serial.printf("[%2d]", pos + 1);
				Serial.printf("%s", (temp_pos[pos]) ? "T" : "F");
				if (col == cols)
					Serial.printf("[%2d]\n", pos + 1);
			}
		}
	}

	// find temp text on each row
	for (uint8_t line = 1; line <= lines; line++)
	{
		for (uint8_t col = 1; col <= cols + 1; col++)
		{
			pos = getPos(line, col);
			if (needs_revert > -1 && (col > cols || !temp_pos[pos]))
			{
				// either at end of a temp section or end of the line with temp text to revert
				strncpy(revert, memory + needs_revert, pos - needs_revert);
				revert[pos - needs_revert] = 0; // make sure it's 0-pointer terminated

				if (debug_display) {
					Serial.printlnf(" - reverting line %d, col %d to %d to '%s'",
								line, col - (pos - needs_revert), col - 1, revert);
				}

				// reset affected temp text
				for (i = needs_revert; i < pos; i++)
					temp_pos[i] = false;
				moveToPos(line, col - (pos - needs_revert));
				print(revert, false);
				needs_revert = -1;
			}
			else if (needs_revert == -1 && col <= cols && temp_pos[pos])
			{
				// found the beginning of a temp section
				needs_revert = pos; // mark beginning of revert
			}
		}
	}

	// flag temp text as false and reset all temp fields
	temp_text = false;
}

// loop update
void Display::updateDisplay() {
	if (present && temp_text && (millis() - temp_text_show_start) > temp_text_show_time) {
		clearTempText();
	}
}

/*** lcd configuration ***/

void Display::init_lcd() {
	Wire.setSpeed(CLOCK_SPEED_100KHZ);
	Wire.stretchClock(true);
	Wire.begin();
	
	if (lines > 1) _displayfunction |= LCD_2LINE;
	
	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay(50);

	// Now we pull both RS and R/W low to begin commands
	expanderWrite(_backlightval);
	delay(1000);

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
    // we start in 8bit mode, try to set 4 bit mode
   	write4bits(0x03 << 4);
   	delayMicroseconds(4500); // wait min 4.1ms

	// second try
	write4bits(0x03 << 4);
	delayMicroseconds(4500); // wait min 4.1ms

	// third go!
	write4bits(0x03 << 4);
	delayMicroseconds(150);

	// finally, set to 4-bit interface
	write4bits(0x02 << 4);

	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	command(LCD_DISPLAYCONTROL | _displaycontrol);

	// clear it off
	clear();

	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	home();
}

void Display::clear(){
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void Display::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	delayMicroseconds(30);
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
		delayMicroseconds(40);
	}
}

// move to zero cursor position
void Display::home(){
	command(LCD_RETURNHOME);  // set cursor position to zero
	delayMicroseconds(2000);  // this command takes a long time!
}

// set cursor
void Display::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > lines ) row = lines-1;    // we count rows starting w/0
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the (optional) backlight off/on
void Display::noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void Display::backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	expanderWrite(0);
}

/*** low level functions ***/

// Print::write
size_t Display::write(uint8_t value) {
   send(value, 1);
   return 0;
}

// send command
void Display::command(uint8_t value) {
   send(value, 0);
}

// write either command or data
void Display::send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
	write4bits((highnib)|mode);
	write4bits((lownib)|mode);
}

void Display::write4bits(uint8_t value) {
	//expanderWrite(value); //SK: is this necessary?
	pulseEnable(value);
}

void Display::expanderWrite(uint8_t _data){
	Wire.beginTransmission(lcd_addr);
	delayMicroseconds(2);
	Wire.write((int)(_data) | _backlightval);
	delayMicroseconds(2);
	Wire.endTransmission();
	delayMicroseconds(2);
}

void Display::pulseEnable(uint8_t _data){
	expanderWrite(_data | (1<<2));  // En high
	delayMicroseconds(1);           // enable pulse must be >450ns
	expanderWrite(_data & ~(1<<2)); // En low
	//delayMicroseconds(50);        // commands need > 37us to settle
	delayMicroseconds(1);           // SK: the shorter time seems to work
}
