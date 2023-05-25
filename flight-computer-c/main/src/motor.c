//
// Created by Molnar Botond on 2023. 03. 05..
//

#include "motor.h"


void init_motor() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<ESC_GPIO_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configure the GPIO pin for the ESC signal wire
    gpio_reset_pin(ESC_GPIO_PIN);
    gpio_set_direction(ESC_GPIO_PIN, GPIO_MODE_OUTPUT);

    // Configure the LEDC module for PWM on the ESC signal wire
    ledc_timer_config_t timer_config = {
            .duty_resolution = LEDC_TIMER_11_BIT,
            .freq_hz = ESC_FREQUENCY,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .timer_num = LEDC_TIMER_2
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
            .channel = ESC_CHANNEL,
            .duty = ESC_MIN_DUTY,
            .gpio_num = ESC_GPIO_PIN,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .timer_sel = LEDC_TIMER_2
    };
    ledc_channel_config(&channel_config);

    // Send a signal to the ESC to arm it
    // ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 2048, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, ESC_MIN_DUTY); // set duty cycle for LEDC channel
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

void motor_set_motor_speed(uint16_t pwm_duty) {
    ESP_LOGI("MOTOR", "motor duty set to: %d", pwm_duty);
    if (pwm_duty < ESC_MIN_DUTY) {
        pwm_duty = ESC_MIN_DUTY;
    } else if (pwm_duty > ESC_MAX_DUTY) {
        pwm_duty = ESC_MAX_DUTY;
    }

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, pwm_duty); // set duty cycle for LEDC channel
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
}

uint16_t motor_get_duty_value_from_percentage(uint8_t percentage) {
    float one_percent_duty = (float) (ESC_MAX_DUTY - ESC_MIN_DUTY) / 100.0;

    printf("Duty at %d%%: %d",percentage, (uint16_t) ((float)percentage * one_percent_duty) + ESC_MIN_DUTY);
    return (uint16_t) ((float)percentage * one_percent_duty) + ESC_MIN_DUTY;
}

