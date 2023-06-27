//
// Created by gepsonka on 6/18/23.
//

#ifndef PICO_CONTROLLER_ELEVON_H
#define PICO_CONTROLLER_ELEVON_H

#include "stdint.h"
#include "servo.h"

#define ELEVON_MIN_PULSE_WIDTH 1280
#define ELEVON_MAX_PULSE_WIDTH 1520
#define ELEVON_ONE_PERCENT_DUTY 1.5

typedef enum {
    LEFT_ELEVON = 21, // representing the connected gpio nums
    RIGHT_ELEVON = 27
} Elevon_Number;

void elevon_init();

float elevon_left_elevon_mix(float pitch_input, float roll_input, float mixing_gain);

float elevon_right_elevon_mix(float pitch_input, float roll_input, float mixing_gain);

void elevon_set_elevon_by_percentage(Elevon_Number elevon_number, float percentage);


#endif //PICO_CONTROLLER_ELEVON_H
