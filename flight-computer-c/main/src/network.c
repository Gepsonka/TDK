//
// Created by molnar on 2023.02.14..
//
#include "network.h"

static const char TAG[] = "LoRa";

sx127x *lora_device;

// device can only be accessed by one task or interrupt at a time
SemaphoreHandle_t xLoraMutex;
// Rx buff mutex is not needed because the rx_callback dispatches the tasks
// TX buffer is not needed because the task will send data when it receives a packet from queue
QueueHandle_t lora_tx_queue;
SemaphoreHandle_t xLoraTXQueueMutex;

spi_device_handle_t lora_spi_device;
TaskHandle_t lora_interrupt_handler;
// When a packet needs to be sent, only put in the rx buffer
// The sender task sends the packets
TaskHandle_t lora_packet_sender_handler;
LoRa_Packet lora_rx_buff[20];

Network_Device_Container device_container;

TaskHandle_t network_rx_packet_handler;
QueueHandle_t packet_rx_queue;
TaskHandle_t network_device_processor_handler;
QueueHandle_t network_device_processor_queue;

QueueHandle_t device_queue;

extern SemaphoreHandle_t joystick_semaphore_handle;

extern SemaphoreHandle_t lcd_mutex;

extern RTLG_Status RTLG_status;
extern SemaphoreHandle_t RTLG_status_mutex;

static bool network_is_device_in_arp(Network_Device_Container* dev_container, uint8_t dev_addr) {
    for (uint8_t i = 0; i < dev_container->num_of_devices; i++) {
        if (dev_container->device_contexts[i].address == dev_addr) {
            return true;
        }
    }


    return false;
}

static Network_Device_Context* get_device_from_arp(Network_Device_Container* dev_container, uint8_t dev_addr) {
    for (uint8_t i = 0; i < dev_container->num_of_devices; i++) {
        if (dev_container->device_contexts[i].address == dev_addr) {
            return &dev_container->device_contexts[i];
        }
    }

    return NULL;
}

static uint8_t build_packet_from_bytes(LoRa_Packet* packet, uint8_t* raw_data, uint8_t raw_data_size){
    if (raw_data_size <= 9 ) {
        return 1; // Error, packet cannot be empty, or have missing header parameters
    }

    packet->header.src_device_addr = raw_data[0];
    packet->header.dest_device_addr = raw_data[1];
    packet->header.num_of_packets = raw_data[2];
    packet->header.packet_num = raw_data[3];
    packet->header.payload_size = raw_data[4];
    packet->header.header_crc = ((uint16_t)raw_data[5] << 8) | raw_data[6];
    packet->payload.payload_crc = ((uint16_t)raw_data[7] << 8) | raw_data[8];
    memcpy(packet->payload.payload, &raw_data[9], raw_data_size - 9);

    return 0;
}

