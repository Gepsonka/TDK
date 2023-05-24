//
// Created by gepsonka on 5/23/23.
//

#include "landing_gear.h"



#define LANDING_GEAR_SWITCH_PIN 15


TaskHandle_t interrupt_task_handle;
SemaphoreHandle_t lg_state_mutex;
LandingGearState lg_state;

static void IRAM_ATTR landing_gear_toggle_switch_interrupt(void *args) {
    vTaskResume(interrupt_task_handle);
}

void rtlg_toggle_switch_task(void* params) {
    while (1) {
        if (xSemaphoreTake(lg_state_mutex, portMAX_DELAY) == pdPASS) {
            lg_state = rtlg_get_status();
            switch (lg_state) {
                case RETRACTED:
                    printf("RTLG are set retracted\n");
                    break;
                case EXTRACTED:
                    printf("RTLG are set extracted\n");
                    break;
            }
            xSemaphoreGive(lg_state_mutex);
        }
        vTaskSuspend(interrupt_task_handle);
    }
}


uint8_t rtlg_get_status() {
    printf("status is %d\n", gpio_get_level(LANDING_GEAR_SWITCH_PIN));
    return gpio_get_level(LANDING_GEAR_SWITCH_PIN);
}



void init_landing_gear() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = (1ULL << LANDING_GEAR_SWITCH_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);


    lg_state_mutex = xSemaphoreCreateMutex();
    gpio_isr_handler_add(LANDING_GEAR_SWITCH_PIN, landing_gear_toggle_switch_interrupt, NULL);
    xTaskCreate(rtlg_toggle_switch_task, "rtlg_toggle_switch_task", 2048, NULL, 1, &interrupt_task_handle);

}


