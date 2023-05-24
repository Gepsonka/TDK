
#include "lora.h"

static const char TAG[] = "LoRa";

// device can only be accessed by one task or interrupt at a time
SemaphoreHandle_t xLoraMutex;
// Rx buff mutex is not needed because the rx_callback dispatches the tasks
// TX buffer is not needed because the task will send data when it receives a packet from queue
QueueHandle_t lora_tx_queue;
SemaphoreHandle_t xLoraTXQueueMutex;
struct sx127x_t *lora_device;
spi_device_handle_t lora_spi_device;
TaskHandle_t lora_interrupt_handler;
// When a packet needs to be sent, only put in the rx buffer
// The sender task sends the packets
TaskHandle_t lora_packet_sender_handler;


uint16_t lora_calc_header_crc(LoRa_Packet_Header* header){
    uint8_t header_arr[5] = {header->src_device_addr, header->dest_device_addr,
                             header->num_of_packets, header->packet_num,
                             header->payload_size};

    return crc16_be(0, header_arr, 5);
}

uint16_t lora_calc_packet_crc(LoRa_Packet_Payload* payload, uint8_t payload_length){
    return crc16_be(0, payload->payload, payload_length);
}

void lora_display_packet(LoRa_Packet* packet_to_display){
    printf("Printing packet...\n");
    printf("\tHeader:\n");
    printf("\t\tSource device address: %d\n", packet_to_display->header.src_device_addr);
    printf("\t\tDestination device address: %d\n", packet_to_display->header.dest_device_addr);
    printf("\t\tNumber of Packets: %d\n", packet_to_display->header.num_of_packets);
    printf("\t\tPacket number: %d\n", packet_to_display->header.packet_num);
    printf("\t\tPayload size: %d\n", packet_to_display->header.payload_size);
    printf("\t\tHeader CRC: %d\n", packet_to_display->header.payload_size);
    printf("\tPayload:\n");
    printf("\t\t");
    for (uint8_t i = 0; i < packet_to_display->header.payload_size; i++) {
        printf("%#X ", packet_to_display->payload.payload[i]);
    }

    printf("\tPayload CRC:\n");
    printf("\t\t%d\n", packet_to_display->payload.payload_crc);
}

void IRAM_ATTR lora_handle_interrupt_fromisr(void *arg)
{
    xTaskResumeFromISR(lora_interrupt_handler);
}

