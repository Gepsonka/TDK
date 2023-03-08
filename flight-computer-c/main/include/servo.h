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
#define LEFT_SERVO_LEDC_CHANNEL LEDC_CHANNEL_1
void init_servo();
void set_servo_angle(float angle, uint8_t ledc_channel);

#endif //FLIGHT_COMPUTER_C_SERVO_H