static void display_packet(LoRa_Packet* packet_to_display){
    printf("Printing packet...\n");
    printf("\tHeader:\n");
    printf("\t\tSource device address: %d\n", packet_to_display->header.src_device_addr);
    printf("\t\tDestination device address: %d\n", packet_to_display->header.dest_device_addr);
    printf("\t\tNumber of packets: %d\n", packet_to_display->header.num_of_packets);
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
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
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

void network_init(Network_Device_Container* device_cont)
{
    device_cont->device_contexts = NULL;
    device_cont->num_of_devices = 0;
    network_add_device(device_cont, 0x00);

    Network_Device_Context* device_ctx = get_device_from_arp(device_cont, 0x00);
    device_ctx->status = ONLINE;
    packet_rx_queue = xQueueCreate(50, sizeof(LoRa_Packet));
    device_queue = xQueueCreate(15, sizeof(uint8_t));

    // TODO: Check tasks for safety purposes and create task handles for them
    xTaskCreate(network_packet_rx_handler_task, "PacketReceiveTask", 4096, &device_container, 1, &network_rx_packet_handler);
    network_device_processor_queue = xQueueCreate(20, sizeof(uint8_t));
    xTaskCreate(network_packet_rx_handler_task, "DeviceProcessorTask", 4096, &device_container, 1, &network_device_processor_handler);
    xTaskCreate(network_packet_processor_task, "PacketProcessorTask", 5120, &device_container, 1, NULL);
    xTaskCreate(network_device_processor_task, "PacketProcessorTask", 5120, &device_container, 1, NULL);
    ESP_LOGI("Network", "Network init finished.");

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
    ESP_LOGI(TAG, "Transmitted");
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

    LoRa_Packet packet_received;
    build_packet_from_bytes(&packet_received, data, data_length);
    xQueueSend(packet_rx_queue, &packet_received, portMAX_DELAY);

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
        if( xQueueReceive(lora_tx_queue, &packet_to_send, 1000) == pdPASS )
        {
            if ( lora_mutex_is_held_by_task || xSemaphoreTake(xLoraMutex, portMAX_DELAY) == pdTRUE) {
                lora_mutex_is_held_by_task = 1;
                if (mode != SX127x_MODE_TX) {
                    //spi_device_polling_end(lora_spi_device, portMAX_DELAY);
                    ESP_ERROR_CHECK(sx127x_set_opmod(SX127x_MODE_TX, lora_dev));
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    ESP_LOGI("LoRa", "Device set to tx...");
                    mode = SX127x_MODE_TX;
                }

                //lora_display_packet(&packet_to_send);

                // TODO: Put a while (spi_device_is_polling_transaction(spi)) here to
                // check when the spi bs is available
                // otherwise the lora_send_packet will throw spi error
                //lora_display_packet(&packet_to_send);

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


// Sends packet
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
    ESP_LOGI(TAG, "Sent packet...");
    return 0;
}

void network_packet_processor_task(void* pvParameters){
    Network_Device_Container* dev_ctnr = (Network_Device_Container*) pvParameters;
    LoRa_Packet packet;
    Network_Device_Context* device_ctx;
    // TODO: implement required security features later...
    while (1) {
        if( xQueueReceive(packet_rx_queue, &packet, portMAX_DELAY) == pdPASS ) {
            device_ctx = get_device_from_arp(dev_ctnr, packet.header.src_device_addr);
            if (device_ctx == NULL) {
                continue;
            }

            // not safe yet!
            if (packet.header.packet_num == 0) {
                if (device_ctx->packet_rx_buff != NULL) {
                    free(device_ctx->packet_rx_buff);
                }

                device_ctx->packet_rx_buff = (LoRa_Packet*) malloc(packet.header.num_of_packets * sizeof(LoRa_Packet));
                device_ctx->packet_rx_buff[0] = packet;
            } else {
                device_ctx->packet_rx_buff[packet.header.packet_num] = packet;
            }

            if (packet.header.packet_num == packet.header.num_of_packets - 1) {
                xQueueSend(device_queue, &packet.header.src_device_addr, portMAX_DELAY);
            }
        }
    }
}


void network_device_processor_task(void* pvParameters){
    Network_Device_Container* dev_ctnr = (Network_Device_Container*) pvParameters;
    uint8_t dev_addr;
    Network_Device_Context* device_ctx;
    RTLG_Status prev_state_of_RTLG_status = EXTRACTED;

    while (1) {
        if( xQueueReceive(device_queue, &dev_addr, portMAX_DELAY) == pdPASS ) {
            device_ctx = get_device_from_arp(dev_ctnr, dev_addr);
            if (device_ctx == NULL) {
                ESP_LOGE(TAG, "device with address %#X does not exist in ARP.", dev_addr);
                continue;
            }

            construct_message_from_packets(device_ctx);

            printf("\n\nalerion percentage: %d\nelevator precentage: %d\nrudder percentage: %d\nmotor percentage: %d\n\n",
                   device_ctx->rx_secret_message[0],
                   device_ctx->rx_secret_message[1],
                   device_ctx->rx_secret_message[2],
                   device_ctx->rx_secret_message[3]
                   );

            servo_set_ailerons_servo_by_joystick_percentage((int8_t) device_ctx->rx_secret_message[0]);
            servo_set_elevator_servo_by_joystick_percentage((int8_t) device_ctx->rx_secret_message[1]);
            servo_set_rudder_servo_by_joystick_percentage((int8_t) device_ctx->rx_secret_message[2]);
            motor_set_motor_speed(motor_get_duty_value_from_percentage(device_ctx->rx_secret_message[3]));
            if (xSemaphoreTake(RTLG_status_mutex, portMAX_DELAY) == pdPASS) {
                if (device_ctx->rx_secret_message[4] != prev_state_of_RTLG_status) {
                    prev_state_of_RTLG_status = device_ctx->rx_secret_message[4];
                    RTLG_status = device_ctx->rx_secret_message[4];
                    servo_set_RTLG_status(device_ctx->rx_secret_message[4]);
                }
                xSemaphoreGive(RTLG_status_mutex);
            }

        }
    }
}

void network_packet_rx_handler_task(void* pvParameters){
    Network_Device_Container* dev_cntr = (Network_Device_Container*) pvParameters;
    LoRa_Packet received_packet;
    Network_Device_Context* packet_device_ctx;

    while (1) {
        if( xQueueReceive(packet_rx_queue, &received_packet, portMAX_DELAY) == pdPASS ) {
            // check if the packet was addressed to this device
            if (received_packet.header.dest_device_addr != LORA_BASE_STATION_ADDR) {
                continue;
            }

            // check if device is in ARP, if not add a new device with status OFFLINE, start copy packet to its rx buff
            // only if this is the first packet of the device, else throw out packet
            if (!network_is_device_in_arp(dev_cntr, received_packet.header.src_device_addr) &&
                received_packet.header.packet_num == 0)
            {
                network_add_device(dev_cntr, received_packet.header.src_device_addr);
            } else if (!network_is_device_in_arp(dev_cntr, received_packet.header.src_device_addr) &&
                       received_packet.header.packet_num != 0)
            {
                continue;
                // TODO: send back some type of error message
            }

            packet_device_ctx = get_device_from_arp(dev_cntr, received_packet.header.src_device_addr);

            // checking the packet indexing
            if (received_packet.header.packet_num > received_packet.header.num_of_packets - 1) {
                continue;
                // TODO: send back error
            }

            // the procedure is different when the station is waiting for corrected devices
            if (packet_device_ctx->connection_status == WAITING_FOR_PACKET_CORRECTION) {
                packet_device_ctx->packet_rx_buff[received_packet.header.packet_num] = received_packet;

                // last corrected packet
                if (received_packet.header.packet_num == packet_device_ctx->packet_num_of_faulty_packets[packet_device_ctx->num_of_faulty_packets - 1]) {
                    // TODO: pass device to the device processor
                }
            } else {
                if (received_packet.header.packet_num == 0) {
                    network_free_device_network_rx_buff(packet_device_ctx);
                    packet_device_ctx->packet_rx_buff = (LoRa_Packet*) malloc(received_packet.header.num_of_packets * sizeof(LoRa_Packet));
                }

                packet_device_ctx->packet_rx_buff[received_packet.header.packet_num] = received_packet;

                if (received_packet.header.packet_num == received_packet.header.num_of_packets - 1){
                    // TODO: pass device to device processor
                }
            }
        }
    }
}




network_operation_t network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr)
{
    Network_Device_Context new_device;
    new_device.address = dev_addr;
    new_device.status = ADDING_DEVICE_TO_NETWORK;
    new_device.connection_status = CONNECTION_ESTABLISHED;
    new_device.cipher_text = NULL;
    new_device.tx_secret_message = NULL;
    new_device.tx_secret_message_size = 0;
    new_device.rx_secret_message = NULL;
    new_device.rx_secret_message_size = 0;
    new_device.tx_message = NULL;
    new_device.tx_message_size = 0;
    new_device.rx_message = NULL;
    new_device.rx_message_size = 0;
    new_device.packet_rx_buff = NULL;
    new_device.packet_tx_buff = NULL;
    new_device.packet_num_of_faulty_packets = NULL;
    new_device.num_of_faulty_packets = 0;

    if (device_cont->num_of_devices == 0) {
        device_cont->num_of_devices++;
        device_cont->device_contexts = (Network_Device_Context*) malloc(sizeof(Network_Device_Context));

    } else {
        device_cont->num_of_devices++;
        device_cont->device_contexts = (Network_Device_Context*) realloc(device_cont->device_contexts, device_cont->num_of_devices * sizeof(Network_Device_Context));
    }

    if (device_cont->device_contexts == NULL) {
        return NETWORK_OUT_OF_MEMORY;
    }

    device_cont->device_contexts[device_cont->num_of_devices - 1] = new_device;

    return NETWORK_OK;
}

uint8_t check_packet_crc(LoRa_Packet* packet){
    uint16_t header_crc;
    uint16_t payload_crc;

    header_crc = lora_calc_header_crc(&packet->header);
    payload_crc = lora_calc_packet_crc(&packet->payload, packet->header.payload_size);
    if (header_crc != packet->header.header_crc || payload_crc != packet->payload.payload_crc){
        return 1;
    }


    return 0;
}


uint8_t construct_message_from_packets(Network_Device_Context* device_ctx){
    // TODO: construct package only into rx_message, same when deconstructing
    if (device_ctx->packet_rx_buff == NULL) {
        ESP_LOGE(TAG, "Network rx buffer empty");
        return NETWORK_BUFFER_EMPTY_ERROR;
    }

    // free both message rx buffers
    if (device_ctx->rx_secret_message != NULL) {
        free(device_ctx->rx_secret_message);
        device_ctx->rx_secret_message = NULL; // for safety
        device_ctx->rx_secret_message_size = 0;
    }

    if (device_ctx->rx_message != NULL) {
        free(device_ctx->rx_message);
        device_ctx->rx_message = NULL; // for safety
        device_ctx->rx_message_size = 0;
    }

    // if device is online decryption is needed so message gets copied into
    // the rx_secret_message buffer
    if (device_ctx->status == ONLINE){
        // calculate message length
        for (uint8_t i = 0; i < device_ctx->packet_rx_buff[0].header.num_of_packets; i++) {
            device_ctx->rx_secret_message_size += device_ctx->packet_rx_buff->header.payload_size;
        }



        device_ctx->rx_secret_message = (uint8_t*) malloc(device_ctx->rx_secret_message_size * sizeof(uint8_t));

        // test it thoroughly!
        uint8_t rx_secret_message_buff_index = 0;
        for (uint8_t i = 0; i < device_ctx->packet_rx_buff[0].header.num_of_packets; i++) {
            memcpy(&device_ctx->rx_secret_message[rx_secret_message_buff_index],
                   device_ctx->packet_rx_buff[i].payload.payload,
                   device_ctx->packet_rx_buff[i].header.payload_size);

            rx_secret_message_buff_index += (LORA_PAYLOAD_MAX_SIZE + 1);
        }



    } else { // message gets copied into rx_message buffer
        for (uint8_t i = 0; i < device_ctx->packet_rx_buff[0].header.num_of_packets; i++) {
            device_ctx->rx_message_size += device_ctx->packet_rx_buff->header.payload_size;
        }

        device_ctx->rx_message = (uint8_t*) malloc(device_ctx->rx_message_size * sizeof(uint8_t));

        // test it thoroughly!
        uint8_t rx_secret_message_buff_index = 0;
        for (uint8_t i = 0; i < device_ctx->packet_rx_buff[0].header.num_of_packets; i++) {
            memcpy(&device_ctx->rx_message[rx_secret_message_buff_index],
                   device_ctx->packet_rx_buff[i].payload.payload,
                   device_ctx->packet_rx_buff[i].header.payload_size);

            rx_secret_message_buff_index += (LORA_PAYLOAD_MAX_SIZE);
        }
    }


    if (device_ctx->packet_rx_buff != NULL) {
        free(device_ctx->packet_rx_buff);
        device_ctx->packet_rx_buff = NULL;
    }

    ESP_LOGI(TAG, "Secret message size: %d", device_ctx->rx_secret_message_size);

    return NETWORK_OK;
}


uint8_t deconstruct_message_into_packets(Network_Device_Context *device_ctx) {
    if (device_ctx->packet_tx_buff != NULL) {
        free(device_ctx->packet_tx_buff);
        device_ctx->packet_tx_buff = NULL;
    }

    if (device_ctx->status == ONLINE) { // device is authenticated, only encrypted message is accepted
        uint8_t num_of_packets = device_ctx->tx_secret_message_size / LORA_PAYLOAD_MAX_SIZE +
                                 ((device_ctx->tx_secret_message_size % LORA_PAYLOAD_MAX_SIZE) != 0);

        uint8_t last_packet_payload_size = device_ctx->tx_secret_message_size % LORA_PAYLOAD_MAX_SIZE;
        device_ctx->packet_tx_buff = (LoRa_Packet*) malloc(num_of_packets * sizeof(LoRa_Packet));
        if (device_ctx->packet_tx_buff == NULL) {
            ESP_LOGE("Network", "Mem allocation for tx has failed");
            return NETWORK_OUT_OF_MEMORY;
        }

        // test thoroughly!!
        for (uint8_t i = 0; i < num_of_packets; i++) {
            device_ctx->packet_tx_buff[i].header.num_of_packets = num_of_packets;
            device_ctx->packet_tx_buff[i].header.payload_size = (i == (num_of_packets - 1)) ? last_packet_payload_size : LORA_PAYLOAD_MAX_SIZE;
            device_ctx->packet_tx_buff[i].header.dest_device_addr = device_ctx->address;
            device_ctx->packet_tx_buff[i].header.src_device_addr = LORA_BASE_STATION_ADDR;
            device_ctx->packet_tx_buff[i].header.packet_num = i;
            if (i == num_of_packets - 1) { // last packet
                memcpy(device_ctx->packet_tx_buff[i].payload.payload, &device_ctx->tx_secret_message[i * LORA_PAYLOAD_MAX_SIZE], last_packet_payload_size);
            } else {
                memcpy(device_ctx->packet_tx_buff[i].payload.payload, &device_ctx->tx_secret_message[i * LORA_PAYLOAD_MAX_SIZE], LORA_PAYLOAD_MAX_SIZE);
            }
            // calc CRCs
            device_ctx->packet_tx_buff[i].header.header_crc = lora_calc_header_crc(&device_ctx->packet_tx_buff[i].header);
            device_ctx->packet_tx_buff[i].payload.payload_crc = lora_calc_packet_crc(&device_ctx->packet_tx_buff[i].payload, device_ctx->packet_tx_buff[i].header.payload_size);
        }

    } else {
        uint8_t num_of_packets = device_ctx->tx_message_size / LORA_PAYLOAD_MAX_SIZE +
                                 (device_ctx->tx_message_size / LORA_PAYLOAD_MAX_SIZE != 0);
        uint8_t last_packet_payload_size = device_ctx->tx_message_size % LORA_PAYLOAD_MAX_SIZE;

        device_ctx->packet_tx_buff = (LoRa_Packet*) malloc(num_of_packets * sizeof(LoRa_Packet));
        if (device_ctx->packet_tx_buff == NULL) {
            return NETWORK_OUT_OF_MEMORY;
        }

        for (uint8_t i = 0; i < num_of_packets; i++) {
            device_ctx->packet_tx_buff[i].header.num_of_packets = num_of_packets;
            device_ctx->packet_tx_buff[i].header.payload_size = i == num_of_packets - 1 ? last_packet_payload_size : LORA_PAYLOAD_MAX_SIZE;
            device_ctx->packet_tx_buff[i].header.dest_device_addr = device_ctx->address;
            device_ctx->packet_tx_buff[i].header.src_device_addr = LORA_BASE_STATION_ADDR;
            device_ctx->packet_tx_buff[i].header.packet_num = i;
            if (i == num_of_packets - 1) {
                memcpy(device_ctx->packet_tx_buff[i].payload.payload, &device_ctx->tx_message[i * LORA_PAYLOAD_MAX_SIZE], last_packet_payload_size);
            } else {
                memcpy(device_ctx->packet_tx_buff[i].payload.payload, &device_ctx->tx_message[i * LORA_PAYLOAD_MAX_SIZE], LORA_PAYLOAD_MAX_SIZE);
            }
            // calc CRCs
            device_ctx->packet_tx_buff[i].header.header_crc = lora_calc_header_crc(&device_ctx->packet_tx_buff[i].header);
            device_ctx->packet_tx_buff[i].payload.payload_crc = lora_calc_packet_crc(&device_ctx->packet_tx_buff[i].payload, device_ctx->packet_tx_buff[i].header.payload_size);
        }
    }
    return NETWORK_OK;
}

void set_packets_for_tx(Network_Device_Context* device_ctx, QueueHandle_t* lora_tx_queue_ptr) {
    if (device_ctx->packet_tx_buff == NULL) {
        return;
    }

    for (uint8_t i = 0; i < device_ctx->packet_tx_buff->header.num_of_packets; i++) {
        //display_packet(&device_ctx->packet_tx_buff[i]);
        if (xQueueSend(*lora_tx_queue_ptr, &device_ctx->packet_tx_buff[i], portMAX_DELAY) != pdPASS) {
            ESP_LOGE("packet setup", "Could not send packet to queue");
        }
    }
}

void network_encrypt_device_message(Network_Device_Context* device_ctx){
    if (device_ctx->tx_secret_message != NULL) {
        free(device_ctx->tx_secret_message);
        device_ctx->tx_secret_message_size = 0;
    }

    device_ctx->tx_secret_message = (uint8_t*) malloc(device_ctx->tx_message_size * sizeof(uint8_t));
    device_ctx->tx_secret_message_size = device_ctx->tx_message_size;

    aes_gcm_encrypt(
            device_ctx->aes_key,
            device_ctx->init_vector,
            device_ctx->tx_message,
            device_ctx->tx_message_size,
            device_ctx->aad,
            device_ctx->tx_secret_message,
            device_ctx->auth_tag
    );
}


network_operation_t network_decrypt_device_message(Network_Device_Context* device_ctx){
    uint8_t message_auth_tag[SECURITY_AUTH_TAG_SIZE];

    if (device_ctx->rx_message != NULL) {
        free(device_ctx->rx_message);
        device_ctx->rx_message_size = 0;
    }


    device_ctx->rx_message = (uint8_t*) malloc(device_ctx->rx_secret_message_size * sizeof(uint8_t));
    device_ctx->rx_message_size = device_ctx->rx_secret_message_size;

    aes_gcm_decrypt(
            device_ctx->aes_key,
            device_ctx->init_vector,
            device_ctx->tx_message,
            device_ctx->aad,
            device_ctx->rx_secret_message,
            device_ctx->rx_secret_message_size,
            message_auth_tag
    );



    if (memcmp(message_auth_tag, device_ctx->auth_tag, SECURITY_AUTH_TAG_SIZE) == 0) {
        return NETWORK_COMPROMITTED_MESSAGE;
    }

    return NETWORK_OK;
}


uint8_t network_parse_byte_array_into_packet(LoRa_Packet* packet, uint8_t* byte_arr, uint16_t arr_size){
    // size needs to be at least 9 bytes (header + payload crc)
    if (arr_size < sizeof(LoRa_Packet_Header) + 2) {
        return 1;
    }

    // copy header
    memcpy(&packet->header, byte_arr, sizeof(LoRa_Packet_Header));

    //copy payload
    memcpy(&packet->payload.payload, &byte_arr[sizeof(LoRa_Packet_Header)], packet->header.payload_size);
    // copy payload crc
    memcpy(&packet->payload.payload_crc, &byte_arr[arr_size - 2], sizeof(uint16_t));

    return 0;
}

void network_parse_packet_into_byte_array(LoRa_Packet* packet, uint8_t* byte_arr){
    // copy header
    memcpy(byte_arr, &packet->header, sizeof(LoRa_Packet_Header));
    // copy payload
    memcpy(&byte_arr[sizeof(LoRa_Packet_Header)], &packet->payload.payload, packet->header.payload_size);
    // copy payload crc
    memcpy(&byte_arr[sizeof(LoRa_Packet_Header) + packet->header.payload_size], &packet->payload.payload_crc, sizeof(uint16_t));
}


void network_free_device_ctx(Network_Device_Context* device_ctx) {
    if (device_ctx->cipher_text != NULL) {
        free(device_ctx->cipher_text);
        device_ctx->cipher_text = NULL;
    }

    if (device_ctx->tx_secret_message != NULL) {
        free(device_ctx->tx_secret_message);
        device_ctx->tx_secret_message = NULL;
        device_ctx->tx_secret_message_size = 0;
    }

    if (device_ctx->rx_secret_message != NULL) {
        free(device_ctx->rx_secret_message);
        device_ctx->rx_secret_message = NULL;
        device_ctx->rx_secret_message_size = 0;
    }

    if (device_ctx->tx_message != NULL) {
        free(device_ctx->tx_message);
        device_ctx->tx_message = NULL;
        device_ctx->tx_message_size = 0;
    }

    if (device_ctx->rx_message != NULL) {
        free(device_ctx->rx_message);
        device_ctx->rx_message = NULL;
        device_ctx->rx_message_size = 0;
    }

    if (device_ctx->packet_tx_buff != NULL) {
        free(device_ctx->packet_tx_buff);
        device_ctx->packet_tx_buff = NULL;
    }

    if (device_ctx->packet_rx_buff != NULL) {
        free(device_ctx->packet_rx_buff);
        device_ctx->packet_rx_buff = NULL;
    }
}




void network_free_device_cipher_txt(Network_Device_Context* device_ctx){
    if (device_ctx->cipher_text != NULL) {
        free(device_ctx->cipher_text);
        device_ctx->cipher_text = NULL;
    }
}


void network_free_device_tx_secret_message(Network_Device_Context* device_ctx) {
    if (device_ctx->tx_secret_message != NULL) {
        free(device_ctx->tx_secret_message);
        device_ctx->tx_secret_message = NULL;
        device_ctx->tx_secret_message_size = 0;
    }
}


void network_free_device_rx_secret_message(Network_Device_Context* device_ctx) {
    if (device_ctx->rx_secret_message != NULL) {
        free(device_ctx->rx_secret_message);
        device_ctx->rx_secret_message = NULL;
        device_ctx->rx_secret_message_size = 0;
    }
}


void network_free_device_tx_message(Network_Device_Context* device_ctx) {
    if (device_ctx->tx_message != NULL) {
        free(device_ctx->tx_message);
        device_ctx->tx_message = NULL;
        device_ctx->tx_message_size = 0;
    }
}

void network_free_device_rx_message(Network_Device_Context* device_ctx) {
    if (device_ctx->rx_message != NULL) {
        free(device_ctx->rx_message);
        device_ctx->rx_message = NULL;
        device_ctx->rx_message_size = 0;
    }
}


void network_free_device_network_rx_buff(Network_Device_Context* device_ctx) {
    if (device_ctx->packet_rx_buff != NULL) {
        free(device_ctx->packet_rx_buff);
        device_ctx->packet_rx_buff = NULL;
    }
}

void network_free_device_network_tx_buff(Network_Device_Context* device_ctx) {
    if (device_ctx->packet_tx_buff != NULL) {
        free(device_ctx->packet_tx_buff);
        device_ctx->packet_tx_buff = NULL;
    }
}
