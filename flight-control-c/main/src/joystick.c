#include "joystick.h"



SemaphoreHandle_t joystick_semaphore_handle = NULL;

void joystick_init(){
    joystick_semaphore_handle = xSemaphoreCreateMutex();
    //already done in throttle
    //adc1_config_width(ADC_WIDTH);
    adc2_config_channel_atten(JOYSTICK_X_AXIS_ADC_CHANNEL, ADC_ATTEN);
    adc2_config_channel_atten(JOYSTICK_Y_AXIS_ADC_CHANNEL, ADC_ATTEN);
}

