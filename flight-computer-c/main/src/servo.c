//
// Created by molnar on 2023.03.01..
//
#include "servo.h"



void init_servo() {
    ledc_timer_config_t ledc_timer_r = {
            .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_r);

    ledc_channel_config_t ledc_channel_r = {
            .gpio_num = 32,            // GPIO number
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = RIGHT_SERVO_LEDC_CHANNEL,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = NEUTRAL_DUTY,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_r);

    ledc_timer_config_t ledc_timer_l = {
            .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_l);

    ledc_channel_config_t ledc_channel_l = {
            .gpio_num = 33,            // GPIO number
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = LEFT_SERVO_LEDC_CHANNEL,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = NEUTRAL_DUTY,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_l);

    ledc_timer_config_t ledc_timer_elev = {
            .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_elev);

    ledc_channel_config_t ledc_channel_elev = {
            .gpio_num = 26,            // GPIO number
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = ELEVATOR_SERVO_LEDC_CHANNEL,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = NEUTRAL_DUTY,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_elev);

    ledc_timer_config_t ledc_timer_rudder = {
            .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_rudder);

    ledc_channel_config_t ledc_channel_rudder = {
            .gpio_num = 27,            // GPIO number
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = RUDDER_SERVO_LEDC_CHANNEL,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = NEUTRAL_DUTY,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_rudder);

    ledc_timer_config_t ledc_timer_right_lg = {
            .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_right_lg);

    ledc_channel_config_t ledc_channel_right_lg = {
            .gpio_num = 14,            // GPIO number CHANGE IT!!
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = RIGHT_LANDING_GEAR_SERVO_LEDC_CHANNEL,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = NEUTRAL_DUTY,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_right_lg);

    ledc_timer_config_t ledc_timer_left_lg = {
            .duty_resolution = LEDC_TIMER_11_BIT, // resolution of PWM duty
            .freq_hz = 50,                      // frequency of PWM signal
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .timer_num = LEDC_TIMER_0,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,             // auto select the source clock
    };
    ledc_timer_config(&ledc_timer_left_lg);

    ledc_channel_config_t ledc_channel_left_lg = {
            .gpio_num = 12,            // GPIO number CHANGE IT!!
            .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
            .channel = LEFT_LANDING_GEAR_SERVO_LEDC_CHANNEL,            // LEDC channel (0-7)
            .intr_type = LEDC_INTR_DISABLE,       // no interrupt
            .timer_sel = LEDC_TIMER_0,            // timer index
            .duty = NEUTRAL_DUTY,                            // initial duty cycle
            .hpoint = 0,                          // duty cycle phase
    };
    ledc_channel_config(&ledc_channel_left_lg);

    servo_set_duty(RIGHT_SERVO_LEDC_CHANNEL, 152);
    servo_set_duty(LEFT_SERVO_LEDC_CHANNEL, 152);
    servo_set_duty(ELEVATOR_SERVO_LEDC_CHANNEL, 152);
    servo_set_duty(RUDDER_SERVO_LEDC_CHANNEL, 152);
    servo_set_duty(RIGHT_LANDING_GEAR_SERVO_LEDC_CHANNEL, RIGHT_LANDING_GEAR_EXTRACTED_DUTY);
    servo_set_duty(LEFT_LANDING_GEAR_SERVO_LEDC_CHANNEL, LEFT_LANDING_GEAR_EXTRACTED_DUTY);
    printf("servos in neutral\n");
}

void servo_set_duty(uint8_t pwm_channel, uint8_t duty) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, pwm_channel, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, pwm_channel);
}

void set_servo_angle(float angle, uint8_t ledc_channel)
{
    uint32_t pulse_width_us = (uint32_t)(500 + angle * 2000 / 180); // calculate pulse width in microseconds
    uint32_t duty = (uint32_t)(pulse_width_us * (1 << LEDC_TIMER_10_BIT) / (1000000 / 50)); // calculate duty cycle based on pulse width and PWM frequency
    printf("Duty: %lu\n", duty);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, ledc_channel, duty); // set duty cycle for LEDC channel
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, ledc_channel); // update duty cycle for LEDC channel
}

void set_wing_servos_by_joystick_percentage(int8_t x_percentage) {
    uint16_t difference = NEUTRAL_DUTY - LEFT_SERVO_MIN_DUTY;
    float one_percent_duty = ((float) difference / (float) 100);

    if (x_percentage > 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEFT_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY + (int16_t) (one_percent_duty * (float) x_percentage));
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEFT_SERVO_LEDC_CHANNEL);
        //printf("x duty: %d\n", NEUTRAL_DUTY + (uint16_t) (one_percent_duty * (float) x_percentage));
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, RIGHT_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY + (int16_t) (one_percent_duty * (float) x_percentage));
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, RIGHT_SERVO_LEDC_CHANNEL);
    } else if (x_percentage < 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEFT_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY + (int16_t) (one_percent_duty * (float) x_percentage));
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEFT_SERVO_LEDC_CHANNEL);
        //printf("x duty: %d\n", NEUTRAL_DUTY + (uint16_t) (one_percent_duty * (float) x_percentage));
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, RIGHT_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY + (int16_t) (one_percent_duty * (float) x_percentage));
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, RIGHT_SERVO_LEDC_CHANNEL);
    } else if (x_percentage == 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEFT_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEFT_SERVO_LEDC_CHANNEL);
        //printf("x duty: %d\n", NEUTRAL_DUTY + (uint16_t) (one_percent_duty * (float) x_percentage));
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, RIGHT_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, RIGHT_SERVO_LEDC_CHANNEL);
    }
}

void set_elevator_servo_by_joystick_percentage(int8_t y_percentage) {
    uint16_t difference = NEUTRAL_DUTY - ELEVATOR_SERVO_MIN_DUTY;
    float one_percent_duty = ((float) difference / (float) 100);

    if (y_percentage < 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ELEVATOR_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY + (int16_t) (one_percent_duty * (float) y_percentage));
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ELEVATOR_SERVO_LEDC_CHANNEL);
    } else if (y_percentage > 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ELEVATOR_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY + (int16_t) (one_percent_duty * (float) y_percentage));
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ELEVATOR_SERVO_LEDC_CHANNEL);
    } else if (y_percentage == 0) {
        ledc_set_duty(LEDC_HIGH_SPEED_MODE, ELEVATOR_SERVO_LEDC_CHANNEL, NEUTRAL_DUTY);
        ledc_update_duty(LEDC_HIGH_SPEED_MODE, ELEVATOR_SERVO_LEDC_CHANNEL);
    }
}