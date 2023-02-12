#include "lora.h"

static const char TAG[] = "LoRa";

// device can only be accessed by one task or interrupt at a time
SemaphoreHandle_t xLoraMutex;
// Rx buff mutex is not needed because the rx_callback dispatches the tasks
// TX buffer is not needed because the task will send data when it receives a packet from queue
QueueHandle_t lora_tx_queue;

sx127x *lora_device = NULL;
spi_device_handle_t lora_spi_device;
TaskHandle_t lora_interrupt_handler;

// When a packet needs to be sent, only put in the rx buffer
// The sender task sends the packets
TaskHandle_t lora_packet_sender_handler;
LoRa_Packet lora_rx_buff[20];


void IRAM_ATTR lora_handle_interrupt_fromisr(void *arg)
{
    xTaskResumeFromISR(lora_interrupt_handler);
}

void init_lora() {
    spi_device_interface_config_t dev_cfg = {
            .clock_speed_hz = 100000,
            .spics_io_num = LORA_SS_PIN,
            .queue_size = 16,
            .command_bits = 0,
            .address_bits = 8,
            .dummy_bits = 0,
            .mode = 0};
    ESP_ERROR_CHECK(spi_bus_add_device(LORA_SPI_HOST, &dev_cfg, &lora_spi_device));
    ESP_ERROR_CHECK(sx127x_create(lora_spi_device, &lora_device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, lora_device));
    ESP_ERROR_CHECK(sx127x_set_frequency(437200012, lora_device));
    ESP_ERROR_CHECK(sx127x_reset_fifo(lora_device));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, lora_device));
    ESP_ERROR_CHECK(sx127x_set_bandwidth(SX127x_BW_125000, lora_device));
    ESP_ERROR_CHECK(sx127x_set_implicit_header(NULL, lora_device));
    ESP_ERROR_CHECK(sx127x_set_modem_config_2(SX127x_SF_7, lora_device));
    ESP_ERROR_CHECK(sx127x_set_syncword(18, lora_device));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, lora_device));
    sx127x_set_tx_callback(tx_callback, lora_device);
    sx127x_set_rx_callback(rx_callback, lora_device);

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_device));
    ESP_ERROR_CHECK(sx127x_set_pa_config(SX127x_PA_PIN_BOOST, 4, lora_device));

    BaseType_t task_code = xTaskCreatePinnedToCore(handle_interrupt_task, "handle interrupt", 8196, lora_device, 100, &lora_interrupt_handler, xPortGetCoreID());
    if (task_code != pdPASS)
    {
        ESP_LOGE(TAG, "can't create task %d", task_code);
        sx127x_destroy(lora_device);
        return;
    }

    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)LORA_DIO0_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)LORA_DIO0_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)LORA_DIO0_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)LORA_DIO0_PIN, GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(
            gpio_isr_handler_add((gpio_num_t) LORA_DIO0_PIN, lora_handle_interrupt_fromisr, (void *) lora_device));

    xLoraMutex = xSemaphoreCreateMutex();
    lora_tx_queue = xQueueCreate(10, sizeof(LoRa_Packet));
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
    printf("Transmitted\n");
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

// Calculates CRC and sends packet
uint8_t lora_send_packet(sx127x *lora_dev, LoRa_Packet* packet){
    uint8_t data[255];
    memset(&data[0], packet->src_device_addr, sizeof(uint8_t));
    memset(&data[1], packet->dest_device_addr, sizeof(uint8_t));
    memset(&data[2], packet->num_of_packets, sizeof(uint8_t));
    memset(&data[3], packet->packet_num, sizeof(uint8_t));
    memset(&data[4], packet->payload_size, sizeof(uint8_t));
    memset(&data[5], packet->header_crc, sizeof(uint16_t));
    memcpy(&data[7], packet->payload, packet->payload_size);
    memset(&data[7 + packet->payload_size], packet->payload_crc, sizeof(uint16_t));

    if (xSemaphoreTake(xLoraMutex, portMAX_DELAY) == pdTRUE){
        ESP_ERROR_CHECK(sx127x_set_for_transmission(data, packet->payload_size + 10, lora_dev));
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, lora_dev));
        ESP_LOGI(TAG, "transmitting packet");
        ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_dev));

        xSemaphoreGive(xLoraMutex);
    }
}

