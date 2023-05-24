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

uint16_t throttle_get_thr_raw() {
    uint16_t raw;
    adc2_get_raw(ADC_CHANNEL, ADC_WIDTH, &raw);
    return raw;
}

uint8_t throttle_convert_to_percentage(uint16_t raw_value){
    if (raw_value < THROTTLE_RAW_MIN) {
        return 0;
    }

    if (raw_value > THROTTLE_RAW_MAX) {
        return 100;
    }

    uint16_t true_value = raw_value - THROTTLE_RAW_MIN;
    uint16_t true_value_max = THROTTLE_RAW_MAX - THROTTLE_RAW_MIN;
    return (uint8_t)((((float) true_value / (float) true_value_max)) * 100);
}

