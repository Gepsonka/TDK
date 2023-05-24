//
// Created by gepsonka on 5/23/23.
//

#include "freertos/FreeRTOS.h"
#include <driver/gpio.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>

#ifndef FLIGHT_CONTROL_C_LANDING_GEAR_H
#define FLIGHT_CONTROL_C_LANDING_GEAR_H

typedef enum  {
    RETRACTED,
    EXTRACTED
} LandingGearState;

void init_landing_gear();
void rtlg_toggle_switch_task(void* params);
uint8_t rtlg_get_status();

#endif //FLIGHT_CONTROL_C_LANDING_GEAR_H
