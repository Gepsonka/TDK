#include <stdio.h>
#include <driver/gpio.h>
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include <esp_log.h>
#include <esp_intr_alloc.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/i2c.h"
#include "i2c.h"
#include "lcd.h"
#include "gps.h"
#include "lora.h"
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

static const char *TAG = "sx127x";

extern sx127x *lora_device;
int messages_sent = 0;
int total_packets_received = 0;
TaskHandle_t handle_interrupt;

void IRAM_ATTR handle_interrupt_fromisr(void *arg)
{
    xTaskResumeFromISR(handle_interrupt);
}

void handle_interrupt_task(void *arg)
{
    while (1)
    {
        vTaskSuspend(NULL);
        sx127x_handle_interrupt((sx127x *)arg);
    }
}

void tx_callback(sx127x *device)
{
    ESP_LOGI(TAG, "transmitted");
}


void rx_callback(sx127x *device) {
  uint8_t *data = NULL;
  uint8_t data_length = 0;
  esp_err_t code = sx127x_read_payload(device, &data, &data_length);
  if (code != ESP_OK) {
    ESP_LOGE(TAG, "can't read %d", code);
    return;
  }
  if (data_length == 0) {
    // no message received
    return;
  }
  uint8_t payload[514];
  const char SYMBOLS[] = "0123456789ABCDEF";
  for (size_t i = 0; i < data_length; i++) {
    uint8_t cur = data[i];
    payload[2 * i] = SYMBOLS[cur >> 4];
    payload[2 * i + 1] = SYMBOLS[cur & 0x0F];
  }
  payload[data_length * 2] = '\0';

  int16_t rssi;
  ESP_ERROR_CHECK(sx127x_get_packet_rssi(device, &rssi));
  float snr;
  ESP_ERROR_CHECK(sx127x_get_packet_snr(device, &snr));
  int32_t frequency_error;
  ESP_ERROR_CHECK(sx127x_get_frequency_error(device, &frequency_error));

  ESP_LOGI(TAG, "received: %d %s rssi: %d snr: %f freq_error: %ld", data_length, payload, rssi, snr, frequency_error);

  total_packets_received++;
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
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &config, 1));
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 100000,
        .spics_io_num = LORA_SS_PIN,
        .queue_size = 16,
        .command_bits = 0,
        .address_bits = 8,
        .dummy_bits = 0,
        .mode = 0};
    spi_device_handle_t spi_device;
    ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &dev_cfg, &spi_device));
    ESP_ERROR_CHECK(sx127x_create(spi_device, &lora_device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, lora_device));
    ESP_ERROR_CHECK(sx127x_set_frequency(437200012, lora_device));
    ESP_ERROR_CHECK(sx127x_reset_fifo(lora_device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, lora_device));
    ESP_ERROR_CHECK(sx127x_set_bandwidth(SX127x_BW_125000, lora_device));
    sx127x_implicit_header_t header = {
        .coding_rate = SX127x_CR_4_5,
        .crc = SX127x_RX_PAYLOAD_CRC_ON,
        .length = 2};
    ESP_ERROR_CHECK(sx127x_set_implicit_header(&header, lora_device));
    ESP_ERROR_CHECK(sx127x_set_modem_config_2(SX127x_SF_9, lora_device));
    ESP_ERROR_CHECK(sx127x_set_syncword(18, lora_device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, lora_device));
    sx127x_set_tx_callback(tx_callback, lora_device);

    

    BaseType_t task_code = xTaskCreatePinnedToCore(handle_interrupt_task, "handle interrupt", 8196, lora_device, 2, &handle_interrupt, xPortGetCoreID());
    if (task_code != pdPASS)
    {
        ESP_LOGE(TAG, "can't create task %d", task_code);
        sx127x_destroy(lora_device);
        return;
    }

    gpio_set_direction((gpio_num_t)LORA_DIO0_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en((gpio_num_t)LORA_DIO0_PIN);
    gpio_pullup_dis((gpio_num_t)LORA_DIO0_PIN);
    gpio_set_intr_type((gpio_num_t)LORA_DIO0_PIN, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)LORA_DIO0_PIN, handle_interrupt_fromisr, (void *)lora_device);

    // 4 is OK
    ESP_ERROR_CHECK(sx127x_set_pa_config(SX127x_PA_PIN_BOOST, 4, lora_device));

    init_lcd(0);

    lcd_send_string(0, "Csovii");

    init_gps();

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
