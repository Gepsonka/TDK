#include <driver/spi_master.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "motor.h"
#include "servo.h"
#include "network.h"
#include <puflib.h>

extern sx127x *lora_device;
extern spi_device_handle_t lora_spi_device;

extern Network_Device_Container device_container;


void app_main() {

    puflib_init(); // needs to be called first in app_main

    enroll_puf(); // enrollment needs to be done only once at the beginning

    // condition will be true, if a PUF response is ready (useful after a restart)
    if(PUF_STATE != RESPONSE_READY) {
        bool puf_ok = get_puf_response();
        if(!puf_ok) {
            get_puf_response_reset(); // the device resets now and the app starts again from app_main
        }
    }

    // PUF_RESPONSE_LEN is a PUF response length in bytes
    for (size_t i = 0; i < PUF_RESPONSE_LEN; ++i) {
        printf("%02X ", PUF_RESPONSE[i]); // PUF_RESPONSE is a buffer with the PUF response
    }
    printf("\n");

    clean_puf_response();


    init_motor();

    init_servo();

    spi_bus_config_t config = {
            .mosi_io_num = LORA_MOSI_PIN,
            .miso_io_num = LORA_MISO_PIN,
            .sclk_io_num = LORA_SCK_PIN,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &config, SPI_DMA_CH_AUTO));


    init_lora(&lora_spi_device, lora_device);

    network_init(&device_container);

    while (1) {
//        motor_set_motor_speed(2048);
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//        motor_set_motor_speed((1 << 12) / 2);
//        vTaskDelay(2000 / portTICK_PERIOD_MS);
//        motor_set_motor_speed(1024);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}