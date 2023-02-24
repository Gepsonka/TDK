//
// Created by molnar on 2023.02.14..
//

#ifndef FLIGHT_COMPUTER_NETWORK_H
#define FLIGHT_COMPUTER_NETWORK_H

#include <stdint-gcc.h>
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h" 
#include "security.h"
#include "lora.h"
#include "math.h"

typedef enum {
    NETWORK_OK = 0x00,
    NETWORK_ERR = 0x01,
    NETWORK_OUT_OF_MEMORY = 0x02,
    NETWORK_UNAUTHENTICATED = 0x03,
    NETWORK_COMPROMITTED_MESSAGE = 0x04,
    NETWORK_BUFFER_EMPTY_ERROR,
} network_operation_t;

typedef enum {
    ONLINE,
    OFFLINE,
    KEY_EXCHANGE_STARTED,
    PUBLIC_KEY_SENT,
    DEVICE_PUBLIC_KEY_RECEIVED,

    // device then must send back an ok packet
    // indicating everything went ok, after ok packet received
    // device becomes online
    DEVICE_NETWORK_CREDENTIALS_VERIFIED,
    UNAUTHORIZED,

    // status when first package is received from device
    // if the device is not initializing key exchange
    // return unauthorized message and set the device status to
    // UNAUTHORIZED
    ADDING_DEVICE_TO_NETWORK,
} Network_Device_Status;

typedef enum {
    MESSAGE_SENT,
    MESSAGE_RECEIVED,
    RECEIVING_MESSAGE,
    WAITING_FOR_PACKET_CORRECTION,
    CONNECTION_ESTABLISHED,
} Network_Connection_Status;


typedef struct {
    uint8_t address;
    Network_Device_Status status;
    Network_Connection_Status connection_status;
    uint8_t init_vector[SECURITY_INIT_VECTOR_SIZE];
    uint8_t aes_key[SECURITY_AES_KEY_SIZE_BYTE];
    uint8_t aad[SECURITY_ADDITIONAL_AUTH_DATA_SIZE];
    uint8_t auth_tag[SECURITY_AUTH_TAG_SIZE];
    uint8_t* cipher_text;
    uint8_t* tx_secret_message;
    uint16_t tx_secret_message_size;
    uint8_t* rx_secret_message; // to be decrypted into rx_message
    uint16_t rx_secret_message_size;
    uint8_t* tx_message; // to be encrypted into tx_secret_message
    uint16_t tx_message_size;
    uint8_t* rx_message;
    uint16_t rx_message_size;
    LoRa_Packet* packet_tx_buff; // assembled packets to be sent to device
    LoRa_Packet* packet_rx_buff; // stores last received packets from device
    uint8_t received_packets; // for security measurements when requesting resend of corrupted packets
    uint8_t* packet_num_of_faulty_packets; // for packet correction
    uint8_t num_of_faulty_packets;
} Network_Device_Context;

typedef struct {
    Network_Device_Context* device_contexts;
    uint8_t num_of_devices;
} Network_Device_Container;

void network_device_processor_task(void* pvParameters);
void network_packet_rx_handler_task(void* pvParameters);
uint8_t network_parse_byte_array_into_packet(LoRa_Packet* packet, uint8_t* byte_arr, uint16_t arr_size);
void network_parse_packet_into_byte_array(LoRa_Packet* packet, uint8_t* byte_arr);
void network_init(Network_Device_Container* device_cont);
network_operation_t network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr);
uint8_t check_packet_crc(LoRa_Packet* packet);
uint8_t construct_message_from_packets(Network_Device_Context* device_ctx);
uint8_t deconstruct_message_into_packets(Network_Device_Context *device_ctx);
void network_generate_security_credentials_for_device(Network_Device_Context* device_ctx);
void network_encrypt_device_message(Network_Device_Context* device_ctx);
network_operation_t network_decrypt_device_message(Network_Device_Context* device_ctx);
void set_packets_for_tx(Network_Device_Context* device_ctx);
/// Extracts auth tag from rx_secret_message, put the tag into rx_auth_tag
/// \param device_ctx
void network_get_auth_tag_from_secret_message(Network_Device_Context* device_ctx);

void process_decrypted_naked_message(Network_Device_Context* device_ctx);

/// Free up device dynamic buffers.
/// \param device_ctx device context
void network_free_device_ctx(Network_Device_Context* device_ctx);
void network_free_device_cipher_txt(Network_Device_Context* device_ctx);
void network_free_device_tx_secret_message(Network_Device_Context* device_ctx);
void network_free_device_rx_secret_message(Network_Device_Context* device_ctx);
void network_free_device_tx_message(Network_Device_Context* device_ctx);
void network_free_device_rx_message(Network_Device_Context* device_ctx);
void network_free_device_network_rx_buff(Network_Device_Context* device_ctx);
void network_free_device_network_tx_buff(Network_Device_Context* device_ctx);


#endif //FLIGHT_COMPUTER_NETWORK_H
