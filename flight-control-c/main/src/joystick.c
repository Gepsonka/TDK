#include "joystick.h"



SemaphoreHandle_t joystick_semaphore_handle = NULL;

Joystick_Direction joysctick_state;

void joystick_init(){ 
    joystick_semaphore_handle = xSemaphoreCreateBinary();
    joysctick_state = NA;

}


void joystick_int_handler() {
    if (xSemaphoreTake(joystick_semaphore_handle, portMAX_DELAY) == pdTRUE) {
        if (gpio_get_level(SOUTH_PIN) && gpio_get_level(EAST_PIN)) {
            joysctick_state = SOUTH_EAST;
        } else if (gpio_get_level(SOUTH_PIN) && gpio_get_level(WEST_PIN)) {
            joysctick_state = SOUTH_WEST;
        } else if (gpio_get_level(NORTH_PIN) && gpio_get_level(EAST_PIN)) {
            joysctick_state = NORTH_EAST;
        } else if (gpio_get_level(NORTH_PIN) && gpio_get_level(WEST_PIN)) {
            joysctick_state = NORTH_WEST;
        } else if (gpio_get_level(NORTH_PIN)) {
            joysctick_state = NORTH;
        } else if (gpio_get_level(SOUTH_PIN)) {
            joysctick_state = SOUTH;
        } else if (gpio_get_level(EAST_PIN)) {
            joysctick_state = EAST;
        } else if (gpio_get_level(WEST_PIN)) {
            joysctick_state = WEST;
        } else {
            joysctick_state = NA;
        }

        lcd_print_current_joystick_direction(joysctick_state);

        xSemaphoreGive(joystick_semaphore_handle);
    }
}