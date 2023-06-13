//
// Created by molnar on 2023.03.01..
//

#ifndef FLIGHT_COMPUTER_C_SERVO_H
#define FLIGHT_COMPUTER_C_SERVO_H

#include <driver/spi_master.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"

#define RIGHT_SERVO_LEDC_CHANNEL LEDC_CHANNEL_0
#define LEFT_SERVO_LEDC_CHANNEL LEDC_CHANNEL_2
#define ELEVATOR_SERVO_LEDC_CHANNEL LEDC_CHANNEL_3
#define RUDDER_SERVO_LEDC_CHANNEL LEDC_CHANNEL_4
#define RIGHT_LANDING_GEAR_SERVO_LEDC_CHANNEL LEDC_CHANNEL_5
#define LEFT_LANDING_GEAR_SERVO_LEDC_CHANNEL LEDC_CHANNEL_6


#define LEFT_SERVO_MIN_DUTY 130
#define LEFT_SERVO_MAX_DUTY 190

#define RIGHT_SERVO_MIN_DUTY 180
#define RIGHT_SERVO_MAX_DUTY 130

#define ELEVATOR_SERVO_MAX_DUTY 180
#define ELEVATOR_SERVO_MIN_DUTY 120

#define RUDDER_SERVO_MIN_DUTY 130

#define RIGHT_LANDING_GEAR_EXTRACTED_DUTY 102
#define RIGHT_LANDING_GEAR_RETRACTED_DUTY 204

#define LEFT_LANDING_GEAR_EXTRACTED_DUTY 204
#define LEFT_LANDING_GEAR_RETRACTED_DUTY 102
#define NEUTRAL_DUTY 152


typedef enum {
    RETRACTED,
    EXTRACTED
} RTLG_Status;

void init_servo();
void set_servo_angle(float angle, uint8_t ledc_channel);
void servo_set_duty(uint8_t pwm_channel, uint8_t duty);

void servo_set_ailerons_servo_by_joystick_percentage(int8_t x_percentage);
void servo_set_elevator_servo_by_joystick_percentage(int8_t y_percentage);
void servo_set_rudder_servo_by_joystick_percentage(int8_t z_percentage);
void servo_extract_RTLG();
void servo_retract_RTLG();
void servo_set_RTLG_status(RTLG_Status status);


#endif //FLIGHT_COMPUTER_C_SERVO_H
