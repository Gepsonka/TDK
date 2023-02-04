#include "throttle.h"

esp_adc_cal_characteristics_t adc1_chars;

static void adc_init()
{
    adc1_config_width(ADC_WIDTH);
    adc2_config_channel_atten(ADC_CHANNEL, ADC_ATTEN);
}

void init_throttle() {
    adc_init();
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 0, &adc1_chars);
    adc_cali_handle_t calib_handle = NULL;
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_WIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &calib_handle));
}


uint16_t throttle_convert_to_percentage(uint16_t raw_value){
    return (raw_value / THROTTLE_RAW_MAX ) * 100;
}