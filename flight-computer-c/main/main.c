#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/task.h>
#include "driver/ledc.h"
#include "esp_timer.h"

void set_servo_angle(float angle)
{
    uint32_t pulse_width_us = (uint32_t)(500 + angle * 2000 / 180); // calculate pulse width in microseconds
    uint32_t duty = (uint32_t)(pulse_width_us * (1 << LEDC_TIMER_13_BIT) / (1000000 / 50)); // calculate duty cycle based on pulse width and PWM frequency
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty); // set duty cycle for LEDC channel
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1); // update duty cycle for LEDC channel
}

void app_main() {
    ledc_timer_config_t ledc_timer_r = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_r);

    ledc_channel_config_t ledc_channel_r = {
            .gpio_num = 32,            // GPIO number
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = LEDC_CHANNEL_0,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = 0,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_r);

    ledc_timer_config_t ledc_timer_l = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_l);

    ledc_channel_config_t ledc_channel_l = {
            .gpio_num = 33,            // GPIO number
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = LEDC_CHANNEL_1,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = 0,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_l);

    set_servo_angle(90);

    while (1) {
//        set_servo_angle(0);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        set_servo_angle(90);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        set_servo_angle(180);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP_LOGI("PWM", "PWM reset...");
    }
}