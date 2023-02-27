//
// Created by molnar on 2023.02.14..
//
#include "network.h"

Network_Device_Container device_container;

TaskHandle_t network_rx_packet_handler;
QueueHandle_t packet_rx_queue;
TaskHandle_t network_device_processor_handler;
QueueHandle_t network_device_processor_queue;

extern SemaphoreHandle_t joystick_semaphore_handle;
extern Joystick_Direction joysctick_state;

extern QueueHandle_t lora_tx_queue;




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

void network_uav_temporary_controller_task(void* pvParameters) {
    Network_Device_Context* device_to_send = get_device_from_arp(&device_container, 0x01);
    if (device_to_send == NULL) {
        ESP_LOGI("network", "Device in arp.");
    } else {
        ESP_LOGI("network", "Device not in arp.");
    }
    device_to_send->tx_secret_message = (uint8_t*) malloc(3 * sizeof(uint8_t));
    if (device_to_send->tx_secret_message == NULL) {
        ESP_LOGI("network", "Out of memory temp.");
    }
    device_to_send->tx_message_size = 3 * sizeof(uint8_t);
    while (1) {
        if (xSemaphoreTake(joystick_semaphore_handle, portMAX_DELAY) == pdTRUE) {
            memset(&device_to_send->tx_secret_message[0], joysctick_state, sizeof(uint8_t));
            xSemaphoreGive(joystick_semaphore_handle);
        }

        uint16_t raw_val;
        adc2_get_raw(ADC_CHANNEL, ADC_WIDTH, &raw_val);
        memset(&device_to_send->tx_secret_message[1], raw_val, sizeof(uint16_t));

        construct_message_from_packets(device_to_send);
        set_packets_for_tx(device_to_send, &lora_tx_queue);

        vTaskDelay(100 / portTICK_PERIOD_MS);

    }
}

