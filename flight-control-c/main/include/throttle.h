#ifndef THROTTLE_H
#define THROTTLE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "lcd.h"

#define ADC_CHANNEL (ADC2_CHANNEL_6)
#define ADC_WIDTH (ADC_WIDTH_BIT_12)
#define ADC_ATTEN (ADC_ATTEN_DB_11)

#define THROTTLE_RAW_MIN 0
#define THROTTLE_RAW_MAX 4095


void init_throttle();
uint8_t throttle_convert_to_percentage(uint16_t raw_value);

#endif