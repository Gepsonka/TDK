#include "joystick.h"



SemaphoreHandle_t joystick_semaphore_handle = NULL;
esp_adc_cal_characteristics_t joystick_x_adc_chars;
esp_adc_cal_characteristics_t joystick_y_adc_chars;

void joystick_adc_init(){
    joystick_semaphore_handle = xSemaphoreCreateMutex();
    //already done in throttle
    //adc1_config_width(ADC_WIDTH);
    adc2_config_channel_atten(JOYSTICK_X_AXIS_ADC_CHANNEL, ADC_ATTEN_DB_11);
    adc2_config_channel_atten(JOYSTICK_Y_AXIS_ADC_CHANNEL, ADC_ATTEN_DB_11);
}

void joystick_init(){
    joystick_adc_init();
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &joystick_x_adc_chars);
    adc_cali_handle_t calib_handle_x = NULL;
    adc_cali_line_fitting_config_t cali_config_x = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config_x, &calib_handle_x));

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &joystick_y_adc_chars);
    adc_cali_handle_t calib_handle_y = NULL;
    adc_cali_line_fitting_config_t cali_config_y = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config_y, &calib_handle_y));
    //xTaskCreate(vTaskThrottleDisplay, "ThrottleDisplayTask", 2048, NULL, 1, &xThrottleDisplayTaskHandler);
}