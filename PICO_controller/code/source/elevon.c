//
// Created by gepsonka on 6/18/23.
//
#include "elevon.h"

const float elevon_default_mixing_gain = .5;

void elevon_init() {
    servo_init_servo(LEFT_ELEVON, SERVO_NEUTRAL_PULSE_WIDTH);
    servo_init_servo(RIGHT_ELEVON, SERVO_NEUTRAL_PULSE_WIDTH);
}

float elevon_left_elevon_mix(float pitch_input, float roll_input, float mixing_gain) {
    return pitch_input * mixing_gain + roll_input * mixing_gain;
}

float elevon_right_elevon_mix(float pitch_input, float roll_input, float mixing_gain) {
    return pitch_input * mixing_gain - roll_input * mixing_gain;
}

void elevon_set_elevon_by_percentage(Elevon_Number elevon_number,int8_t percentage) {
    servo_set_millis(elevon_number, SERVO_NEUTRAL_PULSE_WIDTH + (float) percentage * (float) ELEVON_ONE_PERCENT_DUTY);
}

