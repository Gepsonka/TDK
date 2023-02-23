//
// Created by molnar on 2023.02.14..
//
#include "network.h"

Network_Device_Container device_container;
TaskHandle_t network_rx_packet_handler;
QueueHandle_t packet_rx_queue;

void network_packet_rx_handler_task(void* pvParameters){
    LoRa_Packet received_packet;
    while (1) {
        if( xQueueReceive(packet_rx_queue, &received_packet, portMAX_DELAY) == pdPASS ) {
            //process packet
        }
    }
}


void network_init(Network_Device_Container* device_cont)
{
    device_cont->device_contexts = NULL;
    device_cont->num_of_devices = 0;
    packet_rx_queue = xQueueCreate(15, sizeof(LoRa_Packet*));

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
    if (device_ctx->packet_rx_buff == NULL) {
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

    free(device_ctx->packet_rx_buff);
    device_ctx->packet_rx_buff = NULL;

    return NETWORK_OK;
}


uint8_t deconstruct_message_into_packets(Network_Device_Context *device_ctx) {
    if (device_ctx->packet_tx_buff != NULL) {
        free(device_ctx->packet_tx_buff);
        device_ctx->packet_tx_buff = NULL;
    }

    if (device_ctx->status == ONLINE) { // device is authenticated, only encrypted message is accepted
        uint8_t num_of_packets = device_ctx->tx_secret_message_size / LORA_PAYLOAD_MAX_SIZE +
                (device_ctx->tx_secret_message_size / LORA_PAYLOAD_MAX_SIZE != 0);
        uint8_t last_packet_payload_size = device_ctx->tx_secret_message_size % LORA_PAYLOAD_MAX_SIZE;

        device_ctx->packet_tx_buff = (LoRa_Packet*) malloc(num_of_packets * sizeof(LoRa_Packet));
        if (device_ctx->packet_tx_buff == NULL) {
            return NETWORK_OUT_OF_MEMORY;
        }

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
