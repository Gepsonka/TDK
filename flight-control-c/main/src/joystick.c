#include "joystick.h"



SemaphoreHandle_t joystick_semaphore_handle = NULL;

Joystick_Direction joysctick_state;

void joystick_init(){ 
    joystick_semaphore_handle = xSemaphoreCreateBinary();

}