void init_lora(spi_device_handle_t* spi_device, sx127x* lora_dev) {
    spi_device_interface_config_t dev_cfg = {
            .clock_speed_hz = 100000,
            .spics_io_num = LORA_SS_PIN,
            .queue_size = 16,
            .command_bits = 0,
            .address_bits = 8,
            .dummy_bits = 0,
            .mode = 0
    };

    ESP_ERROR_CHECK(spi_bus_add_device(LORA_SPI_HOST, &dev_cfg, spi_device));
    //spi_device_acquire_bus(lora_spi_device, portMAX_DELAY);
    ESP_ERROR_CHECK(sx127x_create(*spi_device, &lora_dev));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_SLEEP, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_frequency(437200012, lora_dev));
    ESP_ERROR_CHECK(sx127x_reset_fifo(lora_dev));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_STANDBY, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_bandwidth(SX127x_BW_500000, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_implicit_header(NULL, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_modem_config_2(SX127x_SF_7, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_syncword(18, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_preamble_length(8, lora_dev));
    sx127x_set_tx_callback(tx_callback, lora_dev);
    sx127x_set_rx_callback(rx_callback, lora_dev);

    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_dev));
    ESP_ERROR_CHECK(sx127x_set_pa_config(SX127x_PA_PIN_BOOST, 4, lora_dev));
    //spi_device_release_bus(lora_spi_device);
    BaseType_t task_code = xTaskCreatePinnedToCore(handle_interrupt_task, "handle interrupt", 8196, lora_dev, 100, &lora_interrupt_handler, xPortGetCoreID());
    if (task_code != pdPASS)
    {
        ESP_LOGE(TAG, "can't create task %d", task_code);
        sx127x_destroy(lora_dev);
        return;
    }

    ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)LORA_DIO0_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_pulldown_en((gpio_num_t)LORA_DIO0_PIN));
    ESP_ERROR_CHECK(gpio_pullup_dis((gpio_num_t)LORA_DIO0_PIN));
    ESP_ERROR_CHECK(gpio_set_intr_type((gpio_num_t)LORA_DIO0_PIN, GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(
            gpio_isr_handler_add((gpio_num_t) LORA_DIO0_PIN, lora_handle_interrupt_fromisr, (void *) lora_dev));

    xLoraMutex = xSemaphoreCreateMutex();
    xLoraTXQueueMutex = xSemaphoreCreateMutex();
    lora_tx_queue = xQueueCreate(50, sizeof(LoRa_Packet));
    BaseType_t sender_task_code = xTaskCreatePinnedToCore(lora_packet_sender_task, "PacketSenderTask", 5120, lora_dev, 2, &lora_packet_sender_handler, 1);
    if (sender_task_code != pdPASS)
    {
        ESP_LOGE(TAG, "can't create sender task %d", task_code);
        sx127x_destroy(lora_dev);
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
    //ESP_LOGI(TAG, "transmitted");
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

void lora_packet_sender_task(void* pvParameters) {
    sx127x* lora_dev = (sx127x*) pvParameters;
    LoRa_Packet packet_to_send;
    // Optimization: if there are messages which has packet left to send
    // the program does not set the LoRa mode back to receiving until every message has been sent
    // which is indicated by the 0 value
    uint8_t messages_unfinished = 0;
    uint8_t mode = SX127x_MODE_RX_CONT;
    uint8_t lora_mutex_is_held_by_task = 0;
    while (1) {
        if( xQueueReceive(lora_tx_queue, &packet_to_send, portMAX_DELAY) == pdPASS )
        {
            if ( lora_mutex_is_held_by_task || xSemaphoreTake(xLoraMutex, portMAX_DELAY) == pdTRUE) {
                lora_mutex_is_held_by_task = 1;
                // ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_dev));

                ESP_ERROR_CHECK(lora_send_packet(lora_dev, &packet_to_send));
                // Indicating the last packet of the message
                if (packet_to_send.header.packet_num == packet_to_send.header.num_of_packets - 1 &&
                    packet_to_send.header.num_of_packets != 1) {
                    messages_unfinished--;
                }

                // Indicating start of the message
                if (packet_to_send.header.packet_num == 0 && packet_to_send.header.num_of_packets != 1) {
                    messages_unfinished++;
                }

//                // Indicating all messages has been sent
//                if (messages_unfinished == 0) {
//                    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_dev));
//                    mode = SX127x_MODE_RX_CONT;
//                    xSemaphoreGive(xLoraMutex);
//                    lora_mutex_is_held_by_task = 0;
//                }

                //lora_display_packet(&packet_to_send);

                xSemaphoreGive(xLoraMutex);
                lora_mutex_is_held_by_task = 0;

            }
        } else { // if packet are not received for a long period of time, reset state
            if (mode != SX127x_MODE_RX_CONT) {
                ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_RX_CONT, lora_dev));
                ESP_LOGI("LoRa", "LoRa set back to rx");
                mode = SX127x_MODE_RX_CONT;
            }

            if (lora_mutex_is_held_by_task == 1) {
                xSemaphoreGive(xLoraMutex);
                lora_mutex_is_held_by_task = 0;
            }
        }
    }
}




// Calculates CRC and sends packet
uint8_t lora_send_packet(sx127x *lora_dev, LoRa_Packet* packet){
    uint8_t data[255];
    memset(&data[0], packet->header.src_device_addr, sizeof(uint8_t));
    memset(&data[1], packet->header.dest_device_addr, sizeof(uint8_t));
    memset(&data[2], packet->header.num_of_packets, sizeof(uint8_t));
    memset(&data[3], packet->header.packet_num, sizeof(uint8_t));
    memset(&data[4], packet->header.payload_size, sizeof(uint8_t));
    memset(&data[5], packet->header.header_crc >> 8, sizeof(uint8_t));
    memset(&data[6], packet->header.header_crc & 0xFF, sizeof(uint8_t));
    memset(&data[7], packet->payload.payload_crc >> 8, sizeof(uint8_t));
    memset(&data[8], packet->payload.payload_crc & 0xFF, sizeof(uint8_t));
    memcpy(&data[9], packet->payload.payload, packet->header.payload_size);
    spi_device_acquire_bus(lora_spi_device, portMAX_DELAY);
    ESP_ERROR_CHECK(sx127x_set_for_transmission(data, packet->header.payload_size + sizeof(LoRa_Packet_Header) - 1 + sizeof(uint16_t), lora_dev));
    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, lora_dev));
    spi_device_release_bus(lora_spi_device);

    return 0;
}

uint8_t lora_send_message(uint8_t src_addr, uint8_t dest_addr, uint8_t* message, uint8_t message_len) {
    LoRa_Packet* lora_packets_buff;

    uint8_t num_of_full_packets = message_len / LORA_PAYLOAD_MAX_SIZE;
    uint8_t remaining_packet_size = message_len % LORA_PAYLOAD_MAX_SIZE;

    uint8_t num_of_packets = num_of_full_packets;
    if (remaining_packet_size != 0) {
        num_of_packets++;
    }

    lora_packets_buff = (LoRa_Packet*) malloc(sizeof(LoRa_Packet) * num_of_packets);
    if (lora_packets_buff == NULL) {
        ESP_LOGE(TAG, "Unable to allocate memory for packets.");
        return MESSAGE_NOT_ENOUGH_MEMORY;
    }

    for (uint8_t i = 0; i < num_of_packets; i++){
        LoRa_Packet packet;
        if (message_len / LORA_PAYLOAD_MAX_SIZE == 0) {
            memcpy(packet.payload.payload, &(message[i * LORA_PAYLOAD_MAX_SIZE]), remaining_packet_size);
            packet.header.payload_size = remaining_packet_size;
        } else {
            memcpy(packet.payload.payload, &(message[i * LORA_PAYLOAD_MAX_SIZE]), LORA_PAYLOAD_MAX_SIZE);
            packet.header.payload_size = LORA_PAYLOAD_MAX_SIZE;
        }

        packet.header.src_device_addr = src_addr;
        packet.header.dest_device_addr = dest_addr;
        packet.header.num_of_packets = num_of_packets;
        packet.header.packet_num = i;
        lora_calc_packet_crc(&(packet.payload), packet.header.payload_size);
        lora_calc_header_crc(&(packet.header));
        message_len -= LORA_PAYLOAD_MAX_SIZE;

        // Do not send immediately, needed to cache the whole message for the networking
        // and error correcting part
        lora_packets_buff[i] = packet;

        xQueueSend(lora_tx_queue, (void*) &packet, portMAX_DELAY);

    }
    free(lora_packets_buff);

    return MESSAGE_OK;
}
