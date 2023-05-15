//
// Created by Molnar Botond on 2023. 03. 05..
//
#ifndef FLIGHT_COMPUTER_C_MOTOR_H
#define FLIGHT_COMPUTER_C_MOTOR_H

#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/mcpwm_prelude.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#define ESC_GPIO_PIN 25
#define ESC_FREQUENCY 50
#define ESC_CHANNEL LEDC_CHANNEL_1
#define ESC_LEDC_TIMER_BIT_NUM     LEDC_TIMER_16_BIT

//#define ESC_MAX_DUTY 1361
//#define ESC_MIN_DUTY 1080

#define ESC_MAX_DUTY 1305
#define ESC_MIN_DUTY 1080

void init_motor();
void motor_set_motor_speed(uint16_t pwm_duty);
uint16_t motor_get_duty_value_from_percentage(uint8_t percentage);

#endif //FLIGHT_COMPUTER_C_MOTOR_H
