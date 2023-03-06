#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/task.h>
#include "driver/ledc.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"


#define ESC_GPIO_NUM 25  // ESC control pin
#define LEDC_TIMER_BIT_NUM     LEDC_TIMER_16_BIT
#define ESC_GPIO_PIN 25
#define ESC_FREQUENCY 50
#define ESC_CHANNEL LEDC_CHANNEL_1

void set_servo_angle(float angle)
{
    uint32_t pulse_width_us = (uint32_t)(500 + angle * 2000 / 180); // calculate pulse width in microseconds
    uint32_t duty = (uint32_t)(pulse_width_us * (1 << LEDC_TIMER_13_BIT) / (1000000 / 50)); // calculate duty cycle based on pulse width and PWM frequency
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty); // set duty cycle for LEDC channel
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1); // update duty cycle for LEDC channel
}

void app_main() {
//    ledc_timer_config_t ledc_timer_r = {
//            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
//            .freq_hz = 50,                      // frequency of PWM signal
//            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
//            .timer_num = LEDC_TIMER_0,            // timer index
//            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
//    };
//    ledc_timer_config(&ledc_timer_r);
//
//    ledc_channel_config_t ledc_channel_r = {
//            .gpio_num = 32,            // GPIO number
//            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
//            .channel = LEDC_CHANNEL_0,            // LEDC channel (0-7)
//            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
//            .timer_sel = LEDC_TIMER_0,            // timer index
//            .duty = 0,                            // initial duty cycle
//            .hpoint = 0,                          // duty cycle phase
//    };
//    ledc_channel_config(&ledc_channel_r);
//
//    ledc_timer_config_t ledc_timer_l = {
//            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
//            .freq_hz = 50,                      // frequency of PWM signal
//            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
//            .timer_num = LEDC_TIMER_0,            // timer index
//            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
//    };
//    ledc_timer_config(&ledc_timer_l);
//
//    ledc_channel_config_t ledc_channel_l = {
//            .gpio_num = 33,            // GPIO number
//            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
//            .channel = LEDC_CHANNEL_1,            // LEDC channel (0-7)
//            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
//            .timer_sel = LEDC_TIMER_0,            // timer index
//            .duty = 0,                            // initial duty cycle
//            .hpoint = 0,                          // duty cycle phase
//    };
//    ledc_channel_config(&ledc_channel_l);


    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<ESC_GPIO_NUM);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configure the GPIO pin for the ESC signal wire
    gpio_reset_pin(ESC_GPIO_PIN);
    gpio_set_direction(ESC_GPIO_PIN, GPIO_MODE_OUTPUT);

    // Configure the LEDC module for PWM on the ESC signal wire
    ledc_timer_config_t timer_config = {
            .duty_resolution = LEDC_TIMER_12_BIT,
            .freq_hz = ESC_FREQUENCY,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .timer_num = LEDC_TIMER_1
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
            .channel = ESC_CHANNEL,
            .duty = 0,
            .gpio_num = ESC_GPIO_PIN,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .timer_sel = LEDC_TIMER_1
    };
    ledc_channel_config(&channel_config);

    // Send a signal to the ESC to arm it
    // ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 2048, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 2048); // set duty cycle for LEDC channel
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
    vTaskDelay(pdMS_TO_TICKS(3000));

//    // Send a signal to the ESC to set the speed to 50%
//    // ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 3072, 0);
//    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 3072); // set duty cycle for LEDC channel
//    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
//    vTaskDelay(pdMS_TO_TICKS(3000));
//
//    // Send a signal to the ESC to set the speed to 0%
//    // ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 2048, 0);
//    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 2048); // set duty cycle for LEDC channel
//    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
//    vTaskDelay(pdMS_TO_TICKS(3000));


    //set_servo_angle(90);

    while (1) {
//        set_servo_angle(0);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        set_servo_angle(90);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//        set_servo_angle(180);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 1024); // set duty cycle for LEDC channel
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL, 2048); // set duty cycle for LEDC channel
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ESC_CHANNEL); // update duty cycle for LEDC channel
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        ESP_LOGI("PWM", "PWM reset...");
    }
}