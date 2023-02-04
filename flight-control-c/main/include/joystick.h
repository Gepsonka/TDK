#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <driver/gpio.h>
#include <esp_log.h>


#ifndef JOYSTICK_H
#define JOYSTICK_H

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

void joystick_south_pin_interrupt_handler(void* arg);
void joystick_north_pin_interrupt_handler(void* arg);
void joystick_east_pin_interrupt_handler(void* arg);
void joystick_west_pin_interrupt_handler(void* arg);



#endif