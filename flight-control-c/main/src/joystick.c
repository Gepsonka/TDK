#include "joystick.h"



TaskHandle_t xJoystickInteruptTask;
SemaphoreHandle_t joystick_semaphore_handle = NULL;
Joystick_Direction joysctick_state;

void IRAM_ATTR joystick_handle_interrupt_from_isr(void *arg) {
    xTaskResumeFromISR(xJoystickInteruptTask);
}

void vTaskJoystick(void* pvParameters){
    while (1) {
        vTaskSuspend(NULL);
        printf("Joystick interrupt task\n");
        joystick_int_handler();
    }
}

void joystick_init(){
    joystick_semaphore_handle = xSemaphoreCreateMutex();
    joysctick_state = NA;
    lcd_print_current_joystick_direction(joysctick_state);
    xTaskCreate(vTaskJoystick, "JoystickInterruptTask", 1024, NULL, 1, &xJoystickInteruptTask);

}


void joystick_int_handler() {
    printf("Joystick int handler\n");
    if (xSemaphoreTake(joystick_semaphore_handle, 10) == pdTRUE) {
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

        xSemaphoreGive(joystick_semaphore_handle);
    } else {
        printf("Could not acquire semaphore\n");
    }

    lcd_print_joystick_direction();
}