#include <driver/spi_master.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include "motor.h"
#include "servo.h"
#include "lora.h"
#include "network.h"

extern sx127x *lora_device;
extern spi_device_handle_t lora_spi_device;

extern Network_Device_Container device_container;


void app_main() {

    init_servo();

    init_motor();

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
        set_servo_angle(0, RIGHT_SERVO_LEDC_CHANNEL);
        set_servo_angle(180, LEFT_SERVO_LEDC_CHANNEL);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        set_servo_angle(180, RIGHT_SERVO_LEDC_CHANNEL);
        set_servo_angle(0, LEFT_SERVO_LEDC_CHANNEL);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}