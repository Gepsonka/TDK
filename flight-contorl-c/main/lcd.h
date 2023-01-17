#include "lcd.c"
#include "i2c.h"
#include <string.h>
#include <stdio.h>

#ifndef LCD_H
#define LCD_H



typedef enum {
    FirstLine = 0x80|0x00,
    SecondLine = 0x80|0x40,
    ThirdLine = 0x80|0x14,
    FourthLine = 0x80|0x54
} LineNumber;


typedef struct {
    i2c_port_t i2c_port;
} LCD;

void init_lcd(i2c_port_t i2c_num);
void lcd_send_string(i2c_port_t i2c_num, char* string);
void lcd_clear_screen(i2c_port_t i2c_num);
void lcd_set_cursor(i2c_port_t i2c_num, LineNumber line_num, uint8_t index);

static void send_cmd(i2c_port_t i2c_num, uint8_t cmd);
static void send_data(i2c_port_t i2c_num, uint8_t data);

#endif
