#include "lora.h"

static const char TAG[] = "LoRa";

const uint8_t lora_device_addr = 0x01;

sx127x *lora_device = NULL;
spi_device_handle_t lora_spi_device;
TaskHandle_t lora_interrupt_handler;
uint8_t lora_rx_buff[2048];
uint8_t lora_tx_buff[2048];


typedef struct  {
    uint16_t src_device_addr;
    uint16_t dest_device_addr;
    uint8_t num_of_packets;
    uint8_t packet_num;
    uint8_t payload[514];
} LoRa_Packet;


void init_lora() {
    spi_device_interface_config_t dev_cfg = {
            .clock_speed_hz = 100000,
            .spics_io_num = LORA_SS_PIN,
            .queue_size = 16,
            .command_bits = 0,
            .address_bits = 8,
            .dummy_bits = 0,
            .mode = 0};
    spi_device_handle_t spi_device;
    ESP_ERROR_CHECK(spi_bus_add_device(LORA_SPI_HOST, &dev_cfg, &spi_device));
    ESP_ERROR_CHECK(sx127x_create(spi_device, &lora_device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, lora_device));
    ESP_ERROR_CHECK(sx127x_set_frequency(437200012, lora_device));
    ESP_ERROR_CHECK(sx127x_reset_fifo(lora_device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, lora_device));
    ESP_ERROR_CHECK(sx127x_set_bandwidth(SX127x_BW_125000, lora_device));
    ESP_ERROR_CHECK(sx127x_set_implicit_header(NULL, lora_device));
    ESP_ERROR_CHECK(sx127x_set_modem_config_2(SX127x_SF_9, lora_device));
    ESP_ERROR_CHECK(sx127x_set_syncword(18, lora_device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, lora_device));
    sx127x_set_tx_callback(tx_callback, lora_device);
    sx127x_set_rx_callback(rx_callback, lora_device);
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_device));
    ESP_ERROR_CHECK(sx127x_set_pa_config(SX127x_PA_PIN_BOOST, 4, lora_device));



    BaseType_t task_code = xTaskCreatePinnedToCore(handle_interrupt_task, "handle interrupt", 8196, lora_device, 2, &lora_interrupt_handler, xPortGetCoreID());
    if (task_code != pdPASS)
    {
        ESP_LOGE(TAG, "can't create task %d", task_code);
        sx127x_destroy(lora_device);
        return;
    }
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
}