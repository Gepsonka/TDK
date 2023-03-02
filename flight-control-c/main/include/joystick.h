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



// start tasks, install interrupts
void joystick_init();

void joystick_int_handler();


#endif