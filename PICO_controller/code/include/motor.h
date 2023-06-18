//
// Created by gepsonka on 6/18/23.
//

#ifndef PICO_CONTROLLER_MOTOR_H
#define PICO_CONTROLLER_MOTOR_H

#include "servo.h"
#include "pico/stdlib.h"

#define MOTOR_MAX_DUTY 2400
#define MOTOR_MIN_DUTY 400
#define MOTOR_ONE_PERCENT_DUTY 20
#define MOTOR_PIN 28

void motor_init();

void motor_set_speed_by_percentage(uint8_t percentage);

#endif //PICO_CONTROLLER_MOTOR_H