void network_device_processor_task(void* pvParameters){
    Network_Device_Container* dev_ctnr = (Network_Device_Container*) pvParameters;
    uint8_t dev_addr;
    Network_Device_Context* device_ctx;
    while (1) {
        if( xQueueReceive(packet_rx_queue, &dev_addr, portMAX_DELAY) == pdPASS ) {
            device_ctx = get_device_from_arp(dev_ctnr, dev_addr);
            if (device_ctx == NULL) {
                continue;
            }

            if (device_ctx->is_packet_rx_buff_empty == true) {
                continue;
            }

            construct_message_from_packets(device_ctx);

            // custom impl
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


void network_init(Network_Device_Container* device_cont)
{
    network_add_device(device_cont, 0x01);
    device_cont->device_contexts[0].status = ONLINE;
    packet_rx_queue = xQueueCreate(15, sizeof(LoRa_Packet*));
    xTaskCreate(network_packet_rx_handler_task, "PacketReceiveTask", 4096, &device_container, 200, &network_rx_packet_handler);
    network_device_processor_queue = xQueueCreate(20, sizeof(uint8_t));
    xTaskCreate(network_packet_rx_handler_task, "DeviceProcessorTask", 4096, &device_container, 1, &network_device_processor_handler);
    xTaskCreate(network_uav_temporary_controller_task, "UAVControllerTask", 4096, NULL, 1, NULL);

}

network_operation_t network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr)
{
    Network_Device_Context new_device;
    new_device.address = dev_addr;
    new_device.status = ADDING_DEVICE_TO_NETWORK;
    new_device.connection_status = CONNECTION_ESTABLISHED;
    new_device.tx_secret_message_size = 0;
    new_device.rx_secret_message_size = 0;
    new_device.tx_message_size = 0;
    new_device.rx_message_size = 0;
    new_device.num_of_faulty_packets = 0;
    new_device.is_packet_rx_buff_empty = true;
    new_device.is_packet_tx_buff_empty = true;

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
    if (device_ctx->is_packet_rx_buff_empty == true) {
        return NETWORK_BUFFER_EMPTY_ERROR;
    }

    // free both message rx buffers
    if (device_ctx->rx_secret_message_size != 0) {
        free(device_ctx->rx_secret_message);
        device_ctx->rx_secret_message_size = 0;
    }

    if (device_ctx->rx_message_size != 0) {
        free(device_ctx->rx_message);
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

    free(device_ctx->packet_rx_buff);
    device_ctx->is_packet_rx_buff_empty = true;

    return NETWORK_OK;
}


uint8_t deconstruct_message_into_packets(Network_Device_Context *device_ctx) {
    if (device_ctx->is_packet_tx_buff_empty != true) {
        free(device_ctx->packet_tx_buff);
        device_ctx->is_packet_tx_buff_empty = true;
    }

    if (device_ctx->status == ONLINE) { // device is authenticated, only encrypted message is accepted
        uint8_t num_of_packets = device_ctx->tx_secret_message_size / LORA_PAYLOAD_MAX_SIZE +
                (device_ctx->tx_secret_message_size / LORA_PAYLOAD_MAX_SIZE != 0);
        uint8_t last_packet_payload_size = device_ctx->tx_secret_message_size % LORA_PAYLOAD_MAX_SIZE;

        device_ctx->packet_tx_buff = (LoRa_Packet*) malloc(num_of_packets * sizeof(LoRa_Packet));
        if (device_ctx->packet_tx_buff == NULL) {
            return NETWORK_OUT_OF_MEMORY;
        }

        device_ctx->is_packet_tx_buff_empty = false;

        // test thoroughly!!
        for (uint8_t i = 0; i < num_of_packets; i++) {
            device_ctx->packet_tx_buff[i].header.num_of_packets = num_of_packets;
            device_ctx->packet_tx_buff[i].header.payload_size = i == num_of_packets - 1 ? last_packet_payload_size : LORA_PAYLOAD_MAX_SIZE;
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

        device_ctx->is_packet_tx_buff_empty = false;

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


void network_encrypt_device_message(Network_Device_Context* device_ctx){
    if (device_ctx->tx_secret_message_size != 0) {
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

    if (device_ctx->rx_message_size != 0) {
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

void set_packets_for_tx(Network_Device_Context* device_ctx, QueueHandle_t* lora_tx_queue_ptr) {
    if (device_ctx->is_packet_tx_buff_empty == true) {
        return;
    }

    for (uint8_t i = 0; i < device_ctx->packet_tx_buff->header.num_of_packets; i++) {
        xQueueSend(*lora_tx_queue_ptr, &device_ctx->packet_tx_buff[i], portMAX_DELAY);
    }

}


void network_free_device_ctx(Network_Device_Context* device_ctx) {
    if (device_ctx->cipher_text_size != 0) {
        free(device_ctx->cipher_text);
        device_ctx->cipher_text_size = 0;
    }

    if (device_ctx->tx_secret_message_size != 0) {
        free(device_ctx->tx_secret_message);
        device_ctx->tx_secret_message_size = 0;
    }

    if (device_ctx->rx_secret_message_size != 0) {
        free(device_ctx->rx_secret_message);
        device_ctx->rx_secret_message_size = 0;
    }

    if (device_ctx->tx_message_size != 0) {
        free(device_ctx->tx_message);
        device_ctx->tx_message_size = 0;
    }

    if (device_ctx->rx_message_size != 0) {
        free(device_ctx->rx_message);
        device_ctx->rx_message_size = 0;
    }

    if (device_ctx->is_packet_tx_buff_empty != true) {
        free(device_ctx->packet_tx_buff);
        device_ctx->is_packet_tx_buff_empty = true;
    }

    if (device_ctx->is_packet_rx_buff_empty != true) {
        free(device_ctx->packet_rx_buff);
        device_ctx->is_packet_rx_buff_empty = true;
    }



}

void network_free_device_cipher_txt(Network_Device_Context* device_ctx){
    if (device_ctx->cipher_text_size != 0) {
        free(device_ctx->cipher_text);
        device_ctx->cipher_text_size = 0;
    }
}


void network_free_device_tx_secret_message(Network_Device_Context* device_ctx) {
    if (device_ctx->tx_secret_message_size != 0) {
        free(device_ctx->tx_secret_message);
        device_ctx->tx_secret_message_size = 0;
    }
}


void network_free_device_rx_secret_message(Network_Device_Context* device_ctx) {
    if (device_ctx->rx_secret_message_size != 0) {
        free(device_ctx->rx_secret_message);
        device_ctx->rx_secret_message_size = 0;
    }
}


void network_free_device_tx_message(Network_Device_Context* device_ctx) {
    if (device_ctx->tx_message_size != 0) {
        free(device_ctx->tx_message);
        device_ctx->tx_message_size = 0;
    }
}

void network_free_device_rx_message(Network_Device_Context* device_ctx) {
    if (device_ctx->rx_message_size != 0) {
        free(device_ctx->rx_message);
        device_ctx->rx_message_size = 0;
    }
}


void network_free_device_network_rx_buff(Network_Device_Context* device_ctx) {
    if (device_ctx->is_packet_rx_buff_empty != true) {
        free(device_ctx->packet_rx_buff);
        device_ctx->is_packet_rx_buff_empty = true;
    }
}

void network_free_device_network_tx_buff(Network_Device_Context* device_ctx) {
    if (device_ctx->is_packet_tx_buff_empty != true) {
        free(device_ctx->packet_tx_buff);
        device_ctx->is_packet_tx_buff_empty = true;
    }
}

