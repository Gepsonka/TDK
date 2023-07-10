//
// Created by gepsonka on 6/18/23.
//


#include "motor.h"


void motor_init() {
    servo_init_servo(MOTOR_PIN, MOTOR_MIN_DUTY);
    sleep_ms(2000);
}

void motor_set_speed_by_percentage(uint8_t percentage) {
    servo_set_millis(MOTOR_PIN, MOTOR_MIN_DUTY + percentage * MOTOR_ONE_PERCENT_DUTY);
}

