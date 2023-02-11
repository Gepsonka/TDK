#include <stdio.h>
#include <driver/gpio.h>
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include <esp_log.h>
#include <esp_intr_alloc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "aes/esp_aes_gcm.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_adc/adc_cali.h"
#include "i2c.h"
#include "lcd.h"
#include "gps.h"
#include "lora.h"
#include "joystick.h"
#include "throttle.h"
#include "security.h"
#include <sx127x.h>



#define UART_TX_PIN 17
#define UART_RX_PIN 16
#define UART_RX_BUFF_SIZE 2048
#define I2C_MASTER_SCL_IO    22    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    21    /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM 0   /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency */
#define ACK_CHECK_EN 1
#define NACK_VAL 0

#define ADC_CHANNEL (ADC2_CHANNEL_6)
#define ADC_WIDTH (ADC_WIDTH_BIT_12)
#define ADC_ATTEN (ADC_ATTEN_DB_11)


static const char *TAG = "UAVsecurity";


extern Joystick_Direction joysctick_state;
extern TaskHandle_t xJoystickInteruptTask;

extern sx127x *lora_device;
extern TaskHandle_t lora_interrupt_handler;

void IRAM_ATTR lora_handle_interrupt_fromisr(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    switch(gpio_num) {
        case LORA_DIO0_PIN:
            xTaskResumeFromISR(lora_interrupt_handler);
            break;
    }
}



void app_main()
{

    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_NUM_2, &uart_config);
    uart_driver_install(UART_NUM_2, UART_RX_BUFF_SIZE, 0, 0, NULL, 0);
    uart_set_pin(UART_NUM_2, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    spi_bus_config_t config = {
        .mosi_io_num = LORA_MOSI_PIN,
        .miso_io_num = LORA_MISO_PIN,
        .sclk_io_num = LORA_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &config, SPI_DMA_CH_AUTO));

    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)LORA_DIO0_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)LORA_DIO0_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)LORA_DIO0_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)LORA_DIO0_PIN, GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(
            gpio_isr_handler_add((gpio_num_t) LORA_DIO0_PIN, lora_handle_interrupt_fromisr, (void *) lora_device));

    init_lora();

    init_lcd();
    lcd_clear_screen();

    lcd_print_display_base();

    joystick_init();

    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)SOUTH_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)SOUTH_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)SOUTH_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)SOUTH_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)SOUTH_PIN, joystick_handle_interrupt_from_isr, (void *)joysctick_state));

    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)NORTH_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)NORTH_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)NORTH_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)NORTH_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)NORTH_PIN, joystick_handle_interrupt_from_isr, (void *)joysctick_state));

    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)EAST_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)EAST_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)EAST_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)EAST_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)EAST_PIN, joystick_handle_interrupt_from_isr, (void *)joysctick_state));
    
    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)WEST_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)WEST_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)WEST_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)WEST_PIN, GPIO_INTR_ANYEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)WEST_PIN, joystick_handle_interrupt_from_isr, (void *)joysctick_state));

    init_gps();

    init_throttle();

    extern uint8_t aes_key[16];
    extern uint8_t plain_data[32];
    extern uint8_t iv[12];

    extern mbedtls_gcm_context aes;

    uint8_t output[32];

    aes_gcm(&aes, aes_key, AES_GCM_ENCRYPT, iv, 12, plain_data, 32, output, 32, (size_t) 32);
    for (uint8_t i = 0; i < 32; i++) {
        printf("%02X\n", output[i]);
    }



    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        uint8_t data[] = {0xCA, 0xFE};
        ESP_ERROR_CHECK(sx127x_set_for_transmission(data, sizeof(data), lora_device));
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, lora_device));
        ESP_LOGI(TAG, "transmitting");
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_device));
    }
}