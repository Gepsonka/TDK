#ifndef JOYSTICK_H
#define JOYSTICK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lcd.h"
#include <driver/gpio.h>
#include <esp_log.h>


#define SOUTH_PIN 33
#define NORTH_PIN 32
#define EAST_PIN 25
#define WEST_PIN 26


typedef enum {
    NA,
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST,
    NORTH_WEST,
} Joystick_Direction;
 
// start tasks, install interrupts
void joystick_init();

void joystick_int_handler();


#endif