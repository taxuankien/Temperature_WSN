#include "liquid_crystal.h"

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
// Note, however, that resetting the board doesn't reset the LCD, so we
// can't assume that it's in that state when a sketch starts

uint8_t _rs_pin; // LOW: command. HIGH: character.
uint8_t _rw_pin; // LOW: write to LCD. HIGH: read from LCD.
uint8_t _enable_pin; // activated by a HIGH pulse.
uint8_t _data_pins[8];

uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;

uint8_t _initialized;

uint8_t _numlines;
uint8_t _row_offsets[4];

void liquid_crystal(uint8_t rs, uint8_t enable, uint8_t d0,
        uint8_t d1, uint8_t d2, uint8_t d3)
{
  lcd_init(1, rs, 255, enable, d0, d1, d2, d3, 0, 0, 0, 0);
}

void lcd_init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
			 uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
			 uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
  _rs_pin = rs;
  _rw_pin = rw;
  _enable_pin = enable;

  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3;
  _data_pins[4] = d4;
  _data_pins[5] = d5;
  _data_pins[6] = d6;
  _data_pins[7] = d7;

  if (fourbitmode)
  {
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }
  else
  {
    _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }

  lcd_begin(16, 1);
}

void lcd_begin(uint8_t cols, uint8_t lines)
{
  uint8_t dotsize = LCD_5x8DOTS;

  if (lines > 1)
  {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;

  lcd_set_row_offsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != LCD_5x8DOTS) && (lines == 1))
  {
    _displayfunction |= LCD_5x10DOTS;
  }

  gpio_set_direction(_rs_pin, GPIO_MODE_OUTPUT);

  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
  if (_rw_pin != 255)
  {
    gpio_set_direction(_rw_pin, GPIO_MODE_OUTPUT);
  }

  gpio_set_direction(_enable_pin, GPIO_MODE_OUTPUT);

  // Do these once, instead of every time a character is drawn for speed reasons.
  for (int i=0; i<((_displayfunction & LCD_8BITMODE) ? 8 : 4); ++i)
  {
    gpio_set_direction(_data_pins[i], GPIO_MODE_OUTPUT);
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40 ms after power rises above 2.7 V
  // before sending commands.
  ets_delay_us(50000);

  // Now we pull both RS and R/W low to begin commands
  gpio_set_level(_rs_pin, 0);
  gpio_set_level(_enable_pin, 0);

  if (_rw_pin != 255)
  {
    gpio_set_level(_rw_pin, 0);
  }

  //put the LCD into 4 bit or 8 bit mode
  if (! (_displayfunction & LCD_8BITMODE))
  {
    // this is according to the Hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    lcd_write_4_bits(0x03);
    ets_delay_us(4500); // wait min 4.1ms

    // second try
    lcd_write_4_bits(0x03);
    ets_delay_us(4500); // wait min 4.1ms

    // third go!
    lcd_write_4_bits(0x03);
    ets_delay_us(150);

    // finally, set to 4-bit interface
    lcd_write_4_bits(0x02);
  }
  else
  {
    // this is according to the Hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    lcd_command(LCD_FUNCTIONSET | _displayfunction);
    ets_delay_us(4500);  // wait more than 4.1 ms

    // second try
    lcd_command(LCD_FUNCTIONSET | _displayfunction);
    ets_delay_us(150);

    // third go
    lcd_command(LCD_FUNCTIONSET | _displayfunction);
  }

  // finally, set # lines, font size, etc.
  lcd_command(LCD_FUNCTIONSET | _displayfunction);

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
  lcd_display();

  // clear it off
  lcd_clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  lcd_command(LCD_ENTRYMODESET | _displaymode);

}

void lcd_set_row_offsets(int row0, int row1, int row2, int row3)
{
  _row_offsets[0] = row0;
  _row_offsets[1] = row1;
  _row_offsets[2] = row2;
  _row_offsets[3] = row3;
}

/********** high level commands, for the user! */
void lcd_clear()
{
  lcd_command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  ets_delay_us(2000);  // this command takes a long time!
}

void lcd_home()
{
  lcd_command(LCD_RETURNHOME);  // set cursor position to zero
  ets_delay_us(2000);  // this command takes a long time!
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
  const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
  if ( row >= max_lines )
  {
    row = max_lines - 1;    // we count rows starting w/ 0
  }
  if ( row >= _numlines )
  {
    row = _numlines - 1;    // we count rows starting w/ 0
  }
  lcd_command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

// Turn the display on/off (quickly)
void lcd_no_display()
{
  _displaycontrol &= ~LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_display()
{
  _displaycontrol |= LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void lcd_no_cursor()
{
  _displaycontrol &= ~LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_cursor()
{
  _displaycontrol |= LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void lcd_no_blink()
{
  _displaycontrol &= ~LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void lcd_blink()
{
  _displaycontrol |= LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void lcd_scroll_display_left(void)
{
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void lcd_scroll_display_right(void)
{
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void lcd_left_to_right(void)
{
  _displaymode |= LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void lcd_right_to_left(void)
{
  _displaymode &= ~LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void lcd_autoscroll(void)
{
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void lcd_no_autoscroll(void)
{
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void lcd_create_char(uint8_t location, uint8_t charmap[])
{
  location &= 0x7; // we only have 8 locations 0-7
  lcd_command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++)
  {
    lcd_write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds */

inline void lcd_command(uint8_t value)
{
  lcd_send(value, 0);
}

inline size_t lcd_write(uint8_t value)
{
  lcd_send(value, 1);
  return 1; // assume success
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void lcd_send(uint8_t value, uint8_t mode)
{
  gpio_set_level(_rs_pin, mode);

  // if there is a RW pin indicated, set it low to Write
  if (_rw_pin != 255)
  {
    gpio_set_level(_rw_pin, 0);
  }

  if (_displayfunction & LCD_8BITMODE)
  {
    lcd_write_8_bits(value);
  }
  else
  {
    lcd_write_4_bits(value>>4);
    lcd_write_4_bits(value);
  }
}

void lcd_pulse_enable(void)
{
  gpio_set_level(_enable_pin, 0);
  ets_delay_us(1);
  gpio_set_level(_enable_pin, 1);
  ets_delay_us(1);    // enable pulse must be >450 ns
  gpio_set_level(_enable_pin, 0);
  ets_delay_us(100);   // commands need >37 us to settle
}

void lcd_write_4_bits(uint8_t value)
{
  for (int i = 0; i < 4; i++)
  {
    gpio_set_level(_data_pins[i], (value >> i) & 0x01);
  }
  lcd_pulse_enable();
}

void lcd_write_8_bits(uint8_t value)
{
  for (int i = 0; i < 8; i++)
  {
    gpio_set_level(_data_pins[i], (value >> i) & 0x01);
  }
  lcd_pulse_enable();
}

void lcd_write_string(const char* data){
	while (*data)
	{
		lcd_write(*data++);
	}
}
