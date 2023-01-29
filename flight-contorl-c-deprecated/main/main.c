


/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/i2c.h"
#include "i2c.h"
#include "lcd.h"
#include "uart.h"
#include "gps.h"
#include "spi.h"

void app_main()
{
    i2c_master_init();
    init_uart();
    ESP_ERROR_CHECK(init_spi());

    init_lcd(0);

    lcd_send_string(0, "Csovii");

    init_gps();

    uint8_t lora_res = lora_init();
    printf("lora init result: %d", lora_res);


    GPSData gps_data;

    gps_read_data(&gps_data);
    
    
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


