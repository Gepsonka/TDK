#ifndef LCD_H
#define LCD_H
#include "i2c.h"
#include <string.h>
#include <stdio.h>
#include "throttle.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "joystick.h"
#include "freertos/semphr.h"


#define LCD_I2C_DEFAULT_PORT 0
#define LCD_DRIVER_I2C_ADDR 0x27

typedef enum {
    FirstLine = 0x80|0x00,
    SecondLine = 0x80|0x40,
    ThirdLine = 0x80|0x14,
    FourthLine = 0x80|0x54
} LCD_LineNumber;

void init_lcd();
void lcd_send_string( char* string);
void lcd_clear_screen();
void lcd_set_cursor(LCD_LineNumber line_num, uint8_t index);

void lcd_print_display_base();
void lcd_print_current_throttle_percentage();
void lcd_print_throttle_percentage(uint16_t raw_val);
void lcd_print_joystick_data();
void lcd_print_current_num_of_devices(uint8_t device_count);

void vLCDGeneralDataDisplay(void* pvParameters);
#endif
