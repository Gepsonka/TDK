//
// Created by molnar on 2023.02.14..
//

#ifndef FLIGHT_COMPUTER_NETWORK_H
#define FLIGHT_COMPUTER_NETWORK_H

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "security.h"
#include "lora.h"

typedef enum {
    NETWORK_OK = 0x00,
    NETWORK_ERR = 0x01,
    NETWORK_OUT_OF_MEMORY = 0x02,
    NETWORK_UNAUTHENTICATED = 0x03
} network_operation_t;

typedef enum {
    ONLINE,
    OFFLINE,
    KEY_EXCHANGE_STARTED,
} Network_Device_Status;

typedef struct {
    uint8_t address;
    Network_Device_Status status;
    uint8_t init_vector[SECURITY_INIT_VECTOR_SIZE];
    uint8_t aes_key[SECURITY_AES_KEY_SIZE_BYTE];
    uint8_t aad[SECURITY_ADDITIONAL_AUTH_DATA_SIZE];
    uint8_t rx_auth_tag[SECURITY_AUTH_TAG_SIZE]; // Used for checking the received data confidentiality
    uint8_t tx_auth_tag[SECURITY_AUTH_TAG_SIZE]; // Sent with the encrypted message
    uint8_t* cipher_text;
    uint8_t* tx_secret_message;
    uint16_t tx_secret_message_size;
    uint8_t* rx_secret_message; // to be decrypted into rx_message
    uint16_t rx_secret_message_size;
    uint8_t* tx_message; // to be encrypted into tx_secret_message
    uint16_t tx_message_size;
    uint8_t* rx_message;
    uint16_t rx_message_size;
    LoRa_Packet* network_tx_buff; // assembled packets to be sent to device
    LoRa_Packet* network_rx_buff; // stores last received packets from device
} Network_Device_Context;

typedef struct {
    Network_Device_Context* device_contexts;
    uint8_t num_of_devices;
} Network_Device_Container;

void network_init_device_container(Network_Device_Container* device_cont);
void construct_message_from_packets(Network_Device_Context* device_ctx);
void deconstruct_message_into_packets(Network_Device_Context* device_ctx);
network_operation_t network_add_device(Network_Device_Container* device_cont, uint8_t dev_addr);
void network_generate_security_credentials_for_device(Network_Device_Context* device_ctx);
void network_encrypt_device_message(Network_Device_Context* device_ctx);
network_operation_t network_decrypt_device_message(Network_Device_Context* device_ctx);
void send_packets_for_tx(Network_Device_Context* device_ctx);
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
void network_free_device_network_rx_buff(Network_Device_Context* device_ctx);
void network_free_device_network_tx_buff(Network_Device_Context* device_ctx);


#endif //FLIGHT_COMPUTER_NETWORK_H
