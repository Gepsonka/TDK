#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lcd.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define JOYSTICK_X_AXIS_ADC_CHANNEL (ADC2_CHANNEL_5)
#define JOYSTICK_Y_AXIS_ADC_CHANNEL (ADC2_CHANNEL_4)

#define JOYSTICK_RAW_MIN 0
#define JOYSTICK_RAW_MAX 4095
#define JOYSTICK_REFERENCE_VALUE 1840


// start tasks, install interrupts
void joystick_init();
uint16_t joystick_get_current_x_raw_value();
uint16_t joystick_get_current_y_raw_value();
int8_t joystick_convert_current_joystick_x_direction_to_percentage();
int8_t joystick_convert_current_joystick_y_direction_to_percentage();

#endif