#ifndef liquid_crystal_h
#define liquid_crystal_h

#include <stdint.h>
#include <rom/ets_sys.h>
#include <driver/gpio.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
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

void liquid_crystal(uint8_t rs, uint8_t enable,
		uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);

void lcd_init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
    uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
    uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);

void lcd_begin(uint8_t cols, uint8_t rows);

void lcd_clear();
void lcd_home();

void lcd_no_display();
void lcd_display();
void lcd_no_blink();
void lcd_blink();
void lcd_no_cursor();
void lcd_cursor();
void lcd_scroll_display_left();
void lcd_scroll_display_right();
void lcd_left_to_right();
void lcd_right_to_left();
void lcd_autoscroll();
void lcd_no_autoscroll();

void lcd_set_row_offsets(int row1, int row2, int row3, int row4);
void lcd_create_char(uint8_t, uint8_t[]);
void lcd_set_cursor(uint8_t, uint8_t);
size_t lcd_write(uint8_t);
void lcd_command(uint8_t);

void lcd_send(uint8_t, uint8_t);
void lcd_write_4_bits(uint8_t);
void lcd_write_8_bits(uint8_t);
void lcd_pulse_enable();
void lcd_write_string(const char* data);

#endif
