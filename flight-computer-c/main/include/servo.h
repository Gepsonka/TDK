//
// Created by molnar on 2023.03.01..
//

#ifndef FLIGHT_COMPUTER_C_SERVO_H
#define FLIGHT_COMPUTER_C_SERVO_H

#include <driver/spi_master.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "driver/ledc.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#define RIGHT_SERVO_LEDC_CHANNEL LEDC_CHANNEL_0
#define LEFT_SERVO_LEDC_CHANNEL LEDC_CHANNEL_2
#define ELEVATOR_SERVO_LEDC_CHANNEL LEDC_CHANNEL_3

#define LEFT_SERVO_MIN_DUTY 545
#define LEFT_SERVO_MAX_DUTY 682
#define RIGHT_SERVO_MIN_DUTY 682
#define RIGHT_SERVO_MAX_DUTY 545
#define ELEVATOR_SERVO_MAX_DUTY 682
#define ELEVATOR_SERVO_MIN_DUTY 500
#define NEUTRAL_DUTY 614

void init_servo();
void set_servo_angle(float angle, uint8_t ledc_channel);

void set_wing_servos_by_joystick_percentage(int8_t x_percentage);
void set_elevator_servo_by_joystick_percentage(int8_t y_percentage);

#endif //FLIGHT_COMPUTER_C_SERVO_H
