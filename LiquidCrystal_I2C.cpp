// Based on the work by DFRobot

#include "Arduino.h"
#include "LiquidCrystal_I2C.h"

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LiquidCrystal_I2C::LiquidCrystal_I2C() { }

void LiquidCrystal_I2C::oled_init(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows, uint8_t dotsize)
{
	_oled = true;
	_Addr = lcd_Addr;
	_cols = lcd_cols;
	_rows = lcd_rows;
	_dotsize = dotsize;
	_backlightval = LCD_NOBACKLIGHT;
	_init();
}

void LiquidCrystal_I2C::init(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows, uint8_t dotsize)
{
	_Addr = lcd_Addr;
	_cols = lcd_cols;
	_rows = lcd_rows;
	_dotsize = dotsize;
	_backlightval = LCD_NOBACKLIGHT;
	_init();
}

void LiquidCrystal_I2C::_init()
{
	Wire.begin();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin();  
}

void LiquidCrystal_I2C::begin()
{
	if (_rows > 1) {
		_displayfunction |= LCD_2LINE;
	}

	// for some 1 line displays you can select a 10 pixel high font
	if ((_dotsize != 0) && (_rows == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50

	delay(50);

	// Now we pull both RS and R/W low to begin commands
	commandInit(0x00 | _backlightval);		// reset expander and turn backlight off (Bit 8 =1)

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

		// we start in 8bit mode, try to set 4 bit mode
	commandInit((LCD_FUNCTIONSET | LCD_8BITMODE));
	delayMicroseconds(4500); // wait min 4.1ms
      
	// second try
	commandInit((LCD_FUNCTIONSET | LCD_8BITMODE));
	delayMicroseconds(4500); // wait min 4.1ms

	// third go!
	commandInit((LCD_FUNCTIONSET | LCD_8BITMODE));
	delayMicroseconds(150);

	// finally, set to 4-bit interface
	commandInit((LCD_FUNCTIONSET | LCD_4BITMODE));

	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);

	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();

	// clear it off
	clear();

	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);

	home();
}

/********** high level commands, for the user! */

/* ******** writes a singel character to the Display     ******************** */
inline size_t LiquidCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
	return 1;
}

/* ******** writes a NULL terminated string to the Display ****************** */
inline size_t LiquidCrystal_I2C::writeString(const char *value) {
	return writeString(value, strlen(value));
}

/* ******** writes a string with defined length to the Display ************** */
inline size_t LiquidCrystal_I2C::writeString(const char *value, uint8_t length) {
	uint8_t countChar = 0;
	uint8_t countI2C = 0;
	Wire.beginTransmission(_Addr);
	while (countChar < length) {
		pulseEnable((value[countChar]&0xF0)|Rs|_backlightval);			// send upper nibble
		pulseEnable(((value[countChar]<<4)&0xF0)|Rs|_backlightval);		// send lower nibble
		countChar++;													// select next character
		countI2C++;														// count number of bytes to be transferred
		if (countI2C >= (BUFFER_LENGTH>>2)) {							// if buffer will be exceeded on next characater, 4 bytes each character (4bit mode and EN High/Low)
			Wire.endTransmission(false);								// write buffer to I2C display
			Wire.beginTransmission(_Addr);								// and prepare a new transmission
			countI2C = 0;												// start new Byte counting
		}
	}
	Wire.endTransmission();
	return (countChar -1);
}

void LiquidCrystal_I2C::clear(){
	command(LCD_CLEARDISPLAY);											// clear display, set cursor position to zero
	delayMicroseconds(2000);  											// this command takes a long time!
	if (_oled) setCursor(0,0);
}

void LiquidCrystal_I2C::home(){
	command(LCD_RETURNHOME);  											// set cursor position to zero
	delayMicroseconds(2000);  											// this command takes a long time!
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, _cols, 0x40 + _cols };
	if ( row > _rows ) {
		row = _rows-1;    												// we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystal_I2C::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystal_I2C::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystal_I2C::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystal_I2C::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal_I2C::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystal_I2C::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystal_I2C::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This moves the cursor one space to the right
void LiquidCrystal_I2C::moveCursorRight(void)
{
	command(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVERIGHT);
}

// This moves the cursor one space to the left
void LiquidCrystal_I2C::moveCursorLeft(void)
{
	command(LCD_CURSORSHIFT | LCD_CURSORMOVE | LCD_MOVELEFT);
}

// This will 'right justify' text from the cursor
void LiquidCrystal_I2C::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystal_I2C::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

//createChar with PROGMEM input
void LiquidCrystal_I2C::createChar(uint8_t location, const char *charmap) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
	    	write(pgm_read_byte_near(charmap++));
	}
}

// Turn the (optional) backlight off/on
void LiquidCrystal_I2C::noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	Wire.beginTransmission(_Addr);
	Wire.write(0x00 | _backlightval);
	Wire.endTransmission(); 
}

void LiquidCrystal_I2C::backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	Wire.beginTransmission(_Addr);
	Wire.write(0x00 | _backlightval);
	Wire.endTransmission(); 
}

/************ low level data pushing commands **********/

inline void LiquidCrystal_I2C::command(uint8_t value) {
	send(value, 0x00);
}

// 4 bit commands are only used during initialization
// and are required to reliably get the LCD and host in nibble sync
// only the upper nibble are send
inline void LiquidCrystal_I2C::commandInit(uint8_t value) {
	Wire.beginTransmission(_Addr);
	pulseEnable((value&0xF0)|_backlightval);
	Wire.endTransmission();
}

// write single Byte either command or data
void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
	Wire.beginTransmission(_Addr);
	pulseEnable((value&0xF0)|mode|_backlightval);
	pulseEnable(((value<<4)&0xF0)|mode|_backlightval);
	Wire.endTransmission();
}

void LiquidCrystal_I2C::pulseEnable(uint8_t _data){
	Wire.write(_data | En);			// En high
// enable pulse must be >450ns, next Byte via I2C will need 25us @ 400kHz
// no delay is required as processing time is bigger than 450ns
	Wire.write(_data & ~En);		// En low
// commands need > 37us to settle, next 2 Byte via I2C will need ~55us @ 400kHz (25us per Byte plus start/stop condition)
// no delay is required as processing time is bigger than 37us
} 

// Alias functions

void LiquidCrystal_I2C::cursor_on(){
	cursor();
}

void LiquidCrystal_I2C::cursor_off(){
	noCursor();
}

void LiquidCrystal_I2C::blink_on(){
	blink();
}

void LiquidCrystal_I2C::blink_off(){
	noBlink();
}

void LiquidCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows){
		createChar(char_num, rows);
}

void LiquidCrystal_I2C::setBacklight(uint8_t new_val){
	if(new_val){
		backlight();													// turn backlight on
	}else{
		noBacklight();													// turn backlight off
	}
}

void LiquidCrystal_I2C::printstr(const char c[]){
	//This function is not identical to the function used for "real" I2C displays
	//it's here so the user sketch doesn't have to be changed 
	print(c);
}


// unsupported API functions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void LiquidCrystal_I2C::off(){}
void LiquidCrystal_I2C::on(){}
void LiquidCrystal_I2C::setDelay (int cmdDelay,int charDelay) {}
uint8_t LiquidCrystal_I2C::status(){return 0;}
uint8_t LiquidCrystal_I2C::keypad (){return 0;}
uint8_t LiquidCrystal_I2C::init_bargraph(uint8_t graphtype){return 0;}
void LiquidCrystal_I2C::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void LiquidCrystal_I2C::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void LiquidCrystal_I2C::setContrast(uint8_t new_val){}
#pragma GCC diagnostic pop
	
