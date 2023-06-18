//
// Created by gepsonka on 6/18/23.
//

#ifndef PICO_CONTROLLER_SERVO_H
#define PICO_CONTROLLER_SERVO_H

#include "inttypes.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"


#define SERVO_NEUTRAL_PULSE_WIDTH 1400

void servo_init_servo(int servoPin, float startMillis);

void servo_set_millis(int servoPin, float millis);


#endif //PICO_CONTROLLER_SERVO_H
