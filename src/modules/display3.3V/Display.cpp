#include "application.h"
#include "Display.h"

void Display::debugDisplay() {
	debug_display = true;
}

void Display::initDisplay() {

	// checking that device available at the supposed address
	if (!exists) Serial.println("INFO: logger does not use an LCD screen.");
	else if (checkAddress()) present = true;
	else Serial.println("WARNING: no I2C LCD fouund, LCD functionality disabled.");

	if (present) {
		// initialize
		begin(Wire, lcd_addr); //Set up the LCD for I2C communication
		
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
		resetAllBuffers();
		clear(); //Clear the display - this moves the cursor to home position as well
		noCursor(); // don't show cursor
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
void Display::resetAllBuffers() {
	for (int i = 0; i < cols * lines; i++) {
		temp_pos[i] = false;
		text[i] = ' ';
		memory[i] = ' ';
	}
	text[cols * lines] = 0;
	memory[cols * lines] = 0;
}

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

void Display::addToBuffer(byte add) {
	size_t end = strlen(buffer);
	if (end < sizeof(buffer)) {
		buffer[end] = add;
		buffer[end+1] = 0;
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

// control functions
void Display::turnDisplayOn() {
	display();
	setFastBacklight(red, green, blue);
};

void Display::turnDisplayOff() {
	setFastBacklight(0, 0, 0);
	noDisplay();
};

void Display::setContrast(byte b) {
	// could use a bigger range but really not helpful to set higher than 100
	byte contrast = map(b, 0, 100, 100, 0);
	SerLCD::setContrast(contrast);
	resetAllBuffers();
	clear();
}

void Display::setColor(byte r, byte g, byte b) {
	red = r;
	green = g;
	blue = b;
	setFastBacklight(red, green, blue);
	resetAllBuffers();
	clear();
}
